#include "EspUsbHost.h"



// USB Host
class MyEspUsbHost : public EspUsbHost {
public:
    void onKeyboardKey(uint8_t ascii, uint8_t keycode, uint8_t modifier) {
        Serial.printf("Key: %02x(%c) Modifier: %02x\n", keycode, ascii, modifier);

        }

};


MyEspUsbHost usbHost;

void setup() {
   Serial.begin(115200);
    Serial.println("Starting setup...");

     // Initialisiere USB-Keyboard
    usbHost.begin();

    Serial.println("USB Keyboard initialized");
}

void loop() {
    // USB Tastatureingaben verarbeiten
    usbHost.task();
    

}
