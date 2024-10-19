#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <BLEUtils.h>
#include <HIDTypes.h>
#include <HIDKeyboardTypes.h>
#include "EspUsbHost.h"
#include "reportMap.h"
#include "esp_log.h" 

// BLE HID service variables
BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;
uint16_t conn_id = 0xFFFF;
bool is_ble_connected = false;

// USB Host
class MyEspUsbHost : public EspUsbHost {
public:
    bool usb_connected = false;  // Flag zum Verfolgen des USB-Geräte-Status

    void onKeyboardKey(uint8_t ascii, uint8_t keycode, uint8_t modifier) {
        usb_connected = true;  // USB-Gerät ist verbunden
        if (is_ble_connected) {  // Nur Datenübergabe, wenn BLE verbunden ist
            Serial.printf("Key: %02x(%c) Modifier: %02x\n", keycode, ascii, modifier);

            // HID-Report erstellen und senden
            uint8_t report[8] = {0};
            report[0] = modifier;
            report[2] = keycode;

            input->setValue(report, sizeof(report));
            input->notify();

            delay(100);
            memset(report, 0, sizeof(report));
            input->setValue(report, sizeof(report));
            input->notify();
        } else {
            Serial.println("BLE not connected, discarding key input.");
        }
    }

    // Diese Methode wird aufgerufen, wenn ein USB-Gerät entfernt wurde
    void onUsbDisconnect() {
        usb_connected = false;  // USB-Gerät wurde entfernt
        Serial.println("\nUSB device disconnected. Restarting USB host...");
        begin();  // Startet den USB-Host erneut, um nach neuen Geräten zu suchen
    }
};

MyEspUsbHost usbHost;

class MyCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        is_ble_connected = true;
        Serial.println("\nBLE connected");
    }

    void onDisconnect(BLEServer* pServer) {
        is_ble_connected = false;
        Serial.println("\nBLE disconnected");
    }
};

// Status für das drehende Kreuz
char spin_chars[] = {'|', '/', '-', '\\'};
int spin_index = 0;

void printStatus(const char* message) {
    // Lösche die letzte Zeile, indem du einen Wagenrücklauf sendest
    Serial.printf("\r%s %c", message, spin_chars[spin_index]);
    spin_index = (spin_index + 1) % 4;  // Zyklisch durch das Kreuz drehen
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting setup...");
  // USB-Logging auf Error setzen, um Debug-Ausgaben zu unterdrücken
    esp_log_level_set("*", ESP_LOG_NONE); // Ändere "USBH" auf den Tag der USB-Bibliothek

    // Initialisiere USB-Keyboard
    usbHost.begin();
    Serial.println("USB Keyboard initialized");

    // BLE HID setup
    BLEDevice::init("ESP32 BLE Keyboard");
    BLEServer* pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyCallbacks());

    hid = new BLEHIDDevice(pServer);
    input = hid->inputReport(1);  // Input report
    output = hid->outputReport(1); // Output report

    hid->manufacturer()->setValue("ESP32");
    hid->pnp(0x01, 0x02e5, 0xabcd, 0x0110);
    hid->hidInfo(0x00, 0x01);

    BLESecurity* pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

    // Verwende die ausgelagerte HID Report Map
    hid->reportMap((uint8_t*)reportMap, sizeof(reportMap));
    hid->startServices();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setAppearance(HID_KEYBOARD);
    pAdvertising->addServiceUUID(hid->hidService()->getUUID());
    pAdvertising->start();
    Serial.println("Waiting for a client connection to notify...");
}

void loop() {
    // USB-Tastatureingaben verarbeiten und Verbindungsstatus überwachen
    usbHost.task();

    // Wenn das USB-Gerät getrennt wurde, überprüfe den Verbindungsstatus erneut
    if (!usbHost.usb_connected) {
        printStatus("No USB device connected, waiting...");
        delay(500);  // Kurze Verzögerung, bevor erneut nach dem USB-Gerät gesucht wird
    }

    // BLE-Tasks verarbeiten, um BLE-Events wie Advertising und Verbindungen zu überwachen
    if (!is_ble_connected) {
        BLEDevice::startAdvertising();  // Wenn BLE getrennt wurde, Advertising erneut starten
        printStatus("Re-starting BLE advertising...");
        delay(500);
    }

    delay(10);  // Kurze Pause, um die CPU nicht zu überlasten
}
