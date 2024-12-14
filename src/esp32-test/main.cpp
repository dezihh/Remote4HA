#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <BLEServer.h>
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
BLECharacteristic* consumerInput;
bool connected = false;

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
    //pAdvertising->setAppearance(0x03C0);  // Generic HID
    pAdvertising->setAppearance(0x0341);  // Keyboard
    
    pAdvertising->addServiceUUID(hid->hidService()->getUUID());
        // Optimierungen
    pAdvertising->setMinInterval(0x20); // 32 * 0.625ms = 20ms
    pAdvertising->setMaxInterval(0x40); // 64 * 0.625ms = 40ms
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // 7.5ms
    pAdvertising->setMaxPreferred(0x12); // 15ms
    pAdvertising->start();
    Serial.println("Advertising started!");
}

void sendKey1(uint8_t modifier, uint8_t keycode) {
    if (connected) {
        uint8_t report[8] = {0};
        report[0] = 0x01;  // Report ID (1) for Keyboard
        report[1] = modifier;  // Modifier (e.g., Ctrl, Shift)
        report[2] = keycode;   // Keycode
        report[3] = 0x00;        // Reserved
        report[4] = 0x00;        // Keine weitere Taste
        report[5] = 0x00;        // Keine weitere Taste
        report[6] = 0x00;        // Keine weitere Taste
        report[7] = 0x00;        // Keine weitere Taste
        Serial.printf("Sending HID Report: Modifier: 0x%02X, Keycode: 0x%02X\n", modifier, keycode);

        input->setValue(report, sizeof(report));
        input->notify();
        delay(250);
        // Reset the report
        memset(report, 0, sizeof(report));
        report[0] = 0x01;  // Report ID (1) for Keyboard
        input->setValue(report, sizeof(report));
        input->notify();
        delay(100);
        String sendInfo = "Sent BLE Keycode: 0x" + String(keycode, HEX) + " Modifier: 0x" + String(modifier, HEX);
        Serial.println(sendInfo);
    } else {
        Serial.println("BLE Keyboard not connected, discarding input.");
    }
}

void sendBLE(uint8_t modifier, uint16_t keycode, bool longPress=false) {
    bool isConsumer = false;
    if (modifier == 0x81){
            isConsumer = true;
    }
    uint16_t duration = longPress ? 555 : 100; // Dauer des Tastendrucks
    if (connected) {
        if (isConsumer) {
            // Consumer-Control-Bericht senden
            uint8_t report[2] = {0};
            report[0] = keycode & 0xFF;
            report[1] = (keycode >> 8) & 0xFF;
            Serial.printf("Sending Consumer HID Report: Keycode: 0x%04X\n", keycode);

            consumerInput->setValue(report, sizeof(report));
            consumerInput->notify();
            delay(100);
            memset(report, 0, sizeof(report));
            consumerInput->setValue(report, sizeof(report));
            consumerInput->notify();
            delay(100);
            String sendInfo = "Sent BLE Consumer Keycode: 0x" + String(keycode, HEX);
            Serial.println(sendInfo);
        } else {
            // Tastatur-Bericht senden
            uint8_t report[8] = {0};
            report[0] = modifier;  // Modifier (Shift, Ctrl, Alt etc.)
            report[2] = keycode;   // Keycode

            input->setValue(report, sizeof(report)); // Key-Press senden
            input->notify();
            //delay(100); --> is unten definiert

            // Key-Up Event
            memset(report, 0, sizeof(report)); // Bericht zurücksetzen
            input->setValue(report, sizeof(report));
            input->notify();
            delay(100);

        }
        delay(100);
        String sendInfo = "Sent BLE Keycode: 0x" + String(keycode, HEX) + 
                          " Modifier: 0x" + String(modifier, HEX) +
                          " Long: " + String(longPress ? "true" : "false") +
                          " Consumer: " + String(isConsumer ? "true" : "false");
        //events.send(sendInfo.c_str());
        Serial.println(sendInfo);
    } else {
        Serial.println("BLE Keyboard not connected, discarding input."); 
        //events.send("BLE Keyboard not connected, discarding input.");
    }
}

void sendBLEok(uint8_t modifier, uint8_t keycode) {
    if (connected) {
        uint8_t report[8] = {0};
        report[0] = modifier;  // Modifier (Shift, Ctrl, Alt etc.)
        report[2] = keycode;   // Keycode

        input->setValue(report, sizeof(report)); // Key-Press senden
        input->notify();
        delay(100);

        // Key-Up Event
        memset(report, 0, sizeof(report)); // Bericht zurücksetzen
        input->setValue(report, sizeof(report));
        input->notify();
        delay(100);
    } else {
        Serial.println("Keine Verbindung, Keycode wird nicht gesendet.");
    }
}

void sendConsumerBLE(uint16_t consumerKeycode) {
    if (connected) {
        uint8_t report[2] = {0};
        report[0] = consumerKeycode & 0xFF;
        report[1] = (consumerKeycode >> 8) & 0xFF;
        Serial.printf("Sending Consumer HID Report: Keycode: 0x%04X\n", consumerKeycode);

        consumerInput->setValue(report, sizeof(report));
        consumerInput->notify();
        delay(100);
        memset(report, 0, sizeof(report));
        consumerInput->setValue(report, sizeof(report));
        consumerInput->notify();
        delay(100);
        String sendInfo = "Sent BLE Consumer Keycode: 0x" + String(consumerKeycode, HEX);
        Serial.println(sendInfo);
    } else {
        Serial.println("BLE Consumer Control not connected, discarding input.");
    }
}

void sendText(String text) {
    for (int i = 0; i < text.length(); i++) {
        char c = text.charAt(i);
        uint8_t modifier = 0;
        uint8_t keycode = 0;

        // Zeichen in HID Keycode übersetzen
        if (c >= 'A' && c <= 'Z') {
            modifier = 0x02; // Shift gedrückt
            keycode = c - 'A' + 0x04; // 'A' = 0x04, 'B' = 0x05 ...
        } else if (c >= 'a' && c <= 'z') {
            keycode = c - 'a' + 0x04; // 'a' = 0x04, 'b' = 0x05 ...
        } else if (c == ' ') {
            keycode = 0x2C; // Leerzeichen
        } else {
            Serial.printf("Zeichen '%c' wird nicht unterstützt.\n", c);
            continue; // Überspringen
        }

        // Key senden
        sendBLE(modifier, keycode,false);
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.begin("MyAp0816", "Wurst1234.");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    initBLE("ESP32 HID");

    server.on("/", handleRoot);
    server.on("/sendKey", handleSendKey);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP server started");
    BLEDevice::startAdvertising(); // Werbung nach Reboot starten

}

void loop() {
    server.handleClient();
}

void handleRoot() {
    server.send(200, "text/plain", "Hello, world!");
}

void handleNotFound() {
    server.send(404, "text/plain", "Not found");
}

void handleSendKey() {
    if (server.hasArg("modifier") && server.hasArg("keycode")) {
        int modifierInt = strtol(server.arg("modifier").c_str(), NULL, 16);
        int keycodeInt = strtol(server.arg("keycode").c_str(), NULL, 16);
        Serial.printf("Received modifier: 0x%02X, keycode: 0x%02X\n", modifierInt, keycodeInt);
        if (modifierInt >= 0 && modifierInt <= 255 && keycodeInt >= 0 && keycodeInt <= 255) {
            sendBLE(modifierInt, keycodeInt,false);
            server.send(200, "text/plain", "Key sent");
        } else {
            server.send(400, "text/plain", "Invalid keycode or modifier");
        }
    } else if (server.hasArg("consumerKeycode")) {
        int consumerKeycodeInt = strtol(server.arg("consumerKeycode").c_str(), NULL, 16);
        Serial.printf("Received consumerKeycode: 0x%04X\n", consumerKeycodeInt);
        if (consumerKeycodeInt >= 0 && consumerKeycodeInt <= 65535) {
            sendConsumerBLE(consumerKeycodeInt);
            server.send(200, "text/plain", "Consumer key sent");
        } else {
            server.send(400, "text/plain", "Invalid consumer keycode");
        }
    } else if (server.hasArg("text")) {
        String textString = server.arg("text");
        Serial.printf("Received Text: %s\n", textString.c_str());

        sendText(textString);
        server.send(200, "text/plain", "Text sent");
    } else {
        server.send(400, "text/plain", "Missing keycode or modifier");
    }
}