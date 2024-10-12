//#include <Arduino.h>
#include <WiFiManager.h>    // WiFiManager Bibliothek
#include <AsyncTCP.h>       // Für den Webserver auf ESP32
#include <ESPAsyncWebServer.h>
#include <Adafruit_TinyUSB.h>
//#include <hidcomposite.h>
//#include <usbhub.h>
#include <SPI.h>

// USB Host using MAX3421E: SPI, CS, INT
#if defined(ARDUINO_METRO_ESP32S2)
  Adafruit_USBH_Host USBHost(&SPI, 15, 14);
#elif defined(ARDUINO_ADAFRUIT_FEATHER_ESP32_V2)
  Adafruit_USBH_Host USBHost(&SPI, 33, 15);
#elif defined(ARDUINO_ADAFRUIT_FEATHER_ESP32C6)
  Adafruit_USBH_Host USBHost(&SPI, 8, 7);
#elif defined(ARDUINO_ESP32C3_DEV)
  Adafruit_USBH_Host USBHost(&SPI, 10, 7);
#else
  // Default CS and INT are pin 10, 9
  //Adafruit_USBH_Host USBHost(&SPI, 10, 9);
  Adafruit_USBH_Host USBHost(&SPI, 5, 34);
#endif

#define MY_MISO  23  // Standardmäßig GPIO12
#define MY_MOSI  25  // Standardmäßig GPIO13
#define MY_SCK   18  // Standardmäßig GPIO14
#define MY_CS    5  // Standardmäßig GPIO15
#define MY_IRQ   34  // IRQ Pin für MAX3421E

// Language ID: English
#define LANGUAGE_ID 0x0409

typedef struct {
  tusb_desc_device_t desc_device;
  uint16_t manufacturer[32];
  uint16_t product[48];
  uint16_t serial[16];
  bool mounted;
} dev_info_t;

// CFG_TUH_DEVICE_MAX is defined by tusb_config header
dev_info_t dev_info[CFG_TUH_DEVICE_MAX] = { 0 };

String hidData = "";  // String, um empfangene HID-Daten zu speichern
AsyncWebServer server(80); // Webserver auf Port 80

// Override HIDComposite to be able to select which interface we want to hook into
//class HIDSelector : public HIDComposite {
//public:
//    HIDSelector(USB *p) : HIDComposite(p) {}

//protected:
//    void ParseHIDData(USBHID *hid, uint8_t ep, bool is_rpt_id, uint8_t len, uint8_t *buf);
//    bool SelectInterface(uint8_t iface, uint8_t proto);
//};

//// Return true for the interface we want to hook into
//bool HIDSelector::SelectInterface(uint8_t iface, uint8_t proto) {
//    if (proto != 0) return true;  // Hier wird die Protokoll-ID gefiltert
//    return false;
//}

// Will be called for all HID data received from the USB interface
//void HIDSelector::ParseHIDData(USBHID *hid, uint8_t ep, bool is_rpt_id, uint8_t len, uint8_t *buf) {
//    if (len && buf) {
//        Serial.print("HID Data: ");
//        for (uint8_t i = 0; i < len; i++) {
//            char hexChar[5];
//            sprintf(hexChar, "%02X ", buf[i]);
//            Serial.print(hexChar);
//            hidData += String(hexChar);  // Speichere die empfangenen Daten im String
//        }
//        Serial.println();
//    }
//}

//USB Usb;
//HIDSelector hidSelector(&Usb);

// Funktion zum Einrichten des Webservers
void setupWebServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = "<html><body><h1>USB HID Data</h1>";
        html += "<p>Data: " + hidData + "</p>";
        html += "</body></html>";
        request->send(200, "text/html", html);  // HTML-Seite mit HID-Daten senden
    });

    server.begin();  // Starte den Server
}

void setup() {
    Serial.begin(115200);
    while (!Serial);  // Warte auf serielle Verbindung
    Serial.println("Start");

    // SPI Pins definieren
    //SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_SS);
 
  #ifdef ESP32
  Serial.println("ESP32 basiertes Board erkannt!");
  #endif
    // WiFiManager initialisieren
    WiFiManager wm;

    // Automatisches Verbinden oder Setup-Seite anzeigen, wenn keine Verbindung hergestellt werden kann
    if (!wm.autoConnect("ESP_USB_Host")) {
        Serial.println("Failed to connect to WiFi");  // Diese Zeile wird nur ausgegeben, wenn die Verbindung fehlschlägt
        // Hier kein ESP.restart(), da der ESP nun im AP-Modus ist und die Setup-Seite bereitstellt
    }
    Serial.println("Connected to WiFi!");
    Serial.println(WiFi.localIP());  // Zeigt die IP-Adresse an

    // Webserver einrichten
    setupWebServer();

    // USB-Host initialisieren
if (USBHost.begin(1) == -1) {
    Serial.println("USB Host Shield initialization failed.");
    return;
} else {
    Serial.println("USB Host Shield initialized.");
}

    // Set this to higher values to enable more debug information
    //UsbDEBUGlvl = 0xff;

    delay(200);

}

void loop() {
    USBHost.task();
    Serial.println("USB Host Task running...");
    delay(1000);
    //Serial.flush();
    //Usb.Task();  // USB-Task ausführen
}

//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+
void print_device_descriptor(tuh_xfer_t *xfer);

void utf16_to_utf8(uint16_t *temp_buf, size_t buf_len);

void print_lsusb(void) {
  bool no_device = true;
  for (uint8_t daddr = 1; daddr < CFG_TUH_DEVICE_MAX + 1; daddr++) {
    // TODO can use tuh_mounted(daddr), but tinyusb has an bug
    // use local connected flag instead
    dev_info_t *dev = &dev_info[daddr - 1];
    if (dev->mounted) {
      Serial.printf("Device %u: ID %04x:%04x %s %s\r\n", daddr,
                    dev->desc_device.idVendor, dev->desc_device.idProduct,
                    (char *) dev->manufacturer, (char *) dev->product);

      no_device = false;
    }
  }

  if (no_device) {
    Serial.println("No device connected (except hub)");
  }
}

// Invoked when device is mounted (configured)
void tuh_mount_cb(uint8_t daddr) {
    Serial.printf("Device attached, address = %d\r\n", daddr);
    dev_info_t *dev = &dev_info[daddr - 1];
    dev->mounted = true;

    // Get Device Descriptor
    tuh_descriptor_get_device(daddr, &dev->desc_device, 18, print_device_descriptor, 0);
}

/// Invoked when device is unmounted (bus reset/unplugged)
void tuh_umount_cb(uint8_t daddr) {
  Serial.printf("Device removed, address = %d\r\n", daddr);
  dev_info_t *dev = &dev_info[daddr - 1];
  dev->mounted = false;

  // print device summary
  print_lsusb();
}

void print_device_descriptor(tuh_xfer_t *xfer) {
  if (XFER_RESULT_SUCCESS != xfer->result) {
    Serial.printf("Failed to get device descriptor\r\n");
    return;
  }

  uint8_t const daddr = xfer->daddr;
  dev_info_t *dev = &dev_info[daddr - 1];
  tusb_desc_device_t *desc = &dev->desc_device;

  Serial.printf("Device %u: ID %04x:%04x\r\n", daddr, desc->idVendor, desc->idProduct);
  Serial.printf("Device Descriptor:\r\n");
  Serial.printf("  bLength             %u\r\n"     , desc->bLength);
  Serial.printf("  bDescriptorType     %u\r\n"     , desc->bDescriptorType);
  Serial.printf("  bcdUSB              %04x\r\n"   , desc->bcdUSB);
  Serial.printf("  bDeviceClass        %u\r\n"     , desc->bDeviceClass);
  Serial.printf("  bDeviceSubClass     %u\r\n"     , desc->bDeviceSubClass);
  Serial.printf("  bDeviceProtocol     %u\r\n"     , desc->bDeviceProtocol);
  Serial.printf("  bMaxPacketSize0     %u\r\n"     , desc->bMaxPacketSize0);
  Serial.printf("  idVendor            0x%04x\r\n" , desc->idVendor);
  Serial.printf("  idProduct           0x%04x\r\n" , desc->idProduct);
  Serial.printf("  bcdDevice           %04x\r\n"   , desc->bcdDevice);

  // Get String descriptor using Sync API
  Serial.printf("  iManufacturer       %u     ", desc->iManufacturer);
  if (XFER_RESULT_SUCCESS ==
      tuh_descriptor_get_manufacturer_string_sync(daddr, LANGUAGE_ID, dev->manufacturer, sizeof(dev->manufacturer))) {
    utf16_to_utf8(dev->manufacturer, sizeof(dev->manufacturer));
    Serial.printf((char *) dev->manufacturer);
  }
  Serial.printf("\r\n");

  Serial.printf("  iProduct            %u     ", desc->iProduct);
  if (XFER_RESULT_SUCCESS ==
      tuh_descriptor_get_product_string_sync(daddr, LANGUAGE_ID, dev->product, sizeof(dev->product))) {
    utf16_to_utf8(dev->product, sizeof(dev->product));
    Serial.printf((char *) dev->product);
  }
  Serial.printf("\r\n");

  Serial.printf("  iSerialNumber       %u     ", desc->iSerialNumber);
  if (XFER_RESULT_SUCCESS ==
      tuh_descriptor_get_serial_string_sync(daddr, LANGUAGE_ID, dev->serial, sizeof(dev->serial))) {
    utf16_to_utf8(dev->serial, sizeof(dev->serial));
    Serial.printf((char *) dev->serial);
  }
  Serial.printf("\r\n");

  Serial.printf("  bNumConfigurations  %u\r\n", desc->bNumConfigurations);

  // print device summary
  print_lsusb();
}

//--------------------------------------------------------------------+
// String Descriptor Helper
//--------------------------------------------------------------------+

static void _convert_utf16le_to_utf8(const uint16_t *utf16, size_t utf16_len, uint8_t *utf8, size_t utf8_len) {
  // TODO: Check for runover.
  (void) utf8_len;
  // Get the UTF-16 length out of the data itself.

  for (size_t i = 0; i < utf16_len; i++) {
    uint16_t chr = utf16[i];
    if (chr < 0x80) {
      *utf8++ = chr & 0xff;
    } else if (chr < 0x800) {
      *utf8++ = (uint8_t) (0xC0 | (chr >> 6 & 0x1F));
      *utf8++ = (uint8_t) (0x80 | (chr >> 0 & 0x3F));
    } else {
      // TODO: Verify surrogate.
      *utf8++ = (uint8_t) (0xE0 | (chr >> 12 & 0x0F));
      *utf8++ = (uint8_t) (0x80 | (chr >> 6 & 0x3F));
      *utf8++ = (uint8_t) (0x80 | (chr >> 0 & 0x3F));
    }
    // TODO: Handle UTF-16 code points that take two entries.
  }
}

// Count how many bytes a utf-16-le encoded string will take in utf-8.
static int _count_utf8_bytes(const uint16_t *buf, size_t len) {
  size_t total_bytes = 0;
  for (size_t i = 0; i < len; i++) {
    uint16_t chr = buf[i];
    if (chr < 0x80) {
      total_bytes += 1;
    } else if (chr < 0x800) {
      total_bytes += 2;
    } else {
      total_bytes += 3;
    }
    // TODO: Handle UTF-16 code points that take two entries.
  }
  return total_bytes;
}

void utf16_to_utf8(uint16_t *temp_buf, size_t buf_len) {
  size_t utf16_len = ((temp_buf[0] & 0xff) - 2) / sizeof(uint16_t);
  size_t utf8_len = _count_utf8_bytes(temp_buf + 1, utf16_len);

  _convert_utf16le_to_utf8(temp_buf + 1, utf16_len, (uint8_t *) temp_buf, buf_len);
  ((uint8_t *) temp_buf)[utf8_len] = '\0';
}

