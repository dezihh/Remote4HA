#include <Arduino.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include "webpage.h"
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <NimBLECharacteristic.h>
//#include <BleKeyboard.h>
#include <Preferences.h> // Für das Speichern und Laden von Daten
#include <nvs_flash.h>
bool debug = false;

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

WiFiClient client; // Verwenden Sie WiFiClient
HTTPClient http; // Globaler HTTPClient

// USB Definitions
bool usb_connected = false;
unsigned long keyPressStartTime = 0;
bool keyPressed = false;
int longKey = 500;
//bool forward = false;

// BLE HID service variables
// BLE HID service variables
/*BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;
BleKeyboard bleKeyboard("ESP32 Keyboard", "DeZi", 100);*/
//BleKeyboard bleKeyboard;
//BleKeyboard bleKeyboard;
bool is_ble_connected = false;
NimBLEHIDDevice* hid;
NimBLECharacteristic* input;
bool connected = false;
NimBLECharacteristic* consumerInput;

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

class MyCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        connected = true;
        Serial.println("Client connected");
    }

    void onDisconnect(NimBLEServer* pServer) {
        connected = false;
        Serial.println("Client disconnected");
        NimBLEDevice::startAdvertising();
    }
};



void sendBle(uint8_t modifier, uint8_t keycode, bool isRepeat);
void handleSendBle(AsyncWebServerRequest *request);
void sendIR(uint32_t address, uint8_t command, bool repeats, decode_type_t protocol);
void sendHttpToAPI(String provider, String data);
void handleServerCommand(AsyncWebServerRequest *request);
void handleSaveRequest(AsyncWebServerRequest *request);
void saveRoutesToNVS();
void handleLoadRequest(AsyncWebServerRequest *request);
void loadRoutesFromNVS();
void route(String source, IRrcv irData);
void route(String source, USBRecv usbData);



// IR: Pin Definitionen
const int RECV_PIN = 15; // (15)(2) IR Empfänger Pin
const int SEND_PIN = 4;  // (4)(3)IR Sender Pin
#define NO_LED_FEEDBACK_CODE

Preferences preferences;
bool sendDataDefaultToApi = false;
bool forward;
String serverURL;
// Task-Handles
TaskHandle_t irReadTaskHandle = NULL;

// Webserver auf Port 80
AsyncWebServer server(80);
// Event-Handler für SSE
AsyncEventSource events("/events");

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
    if (!http.connected()) {
        if (!http.begin(client, serverURL)) { // Verwenden Sie WiFiClient
            Serial.println("Failed to initialize HTTP client");
            return;
        }
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    }

    String payload = "provider=" + provider + "&data=" + data;
    if (debug){Serial.println("Sending data to API: " + payload);}

    int httpResponseCode = http.POST(payload);
    if (httpResponseCode > 0) {
        if (debug){Serial.printf("Server Response: %d\n", httpResponseCode);}
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
            route("IR", data);
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
void sendBLE(uint8_t modifier, uint8_t keycode, bool longPress) {
    uint16_t duration = longPress ? 555 : 100; // 555 ms für langen Tastendruck, 100 ms für kurzen Tastendruck
    if (connected) {
        uint8_t report[8] = {0};
        report[0] = modifier; // Modifier (e.g., Ctrl, Shift)
        report[2] = keycode;  // Keycode
        input->setValue(report, sizeof(report));
        input->notify();
        delay(duration);
        report[0] = 0;
        report[2] = 0;
        input->setValue(report, sizeof(report));
        input->notify();
        delay(100);
        String sendInfo = "Sent BLE Keycode: 0x" + String(keycode, HEX) + 
                        " Modifier: 0x" + String(modifier, HEX) +
                        " Long: " + String(longPress ? "true" : "false");
        events.send(sendInfo.c_str());
        Serial.println(sendInfo);
    } else {
         Serial.println("BLE Keyboad not connected, discarding input."); 
    }
}

// HTTP Endpunkt Handler für BLE
void handleSendBle(AsyncWebServerRequest *request)
{
    if (request->hasParam("modifier", true) &&
        request->hasParam("keycode", true) &&
        request->hasParam("isRepeat", true))
    {
        uint8_t modifier = strtoul(request->getParam("modifier", true)->value().c_str(), nullptr, 16);
        uint8_t keycode = strtoul(request->getParam("keycode", true)->value().c_str(), nullptr, 16);
        bool isRepeat = request->getParam("isRepeat", true)->value() == "true";
        sendBLE(modifier, keycode, isRepeat);
        request->send(200, "text/plain", "BLE Command Sent\n");
    }
    else
    {
        request->send(400, "text/plain", "Missing parameters");
    }
}

// ANCHOR Save Routedata
void handleSaveRequest(AsyncWebServerRequest *request) {
    if (!request->hasParam("data", true)) {
        request->send(400, "text/plain", "No data= provided");
        return;
    }
    String rawData = request->getParam("data", true)->value();

    // Retrieve the data
    if (debug){
        Serial.println("Received data:");
        Serial.println(rawData);
    }

    // Ensure the data ends with a newline character
    if (!rawData.endsWith("\n")) {
        rawData += "\n";
    }

    // Reset route count before processing new data
    routeCount = 0;
    int startIndex = 0;
    int endIndex = rawData.indexOf("\n");

    while (endIndex != -1) {
        String row = rawData.substring(startIndex, endIndex);

        if (row.startsWith("APIHost=")) {
            serverURL = row.substring(String("APIHost=").length());
            serverURL.trim();
            if (debug){Serial.printf("Parsed hostURL: %s\n", serverURL.c_str());}
        } else if (row.startsWith("sendToApi=")) {
            String forwardStr = row.substring(String("sendToApi=").length());
            forwardStr.trim();
            if (debug){Serial.printf("Parsed forwardStr: %s\n", forwardStr.c_str());}
            forward = (forwardStr == "true");
            if(debug){Serial.printf("Parsed forward: %s\n", forward ? "true" : "false");}
        } else if (routeCount < MAX_ROUTES) {
            // Split the row into individual components
            int componentIndex = 0;
            int componentStart = 0;
            int componentEnd = row.indexOf(',');

            while (componentEnd != -1) {
                String component = row.substring(componentStart, componentEnd);
                component.trim();

                switch (componentIndex) {
                    case 0:
                        routeTable[routeCount].source = component;
                        break;
                        // ANCHOR-FIX
                    case 1:
                        routeTable[routeCount].protocol = static_cast<decode_type_t>(strtol(component.c_str(), NULL, 16));
                        break;
                    case 2:
                        routeTable[routeCount].code = strtol(component.c_str(), NULL, 16);
                        break;
                    case 3:
                        routeTable[routeCount].address = strtol(component.c_str(), NULL, 16);
                        break;
                    case 4:
                        routeTable[routeCount].isRepeat = component.toInt();
                        break;
                    case 5:
                        routeTable[routeCount].modifier = strtol(component.c_str(), NULL, 16);
                        break;
                    case 6:
                        routeTable[routeCount].command = strtol(component.c_str(), NULL, 16);
                        break;
                    case 7:
                        routeTable[routeCount].keyLong = component.toInt();
                        break;
                    case 8:
                        routeTable[routeCount].actionFuncName = component;
                        break;
                    case 9:
                        routeTable[routeCount].oIRprot = static_cast<decode_type_t>(strtol(component.c_str(), NULL, 16));
                        break;
                    case 10:
                        routeTable[routeCount].oIRcode = strtol(component.c_str(), NULL, 16);
                        break;
                    case 11:
                        routeTable[routeCount].oIRaddress = strtol(component.c_str(), NULL, 16);
                        break;
                    case 12:
                        routeTable[routeCount].oIRisRepeat = component.toInt();
                        break;
                    case 13:
                        routeTable[routeCount].oBleMod = strtol(component.c_str(), NULL, 16);
                        break;
                    case 14:
                        routeTable[routeCount].oBleCode = strtol(component.c_str(), NULL, 16);
                        break;
                    case 15:
                        routeTable[routeCount].oRBleRepeat = component.toInt();
                        break;
                }

                componentIndex++;
                componentStart = componentEnd + 1;
                componentEnd = row.indexOf(',', componentStart);
            }
            routeCount++;
        }

        startIndex = endIndex + 1;
        endIndex = rawData.indexOf("\n", startIndex);
    }

    // Save all parsed routes to NVS
    saveRoutesToNVS();

    // Debug output for hostAddress and forward
    if (debug){Serial.printf("Saved ServerURL: %s\n", serverURL.c_str());}

    // Send success response
    request->send(200, "text/plain", "Routes and settings saved successfully");
}

void handleLoadRequest(AsyncWebServerRequest *request)
{
    loadRoutesFromNVS(); // Load saved data from NVS
    String responseData;
    for (int i = 0; i < routeCount; i++)
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
             "%s,0x%X,0x%X,0x%X,%d,0x%X,0x%X,%d,%s,0x%X,0x%X,0x%X,%d,0x%X,0x%X,%d\n",
             routeTable[i].source.c_str(),
             routeTable[i].protocol,
             routeTable[i].code,
             routeTable[i].address,
             routeTable[i].isRepeat,
             routeTable[i].modifier,
             routeTable[i].command,
             routeTable[i].keyLong,
             routeTable[i].actionFuncName.c_str(),
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

        // Fügen Sie hostAddress und forward zur Antwort hinzu
    responseData += "APIHost=" + serverURL + "\n";
    responseData += "sendToApi=" + String(forward ? "true" : "false") + "\n";

    request->send(200, "text/plain", responseData);
}
///////////////////////// Routes
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
                    events.send("Router: IR command send");
                    Serial.println("sendIR: Matched, Resend IR Data");
                    sendIR(route.oIRaddress, route.oIRcode, finalRepeat, route.oIRprot);
                }
                /*else if (route.actionFuncName == "sendBLE")
                {
                    bool finalRepeat = (route.isRepeat == 2) ? irData.isRepeat : route.oRBleRepeat;
                    Serial.println("sendIR: Matched, Sent BLE Data");
                    //sendBle(route.oBleMod, route.oBleCode, finalRepeat);
                    // ANCHOR-FIX
                    sendBle(uint8_t(route.oBleMod), uint8_t(route.oBleCode), bool(finalRepeat));

                }*/
                else if (route.actionFuncName == "sendHttpToAPI")
                {
                    String data = "protocol=" + String(irData.protocol) +
                                  "&code=" + String(irData.code) +
                                  "&address=" + String(irData.address) +
                                  "&isRepeat=" + String(irData.isRepeat);
                    Serial.println("sendIR: Matched, Sent API Data");
                    events.send("Route matched: Sending IR to HTTP Endpoint");
                    if (debug){Serial.println(data);}
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
    
    if (irData.protocol != 0) {
        sendHttpToAPI(source, data);
        events.send("No definition: Send IR to default HTTP URL");
        Serial.println("Sending to default URL");
        Serial.println("Not sending Unknown");
        } else { 
        Serial.println("Not sending Unknown");  
         }

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
                    Serial.println("USB: Matched, Sent Data to API");
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

void loadRoutesFromNVS() {
    if (!preferences.begin("routeData", true)) {
        Serial.println("Failed to initialize NVS for reading");
        return;
    }

    routeCount = preferences.getInt("routeCount", 0);
    Serial.printf("Loaded routeCount: %d\n", routeCount);

    if (routeCount > MAX_ROUTES) {
        Serial.println("Route count exceeds maximum capacity");
        preferences.end();
        return;
    }

    // Laden von serverURL und forward
    serverURL = preferences.getString("serverURL", "");
    forward = preferences.getBool("forward", false);

    preferences.end();
    Serial.println("Routes and settings loaded from NVS.");
}

void saveRoutesToNVS()
{
    Preferences preferences;
    if (!preferences.begin("routeData", false))
    {
        Serial.println("Failed to initialize NVS for writing");
        return;
    }

    preferences.putInt("routeCount", routeCount);
    preferences.putString("serverURL", serverURL);
    preferences.putBool("forward", forward);

    preferences.end();
    Serial.println("Routes and settings saved to NVS.");
}

void setup() {
    Serial.begin(115200);
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    wifiTask();
    loadRoutesFromNVS();
    webserver();
    initBLE("BLE IR Router");
    Serial.println("Advertising started!");
    // SSE-Endpunkt registrieren
    server.addHandler(&events);
      // HTTP-Endpunkte
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", htmlPage);    });
    server.on("/sendIR", HTTP_POST, [](AsyncWebServerRequest *request)
            { handleSendIr(request); });
    server.on("/sendBLE", HTTP_POST, [](AsyncWebServerRequest *request)
            { handleSendBle(request); });
    server.on("/loadData", HTTP_POST, [](AsyncWebServerRequest *request)
            { handleLoadRequest(request); });
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request)
            { handleSaveRequest(request); });
    
        // Erstelle eine neue FreeRTOS-Task für die IR-Empfänger-Logik
    BaseType_t result = xTaskCreatePinnedToCore(
            IRRecvTask,        // Task-Funktion
            "IRRecvTask",      // Name der Task
            8192,              // Stapelgröße in Wörtern
            NULL,              // Parameter für die Task
            1,                 // Priorität der Task
            NULL,              // Task-Handle
            1                  // Kern, auf dem die Task ausgeführt werden soll
        );



}

/* // Simulierte Log-Daten
unsigned long previousMillis = 0;
const long interval = 1000; // Intervall (1 Sekunde) */

void loop() {
/* 
        if (connected) {
        uint8_t keycode[8] = {0};
        keycode[2] = 0x0b; // 'h'
        input->setValue(keycode, sizeof(keycode));
        input->notify();
        delay(100);
        keycode[2] = 0x08; // 'e'
        input->setValue(keycode, sizeof(keycode));
        input->notify();
        delay(100);
        keycode[2] = 0x0f; // 'l'
        input->setValue(keycode, sizeof(keycode));
        input->notify();
        delay(100);
        keycode[2] = 0x0f; // 'l'
        input->setValue(keycode, sizeof(keycode));
        input->notify();
        delay(100);
        keycode[2] = 0x12; // 'o'
        input->setValue(keycode, sizeof(keycode));
        input->notify();
        delay(100);
        keycode[2] = 0x00; // release key
        input->setValue(keycode, sizeof(keycode));
        input->notify();
        delay(1000);
    } */
        

}
