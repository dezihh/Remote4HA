#include <Arduino.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include "webpage.h"
#ifdef BOARD_ESP32_DEVKITC
    #include "EspUsbHost.h"
#endif
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <NimBLECharacteristic.h>
#include <Preferences.h> 
#include <nvs_flash.h>
#include <ESPmDNS.h>
#define MAX_ROUTES 100
#include "reportMap.h"
#include <IRremote.hpp>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

WiFiClient client; // Verwenden Sie WiFiClient
HTTPClient http; // Globaler HTTPClient

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

// IR: Pin Definitionen
#ifdef BOARD_ESP32_DEVKITC
    #define IR_RECEIVE_PIN           2
    #define IR_SEND_PIN              3
    #define ENABLE_LED_FEEDBACK     false
#else
    #define IR_RECEIVE_PIN          15 // (15)(2) IR Empfänger Pin
    #define IR_SEND_PIN              4  // (4)(3)IR Sender Pin
    #define ENABLE_LED_FEEDBACK     true
#endif
#define NO_LED_FEEDBACK_CODE 

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

void sendBLE(uint8_t modifier, uint8_t keycode, bool isRepeat);
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
extern AsyncEventSource events;

#ifdef BOARD_ESP32_DEVKITC
class MyEspUsbHost : public EspUsbHost {
public:
    bool usb_connected = false;


    void onKeyboardKey(uint8_t ascii, uint8_t keycode, uint8_t modifier) {
        usb_connected = true;
    
        if (!keyPressed) {  // Taste wurde gerade gedrückt
            keyPressStartTime = millis();  // Speichere den Zeitpunkt des Tastendrucks
            keyPressed = true;  // Setze den Status auf "Taste gedrückt"
        } else {  // Taste wurde losgelassen
            // Berechne die gedrückte Zeit
            unsigned long duration = millis() - keyPressStartTime;  // Zeitdauer in Millisekunden
            USBRecv usbData;
            usbData.command = keycode;
            usbData.modifier = modifier;
            usbData.keyLong = duration > longKey ? 2 : 0;

            String keyEntry = String("Key: ") + String(keycode, HEX) + 
                              " (ASCII:" + String((char)ascii) + 
                              ") Modifier: " + String(modifier, HEX) + 
                              " Duration: " + String(duration) + " ms\n";
            route("USB", usbData);
            Serial.printf("%s", keyEntry.c_str());
            events.send(keyEntry.c_str());

            //sendBleReport(modifier, keycode);

            // Reset der Statusvariablen
            keyPressed = false;  // Setze den Status auf "Taste nicht gedrückt"
        }
     }
};
MyEspUsbHost usbHost;
#endif

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
    bool oRBleRepeat;      // BLE 
    String comment;        // Kommentar
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
    // void IRRecvTask() {
    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);
    //IrReceiver.begin(IR_RECEIVE_PIN, true, IR_RECEIVE_PIN);
    // IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);
    Serial.print(F("Ready to receive IR signals of protocols: "));
    String protocol = []{ String output; class StringPrinter : public Print 
        { public: String &output; StringPrinter(String &output) : 
        output(output) {} size_t write(uint8_t c) { output += (char)c; return 1; } }; 
        StringPrinter printer(output); printActiveIRProtocols(&printer); return output; }();
    String protocols = "Activated IR Protocols: " + protocol;
    events.send(protocol.c_str());
    // ANCHOR ws.textAll(protocol.c_str());
    Serial.println(protocol);

    IrSender.begin(IR_SEND_PIN);
        //IrSender.begin(IR_SEND_PIN, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);

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
                Serial.printf("Sending HID Report: Modifier: 0x%02X, Keycode: 0x%02X\n", modifier, keycode);

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
         events.send("BLE Keyboard not connected, discarding input.");
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
    Serial.println("Received data:");
    Serial.println(rawData);

    if (!rawData.endsWith("\n")) {
        rawData += "\n";
    }

    routeCount = 0;
    int startIndex = 0;
    int endIndex = rawData.indexOf("\n");

    while (endIndex != -1) {
        String row = rawData.substring(startIndex, endIndex);

        if (row.startsWith("APIHost=")) {
            serverURL = row.substring(String("APIHost=").length());
            serverURL.trim();
            Serial.printf("Parsed hostURL: %s\n", serverURL.c_str());
        } else if (row.startsWith("sendToApi=")) {
            String forwardStr = row.substring(String("sendToApi=").length());
            forwardStr.trim();
            Serial.printf("Parsed forwardStr: %s\n", forwardStr.c_str());
            forward = (forwardStr == "true");
            Serial.printf("Parsed forward: %s\n", forward ? "true" : "false");
        } else if (routeCount < MAX_ROUTES) {
            int componentIndex = 0;
            int componentStart = 0;
            int componentEnd = row.indexOf(',');

            while (componentIndex < 17) {
                String component;
                if (componentEnd == -1) {
                    component = row.substring(componentStart);
                } else {
                    component = row.substring(componentStart, componentEnd);
                }
                component.trim();

                switch (componentIndex) {
                    case 0:
                        routeTable[routeCount].source = component;
                        break;
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
                    case 16:
                        routeTable[routeCount].comment = component;
                        Serial.printf("Parsed comment: %s\n", component.c_str());
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

    saveRoutesToNVS();

    Serial.printf("Saved ServerURL: %s\n", serverURL.c_str());
    Serial.printf("Saved forward: %s\n", forward ? "true" : "false");

    request->send(200, "text/plain", "Routes and settings saved successfully");
}

void handleLoadRequest(AsyncWebServerRequest *request)
{
    loadRoutesFromNVS(); // Load saved data from NVS
    String responseData;
    for (int i = 0; i < routeCount; i++)
    {
        char buffer[512];
        snprintf(buffer, sizeof(buffer),
             "%s,0x%X,0x%X,0x%X,%d,0x%X,0x%X,%d,%s,0x%X,0x%X,0x%X,%d,0x%X,0x%X,%d,%s\n",
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
             routeTable[i].oRBleRepeat,
             routeTable[i].comment.c_str());
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
                else if (route.actionFuncName == "sendBLE")
                {
                    bool finalRepeat = (route.isRepeat == 2) ? irData.isRepeat : route.oRBleRepeat;
                    Serial.println("sendIR: Matched, Sent BLE Data");
                    sendBLE(route.oBleMod, route.oBleCode, finalRepeat);
                    // ANCHOR-FIX
                    //sendBle(uint8_t(route.oBleMod), uint8_t(route.oBleCode), bool(finalRepeat));

                }
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
    
    if (irData.protocol != 0x0){
        if (forward == true){
            sendHttpToAPI(source, data);
            events.send("No definition: Send IR to default HTTP URL");
            Serial.println("Sending to default URL");
        } else { 
            Serial.println("Not no forwarding enables. Discarding data.");  
        }
    }
}

void route(String source, USBRecv usbData)
{
    for (int i = 0; i < sizeof(routeTable) / sizeof(Route); i++)
    {
        Route &route = routeTable[i];

        // Überprüfung für USB-Daten
        if (source == "USB" &&
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
                    events.send("Route matched: Sending USB to IR");
                    sendIR(route.oIRaddress, route.oIRcode, finalRepeat, route.oIRprot);
                }
                else if (route.actionFuncName == "sendBLE")
                {
                    bool finalRepeat = (route.keyLong == 2) ? usbData.keyLong : route.oRBleRepeat;
                    Serial.println("USB: Matched, Sent BLE Data");
                    events.send("Route matched: Sending USB to BLE");
                    sendBLE(route.oBleMod, route.oBleCode, finalRepeat);
                }
                else if (route.actionFuncName == "sendHttpToAPI")
                {
                    String data = "modifier=" + String(usbData.modifier) +
                                  "&command=" + String(usbData.command) +
                                  "&keyLong=" + String(usbData.keyLong);
                    Serial.println("USB: Matched, Sent Data to API");
                    events.send("Route matched: Sending IR to HTTP Endpoint");
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
    Serial.println("None: No Route Match found");
    if (forward){
        sendHttpToAPI(source, data);
        events.send("No definition: Send USB to default HTTP URL");
        Serial.println("Sending to default URL");
        }
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

    for (int i = 0; i < routeCount; i++) {
        String key = "route" + String(i);
        if (preferences.getBytesLength(key.c_str()) == sizeof(Route)) {
            preferences.getBytes(key.c_str(), &routeTable[i], sizeof(Route));
        } else {
            Serial.println("Route data size mismatch for key: " + key);
        }
    }

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

    for (int i = 0; i < routeCount; i++) {
        String key = "route" + String(i);
        preferences.putBytes(key.c_str(), &routeTable[i], sizeof(Route));
        Serial.printf("Route %d: source=%s, protocol=0x%X, code=0x%X, address=0x%X, isRepeat=%d, modifier=0x%X, command=0x%X, keyLong=%d, actionFuncName=%s, oIRprot=0x%X, oIRcode=0x%X, oIRaddress=0x%X, oIRisRepeat=%d, oBleMod=0x%X, oBleCode=0x%X, oRBleRepeat=%d, comment=%s\n",
                      i, routeTable[i].source.c_str(), routeTable[i].protocol, routeTable[i].code, routeTable[i].address, routeTable[i].isRepeat, routeTable[i].modifier, routeTable[i].command, routeTable[i].keyLong, routeTable[i].actionFuncName.c_str(), routeTable[i].oIRprot, routeTable[i].oIRcode, routeTable[i].oIRaddress, routeTable[i].oIRisRepeat, routeTable[i].oBleMod, routeTable[i].oBleCode, routeTable[i].oRBleRepeat, routeTable[i].comment.c_str());
    }

    preferences.end();
    Serial.println("Routes and settings saved to NVS.");
}

void listPartitions() {
    const esp_partition_t* partition;
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);

    Serial.println("App Partitions:");
    while (it != NULL) {
        partition = esp_partition_get(it);
        Serial.printf("Name: %s, Type: %d, SubType: %d, Address: 0x%08x, Size: 0x%08x\n",
                      partition->label, partition->type, partition->subtype, partition->address, partition->size);
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);

    it = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
    Serial.println("Data Partitions:");
    while (it != NULL) {
        partition = esp_partition_get(it);
        Serial.printf("Name: %s, Type: %d, SubType: %d, Address: 0x%08x, Size: 0x%08x\n",
                      partition->label, partition->type, partition->subtype, partition->address, partition->size);
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);
}

void showNVSUsage() {
    // NVS-Nutzung anzeigen
    nvs_stats_t nvs_stats;
    esp_err_t err = nvs_get_stats(NULL, &nvs_stats);
    if (err != ESP_OK) {
        Serial.printf("Error (%s) getting NVS stats!\n", esp_err_to_name(err));
        return;
    }

    Serial.printf("NVS Usage: Used entries = %u, Free entries = %u, All entries = %u, Namespace count = %u\n",
                  nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries, nvs_stats.namespace_count);
}


void setup() {
    Serial.begin(115200);
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    if (psramFound()) {
        Serial.println("PSRAM is available.");
        Serial.printf("Total PSRAM: %u bytes\n", ESP.getPsramSize());
        Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
    } else {
        Serial.println("PSRAM is not available.");
    }   

    showNVSUsage(); 
    Serial.printf("Flashsize: %u bytes\n", ESP.getFlashChipSize());
    wifiTask();

    loadRoutesFromNVS();
    webserver();
    initBLE("BLE IR Router");
    Serial.println("Advertising started!");
    #ifdef BOARD_ESP32_DEVKITC
        usbHost.begin();
    #endif

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

void loop() {
    #ifdef BOARD_ESP32_DEVKITC
        usbHost.task();
    #endif
}
