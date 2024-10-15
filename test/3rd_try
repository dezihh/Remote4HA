#include <usbhub.h>
#include <hiduniversal.h>

#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

// Structure to store key state and press duration
struct KeyState {
    bool isPressed;
    unsigned long pressTime;
};

// Class to handle and print raw HID data
class RawHIDParser : public HIDReportParser {
  protected:
    static const uint8_t MAX_KEYS = 10;  // Limit the number of tracked keys to reduce memory usage
    KeyState keyStates[MAX_KEYS];        // Store state for a limited number of keys
    void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) override;
    char HIDToAscii(uint8_t hidKeyCode);

  public:
    RawHIDParser() {
      // Initialize key states
      for (int i = 0; i < MAX_KEYS; i++) {
          keyStates[i].isPressed = false;
          keyStates[i].pressTime = 0;
      }
    }
};

// Function to map HID key codes to ASCII characters
char RawHIDParser::HIDToAscii(uint8_t hidKeyCode) {
    switch (hidKeyCode) {
        case 0x1E: return '1';  // HID key code for '1'
        case 0x1F: return '2';  // HID key code for '2'
        // Add more mappings as needed
        default: return '?';    // Unknown key
    }
}

// Parse and print raw HID data, and track press duration
void RawHIDParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
    Serial.print("Raw HID Data: ");
    for (uint8_t i = 0; i < len; i++) {
        Serial.print(buf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

    // Process key presses (starting from buf[2], as buf[0] and buf[1] are modifiers/reserved)
    for (int i = 2; i < len && i < MAX_KEYS; i++) {
        if (buf[i] != 0) { // Non-zero value means a key is pressed
            if (!keyStates[i].isPressed) {  // If the key was not previously pressed
                keyStates[i].isPressed = true;
                keyStates[i].pressTime = millis();  // Record the time when key was pressed
                char key = HIDToAscii(buf[i]);  // Translate the keycode
                Serial.print("Key Pressed: ");
                Serial.println(key);
            }
        } else {
            // If we encounter 0, we assume keys have been released
            if (keyStates[i].isPressed) {  // If a key was previously pressed
                unsigned long duration = millis() - keyStates[i].pressTime;  // Calculate how long the key was held
                char key = HIDToAscii(buf[i]);  // Translate the keycode
                Serial.print("Key Released: ");
                Serial.print(key);
                Serial.print(" | Held for: ");
                Serial.print(duration);
                Serial.println(" ms");

                // Reset the state
                keyStates[i].isPressed = false;
                keyStates[i].pressTime = 0;
            }
        }
    }
}

// Create USB and HID objects
USB     Usb;
USBHub  *pUsbHub = nullptr;  // Pointer to USB Hub, initialize to nullptr
HIDUniversal HidUniversal(&Usb);  // Universal HID support

RawHIDParser RawParser;  // Create a raw HID parser
bool usbInitialized = false; // Flag to check if USB is initialized

void setup() {
  Serial.begin(115200);
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  Serial.println("Starting USB Host...");

  // Try initializing USB Host
  if (!usbInitialized) {
    int usbInitResult = Usb.Init();
    if (usbInitResult == -1) {
      Serial.println("USB Host initialization failed!");
      delay(5000); // Prevent rapid reinitialization in case of failure
      return; // Exit setup if initialization fails
    } else {
      usbInitialized = true;  // Set flag to true when initialized
      Serial.println("USB Host initialized successfully.");
    }

    // Check if a USB Hub is present and initialize it
    if (pUsbHub == nullptr) {
      pUsbHub = new USBHub(&Usb);
      if (pUsbHub == nullptr) {
        Serial.println("Error: Could not create USB Hub instance!");
      } else {
        Serial.println("USB Hub initialized.");
      }
    }
    
    // Set the parser for universal HID devices
    HidUniversal.SetReportParser(0, &RawParser);
    Serial.println("HID Universal Report Parser set.");
  } else {
    Serial.println("USB Host already initialized.");
  }
}

void loop() {
  // Only proceed if the USB host is initialized
  if (usbInitialized) {
    Usb.Task();  // Handle USB events
  } else {
    Serial.println("USB not initialized, skipping task.");
  }
}
