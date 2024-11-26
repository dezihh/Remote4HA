#include <Arduino.h>
#include <WiFiManager.h>
#include <AsyncTCP.h>
#include "webpage.h"
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <BleKeyboard.h>
#include <ArduinoJson.h>
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

// Common variables
const char *serverURL = "http://192.168.10.3:8125/api/webhook/myid";
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Routingstruktur mit Zielwerten für Protokoll, Adresse, Command und Modifier

struct Route
{
    String source;          // IR, IF
    decode_type_t protocol; // IR Prot
    uint8_t code;           // IR Command
    uint32_t address;       // IR Address/
    int isRepeat;           // IR Repeat 0, 1 oder 2
    uint8_t modifier;       // USB Keycode
    uint8_t command;        // USB Modifier
    int keyLong;            // USB 0, 1 oder 2

    String actionFuncName; // Sendefunction
    decode_type_t oIRprot; // IR Prot
    uint8_t oIRcode;       // IR Code
    uint32_t oIRaddress;   // IR Address
    bool oIRisRepeat;      // IR Repeat 0 oder 1
    uint8_t oBleMod;       // BLE Modifier
    uint8_t oBleCode;      // BLE Command
    bool oRBleRepeat;      // BLE Repeat
};

Route routeTable[MAX_ROUTES];
int routeCount = 0;

// Statische Routingtabelle
/* Route routeTable[MAX_ROWS] = {
    //{"src", IRprot, IRCod, IRAdr, IRRep, USmod, USkey, USLng, action, nullptr, nullptr, UNKNOWN, 0xd, 0x0000, 0, KEY_LEFT_SHIFT, "A", 0},
    {"IR", NEC, 0xe, 0x0000, 2, 0x0, 0, 0, "sendIR", SONY, 0xe, 0x0001, 2, 0, 0, 0},
    {"IR", NEC, 0x12, 0x0000, 2, 0x0, 0, 0, "sendBLE", UNKNOWN, 0x0, 0x0000, 0, KEY_LEFT_SHIFT, 0x4, 0},
    {"IR", NEC, 0xa, 0x0000, 2, 0x0, 0, 0, "sendHttpToAPI", UNKNOWN, 0x0, 0x0000, 0, 0, 0, 0}
}; */

bool sendToApi = false; // Boolescher Wert für die Radiobox

// out
// BLE: int8_t modifier, uint8_t keycode, bool isRepeat
// API: String source, String data
//  IR: uint32_t address, uint8_t command, bool repeats, const String& protocol

/////////////////
void saveRoutesToNVS()
{
    Preferences preferences;
    if (!preferences.begin("routeData", false))
    {
        Serial.println("Failed to initialize NVS for writing");
        return;
    }

    preferences.putInt("routeCount", routeCount);
    for (int i = 0; i < routeCount; i++)
    {
        String key = "route" + String(i);
        preferences.putBytes(key.c_str(), &routeTable[i], sizeof(Route));
    }

    preferences.end();
    Serial.println("Routes saved to NVS.");
}

// Function to load routes from NVS
void loadRoutesFromNVS()
{
    Preferences preferences;
    if (!preferences.begin("routeData", true))
    {
        Serial.println("Failed to initialize NVS for reading");
        return;
    }

    routeCount = preferences.getInt("routeCount", 0);
    if (routeCount > MAX_ROUTES)
    {
        Serial.println("Route count exceeds maximum capacity");
        preferences.end();
        return;
    }

    for (int i = 0; i < routeCount; i++)
    {
        String key = "route" + String(i);
        if (preferences.getBytesLength(key.c_str()) == sizeof(Route))
        {
            preferences.getBytes(key.c_str(), &routeTable[i], sizeof(Route));
        }
        else
        {
            Serial.println("Route data size mismatch for key: " + key);
        }
    }

    preferences.end();
    Serial.println("Routes loaded from NVS.");
}

// const int routeCount = sizeof(routeTable) / sizeof(routeTable[0]);

void route(String source, IRrcv irData)
{
    for (int i = 0; i < sizeof(routeTable) / sizeof(Route); i++)
    {
        Route &route = routeTable[i];

        // Überprüfung für IR-Daten
        if (source == "IR" &&
            irData.protocol == route.protocol &&
            irData.code == route.code &&
            irData.address == route.address)
        {
            // Prüfbedingung für Repeat
            bool repeatCondition = (route.isRepeat == 2) ||
                                   (route.isRepeat == 1 && irData.isRepeat) ||
                                   (route.isRepeat == 0 && !irData.isRepeat);

            if (repeatCondition)
            {
                // Aktion ausführen basierend auf actionFuncName
                if (route.actionFuncName == "sendIR")
                {
                    bool finalRepeat = (route.isRepeat == 2) ? irData.isRepeat : route.oIRisRepeat;
                    // ANCHOR WS Log
                    ws.textAll("IR command send");
                    Serial.println("sendIR: Matched, Resend IR Data");
                    sendIR(route.oIRaddress, route.oIRcode, finalRepeat, route.oIRprot);
                }
                else if (route.actionFuncName == "sendBLE")
                {
                    bool finalRepeat = (route.isRepeat == 2) ? irData.isRepeat : route.oRBleRepeat;
                    Serial.println("sendIR: Matched, Sent BLE Data");
                    sendBle(route.oBleMod, route.oBleCode, finalRepeat);
                }
                else if (route.actionFuncName == "sendHttpToAPI")
                {
                    String data = "protocol=" + String(irData.protocol) +
                                  "&code=" + String(irData.code) +
                                  "&address=" + String(irData.address) +
                                  "&isRepeat=" + String(irData.isRepeat);
                    Serial.println("sendIR: Matched, Sent API Data");
                    sendHttpToAPI(source, data);
                }
                return; // Aktion ausgeführt, keine weitere Prüfung nötig
            }
        }
    }

    // Kein Match gefunden, Standardaktion
    String data = "protocol=" + String(irData.protocol) +
                  "&code=" + String(irData.code) +
                  "&address=" + String(irData.address) +
                  "&isRepeat=" + String(irData.isRepeat);
    sendHttpToAPI(source, data);
}

void route(String source, USBRecv usbData)
{
    for (int i = 0; i < sizeof(routeTable) / sizeof(Route); i++)
    {
        Route &route = routeTable[i];

        // Überprüfung für USB-Daten
        if (source == "IF" &&
            usbData.modifier == route.modifier &&
            usbData.command == route.command)
        {

            // Prüfbedingung für keyLong
            bool keyLongCondition = (route.keyLong == 2) ||
                                    (route.keyLong == 1 && usbData.keyLong) ||
                                    (route.keyLong == 0 && !usbData.keyLong);

            if (keyLongCondition)
            {
                // Aktion ausführen basierend auf actionFuncName
                if (route.actionFuncName == "sendIR")
                {
                    bool finalRepeat = (route.keyLong == 2) ? usbData.keyLong : route.oIRisRepeat;
                    Serial.println("USB: Matched, Sent IR Data");
                    sendIR(route.oIRaddress, route.oIRcode, finalRepeat, route.oIRprot);
                }
                else if (route.actionFuncName == "sendBLE")
                {
                    bool finalRepeat = (route.keyLong == 2) ? usbData.keyLong : route.oRBleRepeat;
                    Serial.println("USB: Matched, Sent BLE Data");
                    sendBle(route.oBleMod, route.oBleCode, finalRepeat);
                }
                else if (route.actionFuncName == "sendHttpToAPI")
                {
                    String data = "modifier=" + String(usbData.modifier) +
                                  "&command=" + String(usbData.command) +
                                  "&keyLong=" + String(usbData.keyLong);
                    Serial.println("USB: Matched, Sent API Data");
                    sendHttpToAPI(source, data);
                }
                return; // Aktion ausgeführt, keine weitere Prüfung nötig
            }
        }
    }

    // Kein Match gefunden, Standardaktion
    String data = "modifier=" + String(usbData.modifier) +
                  "&command=" + String(usbData.command) +
                  "&keyLong=" + String(usbData.keyLong);
    Serial.println("None: No Match found");
    sendHttpToAPI(source, data);
}

void handleGetdefaultRouteFields(AsyncWebServerRequest *request)
{
    String response = defaultToAPI ? "true" : "false";
    request->send(200, "text/plain", response);
    Serial.println("In handleGetdefaultRoute");
}

// POST-Endpunkt: Setzt den neuen Zustand
void handlePostdefaultRouteFields(AsyncWebServerRequest *request)
{
    Serial.println("In PostdefaultRoute");
    if (request->hasParam("value", true))
    { // Prüfen, ob der POST-Parameter existiert
        String value = request->getParam("value", true)->value();
        if (value == "true")
        {
            defaultToAPI = true;
            request->send(200, "text/plain", "OK");
        }
        else if (value == "false")
        {
            defaultToAPI = false;
            request->send(200, "text/plain", "OK");
        }
        else
        {
            request->send(400, "text/plain", "Invalid value");
        }
    }
    else
    {
        request->send(400, "text/plain", "Missing 'value' parameter");
    }
}

// Funktion zum Senden einer HTTP-Anfrage an die API
// void sendHttpToAPI(String provider, String address, String code) {
void sendHttpToAPI(String provider, String data)
{
    HTTPClient http;
    http.begin(serverURL);

    String payload = "provider=" + provider + "&data=" + data;
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0)
    {
        Serial.printf("Server Response: %d\n", httpResponseCode);
    }
    else
    {
        Serial.printf("Error in sending request: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    http.end();
}

// ANCHOR USB beginns
// 19(D-) + 20(D+) (USB not UART)
void setupUSBHost()
{
#ifdef USE_USB
    usbHost.begin();
    Serial.println("USB Keyboard initialized");
#endif
}
void readUSB(uint8_t ascii, uint8_t keycode, uint8_t modifier)
{
    usb_connected = true;

    if (!keyPressed)
    {                                 // Taste wurde gerade gedrückt
        keyPressStartTime = millis(); // Speichere den Zeitpunkt des Tastendrucks
        keyPressed = true;            // Setze den Status auf "Taste gedrückt"
    }
    else
    { // Taste wurde losgelassen
        // Berechne die gedrückte Zeit
        unsigned long duration = millis() - keyPressStartTime; // Zeitdauer in Millisekunden
        USBRecv data;
        data.command = (keycode, HEX);
        data.modifier = (modifier, HEX);
        data.keyLong = (duration > longKey); // Setze isRepeat auf true oder false
        String keyEntry = String("Key: ") + String(keycode, HEX) +
                          " (ASCII:" + String((char)ascii) +
                          ") Modifier: " + String(modifier, HEX) +
                          " Duration: " + String(duration) + " ms\n";

        // serialOutput = keyEntry + serialOutput;  // Füge die Ausgabe zur seriellen Ausgabe hinzu
        Serial.printf("%s", keyEntry.c_str());

        // Reset der Statusvariablen
        keyPressed = false; // Setze den Status auf "Taste nicht gedrückt"

        // Routenaufruf mit den erstellten Daten
        // route("IF", data);  // Rufe die Routingfunktion auf
    }
}
// ANCHOR IR beginns
// IR Setup mit erweiterten Optionen
void setupIRRecv()
{
    IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);
    Serial.print(F("Ready to receive IR signals of protocols: "));
        // printActiveIRProtocols(&Serial);
    String protocol = []{ String output; class StringPrinter : public Print 
        { public: String &output; StringPrinter(String &output) : 
        output(output) {} size_t write(uint8_t c) { output += (char)c; return 1; } }; 
        StringPrinter printer(output); printActiveIRProtocols(&printer); return output; }();
    String protocols = "Aktivated IR Protocols: " + protocol;
    ws.textAll(protocol.c_str());
    Serial.println(protocol);

    IrSender.begin(SEND_PIN, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);
    // Zusätzliche IR-Empfänger Konfiguration
    IrReceiver.enableIRIn(); // Start the receiver
}

// Sendet die IR Daten
void sendIR(uint32_t address, uint8_t command, bool repeats, decode_type_t protocol)
{

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
        IrSender.sendSony(maskedAddress, command, repeats);
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
    ws.textAll(sendInfo.c_str());

    delay(40); // Wartezeit nach dem Senden
    IrReceiver.start();
    delay(60); // Wartezeit nach dem Senden
}

// HTTP Endpunkt Handler für IR
void handleSendIr(AsyncWebServerRequest *request)
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

// Datenverarbeitung IR-Daten
void readIR()
{
    if (IrReceiver.decode())
    {
        IRrcv data;
        data.protocol = IrReceiver.decodedIRData.protocol;
        data.address = IrReceiver.decodedIRData.address;
        data.code = IrReceiver.decodedIRData.command;
        data.isRepeat = (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) != 0;
        String protName = getProtocolString(data.protocol);
        route("IR", data);
        IrReceiver.resume();
        String rcvInfo = "Received IR - Protocol: " +
                         protName + "(" + String(data.protocol) + ")" +
                         " Address: 0x" + String(data.address, HEX) +
                         " Code: 0x" + String(data.code, HEX) +
                         " Repeat: " + String(data.isRepeat ? "true" : "false");
        ws.textAll(rcvInfo.c_str());
    }
}
// ANCHOR WiFi beginns
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    Serial.println("In handleWebSocketMessage");
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
        String message = String((char *)data);
        Serial.println("In der Nachricht WebSocket");
        // Aufteilen der Nachricht in ihre Bestandteile
        int firstComma = message.indexOf(',');
        int secondComma = message.indexOf(',', firstComma + 1);

        if (firstComma > 0 && secondComma > 0)
        {
            decode_type_t protocol = (decode_type_t)message.substring(0, firstComma).toInt();
            uint16_t address = strtoul(message.substring(firstComma + 1, secondComma).c_str(), NULL, 16);
            uint8_t command = strtoul(message.substring(secondComma + 1).c_str(), NULL, 16);

            // IR-Signal senden
            //    sendIR(address, command, 0, protocol);

            // Bestätigung über WebSocket senden
            // String confirmMsg = "IR-Signal gesendet - Adresse: 0x" + String(address, HEX) +
            //                  ", Command: 0x" + String(command, HEX);
            // ws.textAll(confirmMsg);
        }
    }
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}
void setupWiFi()
{
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
// ANCHOR Webserver begins
void setupWebServer()
{
    server.addHandler(&ws);

    // HTTP-POST-Endpunkt hinzufügen
    server.on("/sendCommand", HTTP_POST, handleServerCommand);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  request->send_P(200, "text/html", htmlPage); // send_P für PROGMEM
              });

    /////////
    // Routen definieren
    server.on("/sendBLE", HTTP_POST, [](AsyncWebServerRequest *request)
              { handleSendBle(request); });
    server.on("/sendIR", HTTP_POST, [](AsyncWebServerRequest *request)
              { handleSendIr(request); });
    server.on("/defaultRouteFields", HTTP_GET, handleGetdefaultRouteFields);
    server.on("/defaultRouteFields", HTTP_POST, handlePostdefaultRouteFields);
    // server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request)
              { handleSaveRequest(request); });
    server.on("/loadData", HTTP_POST, [](AsyncWebServerRequest *request)
              { handleLoadRequest(request); });

    // server.on("/loadData", HTTP_GET, [](AsyncWebServerRequest *request) {

    /*       // Endpunkt zum Lesen der Daten
        server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        loadDataFromFlash();  // Daten laden
        request->send(200, "text/plain", "Data loaded successfully");
        }); */

    // Endpunkt zum Speichern der Daten
    /*     server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
            saveDataToFlash();
            request->send(200, "text/plain", "Data saved successfully");
        }); */

    server.begin();
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}
// ANCHOR Bluetooth beginns
void handleServerCommand(AsyncWebServerRequest *request)
{
    if (!request->hasParam("function", true))
    {
        request->send(400, "text/plain", "Missing 'function' parameter");
        return;
    }

    String functionStr = request->getParam("function", true)->value();

    if (functionStr == "IR")
    {
        if (!(request->hasParam("protocol", true) &&
              request->hasParam("address", true) &&
              request->hasParam("code", true) &&
              request->hasParam("repeat", true)))
        {
            request->send(400, "text/plain", "Missing required parameters for IR function");
            return;
        }

        decode_type_t protocol = static_cast<decode_type_t>(strtoul(request->getParam("protocol", true)->value().c_str(), nullptr, 16));
        String addrStr = request->getParam("address", true)->value();
        String codeStr = request->getParam("code", true)->value();
        String repeatStr = request->getParam("repeat", true)->value();

        char *endPtr;
        uint32_t address = strtoul(addrStr.c_str(), &endPtr, 16);
        if (*endPtr != '\0')
        {
            request->send(400, "text/plain", "Invalid address format");
            return;
        }

        uint8_t code = static_cast<uint8_t>(strtoul(codeStr.c_str(), &endPtr, 16));
        if (*endPtr != '\0')
        {
            request->send(400, "text/plain", "Invalid code format");
            return;
        }

        bool repeatBool = (repeatStr == "true");

        sendIR(address, code, repeatBool, protocol);
        request->send(200, "text/plain", "IR command processed successfully");
    }
    else if (functionStr == "BLE")
    {
        // Ähnliche Struktur für BLE
    }
    else
    {
        request->send(400, "text/plain", "Unknown function");
    }
}

// Endpunkt für BLE
void handleSendBle(AsyncWebServerRequest *request)
{
    if (request->hasParam("modifier", true) &&
        request->hasParam("keycode", true) &&
        request->hasParam("isRepeat", true))
    {
        uint8_t modifier = strtoul(request->getParam("modifier", true)->value().c_str(), nullptr, 16);
        uint8_t keycode = strtoul(request->getParam("keycode", true)->value().c_str(), nullptr, 16);
        bool isRepeat = request->getParam("isRepeat", true)->value() == "true";
        Serial.println();
        sendBle(modifier, keycode, isRepeat);
        request->send(200, "text/plain", "BLE Command Sent");
    }
    else
    {
        request->send(400, "text/plain", "Missing parameters");
    }
}

// Funktion zum Senden eines BLE-Reports
void sendBle(uint8_t modifier, uint8_t keycode, bool isRepeat) {
    if (bleKeyboard.isConnected()) {
        if (isRepeat) {
            if (modifier != 0) {
                bleKeyboard.press(modifier);
                bleKeyboard.press(keycode);
                delay(longKey);
                bleKeyboard.release(keycode);
                bleKeyboard.release(modifier);
            } else {
                bleKeyboard.press(keycode );
                delay(longKey);
                bleKeyboard.release(keycode );
            }
        } else {
            if (modifier != 0) {
                bleKeyboard.press(modifier);
                bleKeyboard.press(keycode );
                delay(100);
                bleKeyboard.release(keycode );
                bleKeyboard.release(modifier);
            } else {
                bleKeyboard.releaseAll();
                bleKeyboard.press('c' );
                Serial.println("keycode c");
                delay(100);
                bleKeyboard.release(keycode );
            }
        }

        String sendInfo = "Sent BLE Keycode: 0x" + String(keycode, HEX) +
                          " Modifier: 0x" + String(modifier, HEX) +
                          " Repeats: " + String(isRepeat ? "true" : "false");

        Serial.println(sendInfo);
    }
    else
    {
        Serial.println("BLE not connected, discarding key input.");
    }
}

//ANCHOR Save Routedata
void handleSaveRequest(AsyncWebServerRequest *request) {
    if (!request->hasParam("data", true)) {
        request->send(400, "text/plain", "No data provided");
        return;
    }

    // Retrieve the data
    String rawData = request->getParam("data", true)->value();
    Serial.println("Received data:");
    Serial.println(rawData);

    // Reset route count before processing new data
    routeCount = 0;
    int startIndex = 0;
    int endIndex = rawData.indexOf("\n");

    // Process each line of input
    while (endIndex != -1 && routeCount < MAX_ROUTES)
    {
        String row = rawData.substring(startIndex, endIndex);

        // Temporary buffers for parsing
        char sourceBuffer[16];
        char actionBuffer[32];

        // Parse the current row
        sscanf(row.c_str(), "%15[^,],%hhu,%hhu,%u,%d,%hhu,%hhu,%d,%31[^,],%hhu,%hhu,%u,%d,%hhu,%hhu,%d",
               sourceBuffer,
               &routeTable[routeCount].protocol,
               &routeTable[routeCount].code,
               &routeTable[routeCount].address,
               &routeTable[routeCount].isRepeat,
               &routeTable[routeCount].modifier,
               &routeTable[routeCount].command,
               &routeTable[routeCount].keyLong,
               actionBuffer,
               &routeTable[routeCount].oIRprot,
               &routeTable[routeCount].oIRcode,
               &routeTable[routeCount].oIRaddress,
               &routeTable[routeCount].oIRisRepeat,
               &routeTable[routeCount].oBleMod,
               &routeTable[routeCount].oBleCode,
               &routeTable[routeCount].oRBleRepeat);

        // Assign parsed values to String fields
        routeTable[routeCount].source = String(sourceBuffer);
        routeTable[routeCount].actionFuncName = String(actionBuffer);

        // Debug output for each parsed route
        Serial.printf("Route %d: %s Protocol: %d Code: %d Action: %s\n",
                      routeCount,
                      routeTable[routeCount].source.c_str(),
                      routeTable[routeCount].protocol,
                      routeTable[routeCount].code,
                      routeTable[routeCount].actionFuncName.c_str());

        routeCount++;
        startIndex = endIndex + 1;
        endIndex = rawData.indexOf("\n", startIndex);
    }

    // Handle the last line if it does not end with \n
    if (routeCount < MAX_ROUTES && startIndex < rawData.length())
    {
        String row = rawData.substring(startIndex);

        // Temporary buffers for parsing
        char sourceBuffer[16];
        char actionBuffer[32];

        // Parse the last row
        sscanf(row.c_str(), "%15[^,],%hhu,%hhu,%u,%d,%hhu,%hhu,%d,%31[^,],%hhu,%hhu,%u,%d,%hhu,%hhu,%d",
               sourceBuffer,
               &routeTable[routeCount].protocol,
               &routeTable[routeCount].code,
               &routeTable[routeCount].address,
               &routeTable[routeCount].isRepeat,
               &routeTable[routeCount].modifier,
               &routeTable[routeCount].command,
               &routeTable[routeCount].keyLong,
               actionBuffer,
               &routeTable[routeCount].oIRprot,
               &routeTable[routeCount].oIRcode,
               &routeTable[routeCount].oIRaddress,
               &routeTable[routeCount].oIRisRepeat,
               &routeTable[routeCount].oBleMod,
               &routeTable[routeCount].oBleCode,
               &routeTable[routeCount].oRBleRepeat);

        // Assign parsed values to String fields
        routeTable[routeCount].source = String(sourceBuffer);
        routeTable[routeCount].actionFuncName = String(actionBuffer);

        // Debug output for the last parsed route
        Serial.printf("Route %d: %s Protocol: %d Code: %d Action: %s\n",
                      routeCount,
                      routeTable[routeCount].source.c_str(),
                      routeTable[routeCount].protocol,
                      routeTable[routeCount].code,
                      routeTable[routeCount].actionFuncName.c_str());

        routeCount++;
    }

    // Save all parsed routes to NVS
    saveRoutesToNVS();

    // Send success response
    request->send(200, "text/plain", "Routes saved successfully");
}

// ANCHOR Save Routedata
void handleLoadRequest(AsyncWebServerRequest *request)
{
    loadRoutesFromNVS(); // Load saved data from NVS
    String responseData;
    for (int i = 0; i < routeCount; i++)
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "%s,%u,%u,%u,%d,%u,%u,%d,%s,%u,%u,%u,%d,%u,%u,%d\n",
                 routeTable[i].source,
                 routeTable[i].protocol,
                 routeTable[i].code,
                 routeTable[i].address,
                 routeTable[i].isRepeat,
                 routeTable[i].modifier,
                 routeTable[i].command,
                 routeTable[i].keyLong,
                 routeTable[i].actionFuncName,
                 routeTable[i].oIRprot,
                 routeTable[i].oIRcode,
                 routeTable[i].oIRaddress,
                 routeTable[i].oIRisRepeat,
                 routeTable[i].oBleMod,
                 routeTable[i].oBleCode,
                 routeTable[i].oRBleRepeat);
        responseData += buffer;
    }

    if (responseData.isEmpty())
    {
        responseData = "No data available.\n";
    }

    request->send(200, "text/plain", responseData);
}

void setup()
{
    Serial.begin(115200);
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    setupWiFi();
    setupWebServer(); // Webserver initialisieren
    setupIRRecv();
    bleKeyboard.begin(); // BLE Keyboard initialisieren
    if (nvs_flash_init() != ESP_OK)
    {
        Serial.println("NVS Flash Init failed");
        return;
    }
    loadRoutesFromNVS();
}

void loop()
{
    readIR();
    // Falls IF-Daten existieren, kann hier `processAndRouteIFData()` eingebaut werden.
}
