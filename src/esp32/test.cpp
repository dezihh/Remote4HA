#include <Arduino.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include "webpage.h"
#include <BleKeyboard.h>
#include <Preferences.h> // Für das Speichern und Laden von Daten
#include <nvs_flash.h>

// #include <HIDTypes.h>
// #include <HIDKeyboardTypes.h>
// #include <WebServer.h>
// #include <ESPmDNS.h>
// #define USE_USB
#ifdef USE_USB
#include "EspUsbHost.h"
#endif
#define MAX_ROUTES 100
#include "reportMap.h"
#include <IRremote.hpp>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// USB Definitions
bool usb_connected = false;
unsigned long keyPressStartTime = 0;
bool keyPressed = false;
int longKey = 500;
bool defaultToAPI = false;

// BLE HID service variables
// BLE HID service variables
BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;
BleKeyboard bleKeyboard("ESP32 BLE Keyboard", "ESP32 BLE Keyboard", 100);
bool is_ble_connected = false;



void sendBle(uint8_t modifier, uint8_t keycode, bool isRepeat);
void sendIR(uint32_t address, uint8_t command, bool repeats, decode_type_t protocol);
void sendHttpToAPI(String provider, String data);
void handleSendBle(AsyncWebServerRequest *request);
void handleServerCommand(AsyncWebServerRequest *request);
void handleSaveRequest(AsyncWebServerRequest *request);
void handleLoadRequest(AsyncWebServerRequest *request);

// IR: Pin Definitionen
const int RECV_PIN = 15; // (15)(2) IR Empfänger Pin
const int SEND_PIN = 4;  // (4)(3)IR Sender Pin
#define NO_LED_FEEDBACK_CODE

Preferences preferences;
bool sendDataDefaultToApi = false;

// Task-Handles
TaskHandle_t irReadTaskHandle = NULL;

// Common variables
const char *serverURL = "http://192.168.10.3:8125/api/webhook/myid";

// Webserver auf Port 80
AsyncWebServer server(80);
// Event-Handler für SSE
AsyncEventSource events("/events");
WiFiClientSecure client; // Persistent HTTP Client

struct IRrcv
{ // Eingang IR
    decode_type_t protocol;
    uint8_t code;
    uint32_t address;
    bool isRepeat;
};

struct USBRecv
{ // Eingang USB
    uint8_t modifier;
    uint8_t command;
    bool keyLong;
};

void wifiTask() {
    WiFiManager wm;
    if (!wm.autoConnect("ESP32_AutoConnect"))
    {
        Serial.println("Failed to connect to WiFi. Restarting...");
        delay(3000);
        ESP.restart();
    }
    else
    {
        Serial.println("Connected to WiFi");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        int32_t rssi = WiFi.RSSI();
        Serial.print("Signalstärke (RSSI): ");
        Serial.print(rssi);
        Serial.println(" dBm");
    }
}

// Forward Data to Server
void sendHttpToAPI(String provider, String data) {
    static HTTPClient http;
    if (!http.connected()) {
        http.begin(client, serverURL);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    }

    String payload = "provider=" + provider + "&data=" + data;
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
        Serial.printf("Server Response: %d\n", httpResponseCode);
    } else {
        Serial.printf("Error in sending request: %s\n", http.errorToString(httpResponseCode).c_str());
    }
}


void webserver() {
    // SSE-Endpunkt registrieren
    server.addHandler(&events);

    // HTTP-Endpunkt für die Hauptseite
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", htmlPage);
    });

    // Starte den Server
    server.begin();
}

void IRRecvTask(void *pvParameters) {
    IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);
    Serial.print(F("Ready to receive IR signals of protocols: "));
    String protocol = []{ String output; class StringPrinter : public Print 
        { public: String &output; StringPrinter(String &output) : 
        output(output) {} size_t write(uint8_t c) { output += (char)c; return 1; } }; 
        StringPrinter printer(output); printActiveIRProtocols(&printer); return output; }();
    String protocols = "Activated IR Protocols: " + protocol;
    events.send(protocol.c_str());
    // ANCHOR ws.textAll(protocol.c_str());
    Serial.println(protocol);

    IrSender.begin(SEND_PIN, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);
    IrReceiver.enableIRIn(); // Start the receiver

    while (true) {
        if (IrReceiver.decode()) {
            IRrcv data;
            data.protocol = IrReceiver.decodedIRData.protocol;
            data.address = IrReceiver.decodedIRData.address;
            data.code = IrReceiver.decodedIRData.command;
            data.isRepeat = (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) != 0;
            String protName = getProtocolString(data.protocol);
            // ANCHOR route("IR", data);
            IrReceiver.resume();
            String rcvInfo = "Received IR - Protocol: " +
                             protName + "(" + String(data.protocol) + ")" +
                             " Address: 0x" + String(data.address, HEX) +
                             " Code: 0x" + String(data.code, HEX) +
                             " Repeat: " + String(data.isRepeat ? "true" : "false");
            vTaskDelay(pdMS_TO_TICKS(100)); // Ensure events object is ready
            events.send(rcvInfo.c_str());
            Serial.println(rcvInfo);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Kurze Pause, um anderen Tasks Zeit zu geben
    }
}
// Sendet die IR Daten
void sendIR(uint32_t address, uint8_t command, bool repeats, decode_type_t protocol)
{
    Serial.println(String(protocol));
    String protocolUpper = getProtocolString(protocol);
    std::transform(protocolUpper.begin(), protocolUpper.end(), protocolUpper.begin(), ::toupper);

    IrReceiver.stop();

    uint32_t maskedAddress;

    if (protocolUpper == "KASEIKYO_DENON")
    {
        maskedAddress = address & 0xFFF;
        IrSender.sendKaseikyo_Denon(maskedAddress, command, repeats);
    }
    else if (protocolUpper == "DENON")
    {
        maskedAddress = address & 0xFF;
        IrSender.sendDenon(maskedAddress, command, repeats);
    }
    else if (protocolUpper == "SONY")
    {
        maskedAddress = address & 0xFF;
        //IrSender.sendSony(maskedAddress, command, repeats);
        String diagStr = "Sony: " + String(maskedAddress, HEX) + " " + String(command, HEX) + " " + String(repeats);
        Serial.println(diagStr);
    }
    else if (protocolUpper == "LG")
    {
        maskedAddress = address & 0xFF;
        IrSender.sendLG(maskedAddress, command, repeats);
    }
    else if (protocolUpper == "NEC2")
    {
        maskedAddress = address & 0xFF;
        IrSender.sendNEC2(maskedAddress, command, repeats);
    }
    else if (protocolUpper == "NEC")
    {
        maskedAddress = address & 0xFF;
        IrSender.sendNEC(maskedAddress, command, repeats);
    }
    else if (protocolUpper == "PANASONIC")
    {
        maskedAddress = address;
        IrSender.sendPanasonic(maskedAddress, command, repeats);
    }
    else if (protocolUpper == "SAMSUNG")
    {
        maskedAddress = address & 0xFF;
        IrSender.sendSamsung(maskedAddress, command, repeats);
    }
    else if (protocolUpper == "WHYNTER")
    {
        maskedAddress = address & 0xFF;
        IrSender.sendWhynter(maskedAddress, command);
    }
    else if (protocolUpper == "RC5")
    {
        maskedAddress = address & 0xFF;
        IrSender.sendRC5(maskedAddress, command, repeats);
    }
    else if (protocolUpper == "RC6")
    {
        maskedAddress = address & 0xFF;
        IrSender.sendRC6(maskedAddress, command, repeats);
    }
    else if (protocolUpper == "MAGIQUEST")
    {
        maskedAddress = address & 0xFF;
        IrSender.sendMagiQuest(maskedAddress, command);
    }
    else
    {
        Serial.print(F("Unknown IR protocol requested: "));
        Serial.println(protocolUpper);
    }

    String sendInfo = "Sent IR Command Protocol: " + protocolUpper + "(" + String(protocol) + ")" +
                      " Address: 0x" + String(maskedAddress, HEX) +
                      " Command: 0x" + String(command, HEX) +
                      " Repeats: " + String(repeats ? "true" : "false");
    events.send(sendInfo.c_str());

    delay(40); // Wartezeit nach dem Senden
    IrReceiver.start();
    delay(60); // Wartezeit nach dem Senden
}

// HTTP Endpunkt Handler für IR
void handleSendIr(AsyncWebServerRequest *request)
//curl -X POST -d "protocol=8&address=0x0&command=0x44&repeats=0" http://192.168.10.167/sendIR
{
    if (request->hasParam("address", true) &&
        request->hasParam("command", true) &&
        request->hasParam("protocol", true) &&
        request->hasParam("repeats", true))
    {
        uint32_t address = strtoul(request->getParam("address", true)->value().c_str(), nullptr, 16);
        uint8_t command = strtoul(request->getParam("command", true)->value().c_str(), nullptr, 16);
        bool repeats = request->getParam("repeats", true)->value() == "true";
        decode_type_t protocol = static_cast<decode_type_t>(strtoul(request->getParam("protocol", true)->value().c_str(), nullptr, 16));
        sendIR(address, command, repeats, protocol);
        request->send(200, "text/plain", "IR Command Sent");
    }
    else
    {
        request->send(400, "text/plain", "Missing parameters");
    }
}

void setup() {
    Serial.begin(115200);
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    wifiTask();
    webserver();
    // SSE-Endpunkt registrieren
    server.addHandler(&events);
      // HTTP-Endpunkt für die Hauptseite
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", htmlPage);    });
    server.on("/sendIR", HTTP_POST, [](AsyncWebServerRequest *request)
            { handleSendIr(request); });
    
        // Erstelle eine neue FreeRTOS-Task für die IR-Empfänger-Logik
    BaseType_t result = xTaskCreatePinnedToCore(
            IRRecvTask,        // Task-Funktion
            "IRRecvTask",      // Name der Task
            2048,              // Stapelgröße in Wörtern
            NULL,              // Parameter für die Task
            1,                 // Priorität der Task
            NULL,              // Task-Handle
            1                  // Kern, auf dem die Task ausgeführt werden soll
        );

        if (result != pdPASS) {
            Serial.println("Failed to create IRRecvTask");
        }
}

/* // Simulierte Log-Daten
unsigned long previousMillis = 0;
const long interval = 1000; // Intervall (1 Sekunde) */

void loop() {
    
/*     static unsigned long previousMillis = 0;
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        String message = "Log-Eintrag: Zeit = " + String(millis() / 1000) + " Sekunden";
        if (events.count() > 0) { // Nur senden, wenn Clients verbunden sind
            events.send(message.c_str(), "log", millis());
            Serial.println("Gesendet: " + message);
        } else {
            Serial.println("Keine verbundenen Clients.");
        }
    } */
}
