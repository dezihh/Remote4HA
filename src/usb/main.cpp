#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <BLEUtils.h>
#include <HIDTypes.h>
#include <HIDKeyboardTypes.h>
#include "EspUsbHost.h"
#include "reportMap.h"

// BLE HID service variables
BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;
uint16_t conn_id = 0xFFFF;
bool is_ble_connected = false;

// USB Host
class MyEspUsbHost : public EspUsbHost {
public:
void onKeyboardKey(uint8_t ascii, uint8_t keycode, uint8_t modifier) {
    Serial.printf("Key: %02x(%c) Modifier: %02x\n", keycode, ascii, modifier);

    // Erstellen der HID-Report-Daten: 8-Byte-Report (Modifier, Reserved, Keycode[6])
    uint8_t report[8] = {0};  // Initialisiert alle auf 0
    report[0] = modifier;     // Modifier Byte (z.B. Shift, Control, Alt)
    report[2] = keycode;      // Keycode an der dritten Position (erste Taste)

    // Senden des Tastendrucks (Keydown)
    input->setValue(report, sizeof(report));  // Setzt den Wert des HID-Reports
    input->notify();                          // Benachrichtigt den Host über die Tasteneingabe

    delay(100);  // Kurze Verzögerung, um den Tastendruck zu simulieren

    // Taste loslassen (alle Werte auf 0 setzen)
    memset(report, 0, sizeof(report));        // Setzt den gesamten Report auf 0 (Taste loslassen)
    input->setValue(report, sizeof(report));  // Leerer Report
    input->notify();                          // Benachrichtigt den Host, dass keine Taste mehr gedrückt ist
}
};

MyEspUsbHost usbHost;

class MyCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        is_ble_connected = true;
        Serial.println("BLE connected");
    }

    void onDisconnect(BLEServer* pServer) {
        is_ble_connected = false;
        Serial.println("BLE disconnected");
    }
};

void setup() {
   Serial.begin(115200);
    Serial.println("Starting setup...");

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
    // USB Tastatureingaben verarbeiten
    usbHost.task();
        // BLE-Tasks verarbeiten, um BLE-Events wie Advertising und Verbindungen zu überwachen
    if (is_ble_connected) {
        // Verbindung besteht, möglicherweise Datenübertragung hier verarbeiten
    } else {
        // Keine BLE-Verbindung, möglicherweise neu werben
        BLEDevice::startAdvertising();  // Stellt sicher, dass das Advertising läuft
    }

    delay(10);  // Kurze Pause, um die CPU nicht zu überlasten
}
