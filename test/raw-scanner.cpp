#include <usbhub.h>
#include <hiduniversal.h>

// Satisfy the IDE, which needs to see the include statement in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

// Class to handle and print raw HID data
class RawHIDParser : public HIDReportParser {
  protected:
    void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) override;

  public:
    RawHIDParser() {}
};

// Parse and print raw HID data
void RawHIDParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
    Serial.print("Raw HID Data: ");
    for (uint8_t i = 0; i < len; i++) {
        Serial.print(buf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

// Create USB and HID objects
USB     Usb;
USBHub  Hub(&Usb);  // Support for USB hubs
HIDUniversal HidUniversal(&Usb);  // Universal HID support

RawHIDParser RawParser;  // Create a raw HID parser

void setup() {
  Serial.begin(115200);
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  Serial.println("Starting USB Host...");

  // Initialize USB
  if (Usb.Init() == -1) {
    Serial.println("USB Host initialization failed!");
    while (1); // Stop if initialization fails
  } else {
    Serial.println("USB Host initialized successfully.");
  }

  delay(200);

  // Set the parser for universal HID devices
  HidUniversal.SetReportParser(0, &RawParser);
  Serial.println("HID Universal Report Parser set.");
}

void loop() {
  Usb.Task();  // Handle USB events
}
