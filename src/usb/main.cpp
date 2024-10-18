#include <elapsedMillis.h>
#include <usb/usb_host.h>
#include "show_desc.hpp"
#include "usbhhelp.hpp"
#include <BleKeyboard.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <esp_bt.h> // Für Bluetooth-Heap-Statistiken

// Erstelle ein BLE Keyboard Objekt
BleKeyboard bleKeyboard("BLE_USB_Keyboard", "Arduino", 100);

// Webserver auf Port 80
WebServer server(80);

// Variable für USB-Keyboard und BLE
bool isKeyboard = false;
bool isKeyboardReady = false;
uint8_t KeyboardInterval;
bool isKeyboardPolling = false;
elapsedMillis KeyboardTimer;

const size_t KEYBOARD_IN_BUFFER_SIZE = 8;
usb_transfer_t *KeyboardIn = NULL;

String lastReceivedData = ""; // Zuletzt empfangene Daten für die Anzeige im Webserver

// FreeRTOS Tasks
TaskHandle_t bleTaskHandle = NULL;

// HID-Berichtspuffer für USB-Keyboard
uint8_t hid_report[8];

// Callback für den Empfang von Daten vom USB-Keyboard
void keyboard_transfer_cb(usb_transfer_t *transfer) {
  if (Device_Handle == transfer->device_handle) {
    isKeyboardPolling = false;
    if (transfer->status == 0) {
      if (transfer->actual_num_bytes == 8) {
        uint8_t *const p = transfer->data_buffer;

        // Speichere den HID-Bericht als String für die Anzeige im Webserver
        lastReceivedData = String(p[0], HEX) + " " + String(p[1], HEX) + " " + String(p[2], HEX);

        // Übertrage die HID-Berichte über BLE
        if (bleKeyboard.isConnected()) {
          bleKeyboard.press(p[2]); // Sende den gedrückten Key
          bleKeyboard.releaseAll(); // Lasse alle Keys los
        }
      }
    }
  }
}

// Funktion zum Anzeigen der Webseite mit den gesendeten/empfangenen Daten
void handleRoot() {
  String html = "<html><body><h1>BLE-USB Tastatur</h1>";
  html += "<p>Letzte empfangene Daten: " + lastReceivedData + "</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setupWebServer() {
  // Konfiguriere Webserver
  server.on("/", handleRoot);
  server.begin();
}

// WiFiManager Initialisierung und Verbindung
void setupWiFi() {
  WiFiManager wm;
  if (!wm.autoConnect("AutoConnectAP", "password")) {
    // Neustarten, wenn keine Verbindung möglich ist
    ESP.restart();
  }
}

// Überprüfe den verfügbaren Heap und Bluetooth-Speicher
void checkMemory() {
  Serial.printf("Heap: %d, Minimum Heap: %d\n",
                esp_get_free_heap_size(),
                esp_get_minimum_free_heap_size());
}


// BLE Task, die in einem eigenen Thread läuft
void bleTask(void *pvParameters) {
  while (true) {
    if (!bleKeyboard.isConnected()) {
      checkMemory(); // Überwache den Speicher
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// Bereite das USB-Keyboard vor
void prepare_endpoint(const void *p) {
  const usb_ep_desc_t *endpoint = (const usb_ep_desc_t *)p;
  esp_err_t err;

  // Muss ein Interrupt für HID sein
  if ((endpoint->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK) != USB_BM_ATTRIBUTES_XFER_INT) {
    return;
  }
  if (endpoint->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK) {
    err = usb_host_transfer_alloc(KEYBOARD_IN_BUFFER_SIZE, 0, &KeyboardIn);
    if (err != ESP_OK) {
      KeyboardIn = NULL;
      return;
    }
    KeyboardIn->device_handle = Device_Handle;
    KeyboardIn->bEndpointAddress = endpoint->bEndpointAddress;
    KeyboardIn->callback = keyboard_transfer_cb;
    KeyboardIn->context = NULL;
    isKeyboardReady = true;
    KeyboardInterval = endpoint->bInterval;
  }
}

// USB Setup
void show_config_desc_full(const usb_config_desc_t *config_desc) {
  const uint8_t *p = &config_desc->val[0];
  static uint8_t USB_Class = 0;
  uint8_t bLength;
  for (int i = 0; i < config_desc->wTotalLength; i += bLength, p += bLength) {
    bLength = *p;
    if ((i + bLength) <= config_desc->wTotalLength) {
      const uint8_t bDescriptorType = *(p + 1);
      switch (bDescriptorType) {
        case USB_B_DESCRIPTOR_TYPE_INTERFACE:
          USB_Class = show_interface_desc(p);
          if ((USB_Class == USB_CLASS_HID) &&
              ((const usb_intf_desc_t *)p)->bInterfaceSubClass == 1 &&
              ((const usb_intf_desc_t *)p)->bInterfaceProtocol == 1) {
            isKeyboard = true;
            esp_err_t err = usb_host_interface_claim(Client_Handle, Device_Handle,
                                                     ((const usb_intf_desc_t *)p)->bInterfaceNumber,
                                                     ((const usb_intf_desc_t *)p)->bAlternateSetting);
            if (err != ESP_OK) return;
          }
          break;
        case USB_B_DESCRIPTOR_TYPE_ENDPOINT:
          if (isKeyboard && KeyboardIn == NULL) prepare_endpoint(p);
          break;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Setup USB Host
  usbh_setup(show_config_desc_full);

  // Initialisiere BLE Keyboard
  bleKeyboard.begin();

  // Starte den BLE-Task in einem separaten FreeRTOS-Thread
  xTaskCreate(bleTask, "BLE Task", 4096, NULL, 1, &bleTaskHandle);

  // Verbinde mit WiFi
  setupWiFi();

  // Starte den Webserver
  setupWebServer();
}

void loop() {
  usbh_task();

  // Prüfe, ob die Tastatur bereit ist und starte den Polling-Prozess
  if (isKeyboardReady && !isKeyboardPolling && (KeyboardTimer > KeyboardInterval)) {
    KeyboardIn->num_bytes = 8;
    esp_err_t err = usb_host_transfer_submit(KeyboardIn);
    if (err != ESP_OK) {
      return;
    }
    isKeyboardPolling = true;
    KeyboardTimer = 0;
  }

  // Webserver ausführen
  server.handleClient();
}
