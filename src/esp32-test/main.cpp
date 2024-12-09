#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <NimBLECharacteristic.h>

#include "reportMap.h"


// USB Definitions
bool usb_connected = false;
unsigned long keyPressStartTime = 550;
bool keyPressed = false;
int longKey = 500;

bool debug = false;

bool is_ble_connected = false;
NimBLEHIDDevice* hid;
NimBLECharacteristic* input;
bool connected = false;
NimBLECharacteristic* consumerInput;

void initBLE(String serverName);
void sendBLE(uint8_t modifier, uint8_t keycode);

class MyCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        connected = true;
        Serial.println("Client connected");
        sendBLE(0x0,0x66);
    }

    void onDisconnect(NimBLEServer* pServer) {
        connected = false;
        Serial.println("Client disconnected");
        NimBLEDevice::startAdvertising();
    }
};




void initBLE(String serverName) 
{
    NimBLEDevice::init(serverName.c_str());
    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyCallbacks());

    hid = new NimBLEHIDDevice(pServer);
    input = hid->inputReport(1); // <-- input REPORTID from report map
    consumerInput = hid->inputReport(2); // <-- input REPORTID for consumer control
    hid->manufacturer()->setValue("ESP32 Manufacturer");
    hid->pnp(0x02, 0x1234, 0x5678, 0x0110);
    hid->hidInfo(0x00, 0x01);

    hid->reportMap((uint8_t*)reportMap, sizeof(reportMap));
    hid->startServices();
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setAppearance(HID_KEYBOARD);
    pAdvertising->addServiceUUID(hid->hidService()->getUUID());
    pAdvertising->start();
    Serial.println("Advertising started!");
}

//void sendBLE(uint8_t modifier, uint8_t keycode, bool longPress = false) {
void sendBLE(uint8_t modifier, uint8_t keycode) {
    if (connected) {
        uint8_t report[8] = {0};
        report[0] = modifier; // Modifier (e.g., Ctrl, Shift)
        report[2] = keycode;  // Keycode
                Serial.printf("Sending HID Report: Modifier: 0x%02X, Keycode: 0x%02X\n", modifier, keycode);

        input->setValue(report, sizeof(report));
        input->notify();
        delay(100);
        report[0] = 0;
        report[2] = 0;
        input->setValue(report, sizeof(report));
        input->notify();
        delay(100);
        String sendInfo = "Sent BLE Keycode: 0x" + String(keycode, HEX) + 
                        " Modifier: 0x" + String(modifier, HEX);
        Serial.println(sendInfo);
    } else {
         Serial.println("BLE Keyboad not connected, discarding input."); 
    }
}


void setup() {
    Serial.begin(115200);
    initBLE("BLE IR Test-Router");
}

void loop() {

}
