#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <BLECharacteristic.h>
#include <WiFi.h>
#include <WebServer.h>

#include "reportMap.h"

// USB Definitions
bool usb_connected = false;
unsigned long keyPressStartTime = 550;
bool keyPressed = false;
int longKey = 500;

bool debug = false;

bool is_ble_connected = false;
BLEHIDDevice* hid;
BLECharacteristic* input;
bool connected = false;
BLECharacteristic* consumerInput;

WebServer server(80);

void initBLE(String serverName);
void sendBLE(uint8_t modifier, uint8_t keycode);
void sendConsumerBLE(uint16_t consumerKeycode);
void handleRoot();
void handleNotFound();
void handleSendKey();

class MyCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        connected = true;
        Serial.println("Client connected");
        sendBLE(0x0, 0x66);
    }

    void onDisconnect(BLEServer* pServer) {
        connected = false;
        Serial.println("Client disconnected");
        BLEDevice::startAdvertising();
    }
};

void initBLE(String serverName) {
    BLEDevice::init(serverName.c_str());
    BLEServer* pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyCallbacks());

    hid = new BLEHIDDevice(pServer);
    input = hid->inputReport(1);  // <-- input REPORTID from report map
    consumerInput = hid->inputReport(2);  // <-- input REPORTID for consumer control
    hid->manufacturer()->setValue("ESP32 Manufacturer");
    hid->pnp(0x02, 0x1234, 0x5678, 0x0110);
    hid->hidInfo(0x00, 0x01);

    hid->reportMap((uint8_t*)reportMap, sizeof(reportMap));
    hid->startServices();
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setAppearance(HID_KEYBOARD);
    pAdvertising->addServiceUUID(hid->hidService()->getUUID());
    pAdvertising->start();
    Serial.println("Advertising started!");
}

void sendBLE(uint8_t modifier, uint8_t keycode) {
    if (connected) {
        uint8_t report[8] = {0};
        report[0] = modifier;  // Modifier (e.g., Ctrl, Shift)
        report[2] = keycode;   // Keycode
        Serial.printf("Sending HID Report: Modifier: 0x%02X, Keycode: 0x%02X\n", modifier, keycode);

        input->setValue(report, sizeof(report));
        input->notify();
        delay(100);
        report[0] = 0;
        report[2] = 0;
        input->setValue(report, sizeof(report));
        input->notify();
        delay(100);
        String sendInfo = "Sent BLE Keycode: 0x" + String(keycode, HEX) + " Modifier: 0x" + String(modifier, HEX);
        Serial.println(sendInfo);
    } else {
        Serial.println("BLE Keyboard not connected, discarding input.");
    }
}

void sendConsumerBLE(uint16_t consumerKeycode) {
    if (connected) {
        uint8_t report[2] = {0};
        report[0] = consumerKeycode & 0xFF;
        report[1] = (consumerKeycode >> 8) & 0xFF;

        consumerInput->setValue(report, sizeof(report));
        consumerInput->notify();
        delay(100);
        report[0] = 0;
        report[1] = 0;
        consumerInput->setValue(report, sizeof(report));
        consumerInput->notify();
        delay(100);

        Serial.printf("Sent Consumer Keycode: 0x%04X\n", consumerKeycode);
    } else {
        Serial.println("Consumer control not connected, discarding input.");
    }
}

void setup() {
    Serial.begin(115200);
    initBLE("BLE IR Test-Router");

    WiFi.begin("your_SSID", "your_PASSWORD");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    server.on("/", handleRoot);
    server.on("/sendKey", handleSendKey);
    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();
}

void handleRoot() {
    server.send(200, "text/plain", "Hello, this is ESP32 BLE server");
}

void handleNotFound() {
    server.send(404, "text/plain", "Not found");
}

void handleSendKey() {
    if (server.hasArg("modifier") && server.hasArg("keycode")) {
        uint8_t modifier = server.arg("modifier").toInt();
        uint8_t keycode = server.arg("keycode").toInt();
        sendBLE(modifier, keycode);
        server.send(200, "text/plain", "Key sent");
    } else if (server.hasArg("consumerKeycode")) {
        uint16_t consumerKeycode = server.arg("consumerKeycode").toInt();
        sendConsumerBLE(consumerKeycode);
        server.send(200, "text/plain", "Consumer key sent");
    } else {
        server.send(400, "text/plain", "Invalid request");
    }
}
