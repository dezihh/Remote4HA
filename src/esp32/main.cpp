#include <Arduino.h>
#include <WiFiManager.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <BleKeyboard.h>

#include <BleKeyboard.h> 

//#include <BLEServer.h>
//#include <BLEDevice.h>
//#include <BLEHIDDevice.h>
//#include <BLEUtils.h>
//#include <HIDTypes.h>
//#include <HIDKeyboardTypes.h>
#include <WebServer.h>
#include <ESPmDNS.h>
//#include <NimBLEDevice.h>
//#include <NimBLEServer.h>
//#include <NimBLECharacteristic.h>
//#include <NimBLEHIDDevice.h>  // Für HID-Profile, falls nötig

//#include <HIDTypes.h>
//#include <HIDKeyboardTypes.h>
//#include <WebServer.h>
//#include <ESPmDNS.h>
//#define USE_USB 
#ifdef USE_USB 
    #include "EspUsbHost.h"
#endif
#include "reportMap.h"
#include <IRremote.hpp>
#include <HTTPClient.h> 

// USB Definitions
bool usb_connected = false;
unsigned long keyPressStartTime = 0;
bool keyPressed = false;
int longKey = 500;
uint8_t longKey = 500;


BleKeyboard bleKeyboard("ESP Console");

// BLE HID service variables
//BLEHIDDevice* hid;
/*NimBLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;*/
BleKeyboard bleKeyboard("ESP32 BLE Keyboard", "ESP32 BLE Keyboard", 100);
bool is_ble_connected = false;
void sendBle(uint8_t modifier, const String& keyString);
void sendIR(uint32_t address, uint8_t command, bool repeats, const String& protocol);
void sendBle(uint8_t modifier, const String& keyString, bool repeat);
void handleServerCommand(AsyncWebServerRequest *request);
void sendHttpToAPI(String provider, String data);

// IR: Pin Definitionen
const int RECV_PIN = 15;  // (15)(2) IR Empfänger Pin
const int SEND_PIN = 4;   // (4)(3)IR Sender Pin
#define NO_LED_FEEDBACK_CODE
struct IRrcv {
    decode_type_t protocol;
    uint32_t address;
    uint8_t command;
    bool isRepeat;
};

struct IFrcv {  
    uint8_t modifier;  
    uint8_t keycode;  
    int keyLong;  
};  

struct sendBLE {  
    uint8_t oBleModifier;  
    String oBleCode;  
    bool oRBleRepeat;  
};

struct IFRecv {  
    uint8_t modifier;  
    uint8_t command;  
    bool isRepeat;  
};  

struct sendBLE {  
    decode_type_t modifier;  
    String code;  
    bool isRepeat;  
};


// Common variables
const char* serverURL = "http://192.168.10.3:8125/api/webhook/myid";
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

struct Route {
    String source;               // IR, IF
    decode_type_t protocol;      // IR Prot
    uint8_t command;             // IR Command
    uint32_t address;            // IR Address
    uint8_t isRepeat;            // IR Repeat 0, 1 oder 2  
    uint8_t keycode;             // IF Keycode
    uint8_t modifier;            // IF Modifier
    int keyLong;                 // IF 0, 1 oder 2  

    // Funktionszeiger für spezifische Aktionen
    void (*irActionFunc)(uint32_t, uint8_t, bool, const String&); // Funktionszeiger für sendIR
    void (*bleActionFunc)(uint8_t, const String&, bool);          // Funktionszeiger für sendBle
    void (*httpActionFunc)(String, String);                       // Funktionszeiger für sendHttpToAPI

    decode_type_t IRprotocol;   // IR Prot
    uint8_t IRcommand;          // IR Command
    uint32_t IRaddress;         // IR Address
    bool IRisRepeat;            // IR Repeat 0, 1 oder 2  
    uint8_t oBleModifier;       // BLE Modifier
    String oBleCode;            // BLE Command
    bool oRBleRepeat;           // BLE Repeat
    String source;              // IR, IF
    decode_type_t protocol;     // IR Prot
    uint8_t command;            // IR Command
    uint32_t address;           // IR Address
    uint8_t isRepeat;           // IR Repeat 0, 1 oder 2  
    uint8_t keycode;            // IF Keycode
    uint8_t modifier;           // IF Modifier
    uint8_t keyLong;            // IF 0, 1 oder 2  
    // Ende Inputs
    void (*actionFunc)(IRRecv irData); // oder sendBLE / sendHttpToAPI  
    decode_type_t IRprotocol;   // IR Prot
    uint8_t IRcommand;          // IR Command
    uint32_t IRaddress;         // IR Address
    bool IRisRepeat;            // IR Repeat 0, 1 oder 2  
    decode_type_t modifier;     // BLE Modifier
    String code;                // BLE Command
    bool oRBleRepeat;           // BLE Repeat
};



// Statische Routingtabelle
Route routeTable[] = {
    {"IR", NEC, 0xe, 0x0, 0x0000, 2, 0x0, 0, &sendIR, nullptr, nullptr, UNKNOWN, 0xd, 0x0000, 0, KEY_LEFT_SHIFT, "A", 0},
    {"IF", UNKNOWN, 0x0, 0x0, 0x0000, 2, 0x0, 0, nullptr, &sendBle, nullptr, UNKNOWN, 0x0, 0x0000, 0, 0, "B", 0},
    {"API", UNKNOWN, 0x0, 0x0, 0x0000, 2, 0x0, 0, nullptr, nullptr, &sendHttpToAPI, UNKNOWN, 0x0, 0x0000, 0, 0, "", 0}
};


const int routeCount = sizeof(routeTable) / sizeof(routeTable[0]);  

void route(String source, IRrcv irData) {  
    for (int i = 0; i < routeCount; i++) {  
        Route& route = routeTable[i];  
  
        if (source == route.source && irData.protocol == route.protocol &&  
            irData.command == route.command && irData.address == route.address) {  
  
            bool isRepeatCondition = (route.isRepeat == 2) || (route.isRepeat == 1 && irData.isRepeat) || (route.isRepeat == 0 && !irData.isRepeat);  
            if (isRepeatCondition) {  
                // Aktion ausführen
                if (route.irActionFunc) {  
                    // Ruft sendIR auf
                    route.irActionFunc(route.address, route.command, irData.isRepeat, "IR Protocol String");  
                } else if (route.bleActionFunc) {  
                    // Ruft sendBle auf
                    route.bleActionFunc(route.oBleModifier, route.oBleCode, route.oRBleRepeat);  
                } else if (route.httpActionFunc) {  
                    // Ruft sendHttpToAPI auf
                    String params = "source=" + source + "&protocol=" + String(irData.protocol) +  
                                    "&command=" + String(irData.command) + "&address=" + String(irData.address) +  
                                    "&isRepeat=" + String(irData.isRepeat);  
                    route.httpActionFunc("Provider", params);  
                }  
                return; // Treffer gefunden, keine weitere Verarbeitung notwendig  
            }  
        }  
    }  
  
    // Wenn kein Treffer gefunden wurde  
    String params = "source=" + source + "&protocol=" + String(irData.protocol) +  
                    "&command=" + String(irData.command) + "&address=" + String(irData.address) +  
                    "&isRepeat=" + String(irData.isRepeat);  
    sendHttpToAPI("DefaultProvider", params);  
}

Route routeTable[] = {
    // {"Quelle", IR_Prot, IR_Code, IR_Addr, IR_Rep, IF code, IF Mod, IF KL, Func, "BLE mod", BLE code, KL, "IR_Prot", code, addr,mod}
    {"IR", NEC, 0xe,, 0x0000, 2, 0x0, ,0, "sendBLE", LEFT_SHIFT, "A", 0,"",0x0,0x0,0}, 
    {"IR", NEC, 0xe,, 0x0000, 2, 0x0, ,0, "sendIR", , "", 0,"SONY",0xf,0x0001,0}, 
    {"IR", NEC, 0xc,, 0x0000, 2, 0x0, ,0, "sendHttpToAPI(", , "", 0, "", 0x0, 0x000, 0} 
};

const int routeCount = sizeof(routeTable) / sizeof(routeTable[0]);  

void route(String source, IRRecv irData) {  
    for (int i = 0; i < routeCount; i++) {  
        Route& route = routeTable[i];  
  
        if (source == route.source && irData.protocol == route.protocol &&  
            irData.command == route.command && irData.address == route.address) {  
  
            bool isRepeatCondition = (route.isRepeat == 2) || (route.isRepeat == 1 && irData.isRepeat) || (route.isRepeat == 0 && irData.isRepeat);  
            if (isRepeatCondition) {  
                // Aktion ausführen  
                if (route.actionFunc == sendIR) {  
                    IRRecv outgoingData = {route.protocol, route.address, route.command, irData.isRepeat};  
                    route.actionFunc(outgoingData);  
                } else if (route.actionFunc == sendBLE) {  
                    // sendBLE benötigt die Daten aus routeTable  
                    uint8_t outgoingModifier = route.modifier;  
                    String outgoingCommandBLE = route.commandBLE;  
                    bool outgoingRepeat = route.oRBleRepeat;  
                    route.actionFunc(outgoingModifier, outgoingCommandBLE, outgoingRepeat);  
                } else if (route.actionFunc == sendHttpToAPI) {  
                    String params = "source=" + source + "&protocol=" + String(irData.protocol) +  
                                    "&command=" + String(irData.command) + "&address=" + String(irData.address) +  
                                    "&isRepeat=" + String(irData.isRepeat);  
                    route.actionFunc(params);  
                }  
                return; // Treffer gefunden, keine weitere Verarbeitung notwendig  
            }  
        }  
    }  
  
    // Wenn kein Treffer gefunden wurde  
    String params = "source=" + source + "&protocol=" + String(irData.protocol) +  
                    "&command=" + String(irData.command) + "&address=" + String(irData.address) +  
                    "&isRepeat=" + String(irData.isRepeat);  
    sendHttpToAPI(params); // Senden der
}



// Funktion zum Senden einer HTTP-Anfrage an die API
//void sendHttpToAPI(String provider, String address, String code) {
void sendHttpToAPI(String provider, String data) {
    HTTPClient http;
    http.begin(serverURL); 

    String payload = "provider=" + provider + "&data=" + data;
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
        Serial.printf("Server Response: %d\n", httpResponseCode);
    } else {
        Serial.printf("Error in sending request: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    http.end();
}

// Datenverarbeitung und Routinglogik
/* 
void processAndRouteData(const String& source, IRrcv& data) {
    bool routeMatched = false;

    for (size_t i = 0; i < routeCount; ++i) {
        const Route& route = routes[i];

        bool matchesSource = (route.source == source);
        bool matchesProtocol = (route.protocol == UNKNOWN || route.protocol == data.protocol);
        bool matchesCode = (route.command == 0 || route.command == data.command);
        bool matchesAddress = (route.address == 0 || route.address == data.address);
        bool matchesRepeat = (route.oRepeat  || route.oRepeat == data.isRepeat);

        if (matchesSource && matchesProtocol && matchesCode && matchesAddress && matchesRepeat) {
            routeMatched = true;
            Serial.printf("Routing Matched");
            
            applyRouteTransformations(data, route);

            if (route.actionFuncName == "sendIR") {
                String irstr = "Route Selected: IR sending Address: " + String(data.address, HEX) + " - Code: " + String(data.command, HEX) + 
                    " - Repeat: " + data.isRepeat + " - Protocol: " + (getProtocolString(data.protocol));
                Serial.println(irstr);
                sendIR(data.address, data.command, data.isRepeat, (getProtocolString(data.protocol)));
            } else if (route.actionFuncName == "sendBle") {
                // ANCHOR Fix it
                //String BLEstr = "BLE Protocol: Modifier: " + String(data.modifier, HEX) + " Code: " + String (data.command, HEX) + " Time: " + route.outKeyTime;
                //Serial.println(BLEstr);
                // ANCHOR Fix it
                //sendBle(data.modifier, data.command);
            } else if (route.actionFuncName == "sendHttpToAPI") {
                Serial.print("Route select: API: ");
                String IRstr = "IR to API Protocol: " + String(getProtocolString(data.protocol)) + " Address: " + String(data.address, HEX) + " Code: " + String (data.command, HEX) + " Repeat: " + data.isRepeat ;
                Serial.println(IRstr);
                sendHttpToAPI("IR", "&Protocol=" + String(getProtocolString(data.protocol)) + "&Address" + String( data.address, HEX) + "&Code=" + String(data.command, HEX) + "&Repeat=" + data.isRepeat);
             }
            break;
        }
    }

    // Falls keine Route gefunden wurde, senden wir die Daten an die API als Standard-Route
    if (!routeMatched) {
        Serial.println("Keine passende Route gefunden, Daten werden an API gesendet.");
        if (source == "IR") {
            //String(getProtocolString(data.protocol));
            String IRstr = "IR Protocol: " + String(getProtocolString(data.protocol)) + " Address: " + String(data.address, HEX) + " Code: " + String (data.command, HEX) + " Repeat: " + bool (data.isRepeat);
            Serial.println(IRstr);
            sendHttpToAPI("IR", "&Protocol=" + String(getProtocolString(data.protocol)) + "&Address" + String( data.address, HEX) + "&Code=" + String(data.command, HEX) + "&Repeat=" + data.isRepeat);
        } else {
            Serial.println("Keine gültige Source gefunden");
        }
    }
} */
// ANCHOR USB beginns
//19(D-) + 20(D+) (USB not UART) 
void setupUSBHost() {
    #ifdef USE_USB
        usbHost.begin();
        Serial.println("USB Keyboard initialized");
    #endif
}
void readUSB(uint8_t ascii, uint8_t keycode, uint8_t modifier) {
    usb_connected = true;

    if (!keyPressed) {  // Taste wurde gerade gedrückt
        keyPressStartTime = millis();  // Speichere den Zeitpunkt des Tastendrucks
        keyPressed = true;  // Setze den Status auf "Taste gedrückt"
    } else {  // Taste wurde losgelassen
        // Berechne die gedrückte Zeit
        unsigned long duration = millis() - keyPressStartTime;  // Zeitdauer in Millisekunden
        IFrcv data;
        data.keycode = (keycode, HEX);
        data.modifier = (modifier, HEX);
        data.keyLong = (duration > longKey); // Setze isRepeat auf true oder false 
        IFrcv data;
        data.command = (keycode, HEX);
        data.modifier = (modifier, HEX);
        data.isRepeat = (duration > longKey); // Setze isRepeat auf true oder false 
        String keyEntry = String("Key: ") + String(keycode, HEX) + 
                            " (ASCII:" + String((char)ascii) + 
                            ") Modifier: " + String(modifier, HEX) + 
                            " Duration: " + String(duration) + " ms\n";

        //serialOutput = keyEntry + serialOutput;  // Füge die Ausgabe zur seriellen Ausgabe hinzu
        Serial.printf("%s", keyEntry.c_str());

        // Reset der Statusvariablen
        keyPressed = false;  // Setze den Status auf "Taste nicht gedrückt"
//        route("IF", data);  // Rufe die Routingfunktion auf 

        
        // Routenaufruf mit den erstellten Daten  
        route("IF", data);  // Rufe die Routingfunktion auf 
        
    }
}
}
// ANCHOR IR beginns
void setupIRRecv(){
    // IR Setup mit erweiterten Optionen
    IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);
    Serial.print(F("Ready to receive IR signals of protocols: "));
    printActiveIRProtocols(&Serial);

    IrSender.begin(SEND_PIN, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);
    
    // Zusätzliche IR-Empfänger Konfiguration
    IrReceiver.enableIRIn();  // Start the receiver
    //IrReceiver.blink13(true); // LED blinkt bei IR-Empfang
}
// Funktion zum Senden von IR-Daten
void sendIR(uint32_t address, uint8_t command, bool repeats, const String& protocol) {
    String protocolUpper = String(protocol); // Erstelle eine Kopie des Strings
    protocolUpper.toUpperCase();
    IrReceiver.stop();
    if (protocolUpper  == "KASEIKYO_DENON") {
        IrSender.sendKaseikyo_Denon(address & 0xFFF, command, repeats);
    } else if (protocolUpper == "DENON") {
        IrSender.sendDenon(address & 0xFF, command, repeats);
    } else if (protocolUpper == "SONY") {
        IrSender.sendSony(address & 0xFF, command, repeats);
    } else if (protocolUpper == "LG") {
        IrSender.sendLG(address & 0xFF, command, repeats);
    } else if (protocolUpper == "NEC2") {
        IrSender.sendNEC2(address & 0xFF, command, repeats);
    } else if (protocolUpper == "NEC") {
        IrSender.sendNEC(address & 0xFF, command, repeats);
    } else {
        Serial.print(F("Unknown IR protocol requested: "));
        Serial.println(protocolUpper);
    }
    delay(40); // Wartezeit nach dem Senden
    IrReceiver.start();
    delay(60); // Wartezeit nach dem Senden
}
// Beispielhafte Datenverarbeitung für IR-Daten
void readIR() {
    if (IrReceiver.decode()) {
        IRrcv data;
        data.protocol = IrReceiver.decodedIRData.protocol;
        data.address = IrReceiver.decodedIRData.address;
        data.command = IrReceiver.decodedIRData.command;
        data.isRepeat = (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) != 0;

        route("IR", data);
        route("IR", data);
        IrReceiver.resume();
    }
}
// ANCHOR WiFi beginns
void setupWiFi() {
    WiFiManager wm;
    if (!wm.autoConnect("ESP32_AutoConnect")) {
        Serial.println("Failed to connect to WiFi. Restarting...");
        delay(3000);
        ESP.restart();
    } else {
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
void setupWebServer(){
    server.addHandler(&ws);
    
    
      // HTTP-POST-Endpunkt hinzufügen
    server.on("/sendCommand", HTTP_POST, handleServerCommand);
    
    // Web Server Setup
    //server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    //    request->send(200, "text/html", getHTML());
    //});
    server.begin();
}

void handleServerCommand(AsyncWebServerRequest *request) {
    if (request->hasParam("function", true)) {
        String functionStr = request->getParam("function", true)->value();
        // curl -X POST -d "function=IR&protocol=NEC&address=0x4&code=0x8&repeat=false" http://192.168.10.167/sendCommand
        if (functionStr == "IR") {
            if (request->hasParam("protocol", true) && request->hasParam("address", true) && 
                request->hasParam("code", true) && request->hasParam("repeat", true)) {

                String protocolStr = request->getParam("protocol", true)->value();
                String addrStr = request->getParam("address", true)->value();
                String codeStr = request->getParam("code", true)->value();
                String repeatStr = request->getParam("repeat", true)->value();

                char* endPtr;

                uint32_t address = strtoul(addrStr.c_str(), &endPtr, 16);
                if (*endPtr != '\0') {
                    request->send(400, "text/plain", "Invalid address format");
                    return;
                }

                uint8_t code = static_cast<uint8_t>(strtoul(codeStr.c_str(), &endPtr, 16));
                if (*endPtr != '\0') {
                    request->send(400, "text/plain", "Invalid code format");
                    return;
                }

                bool repeatBool = (repeatStr == "true");
                sendIR(address, code, repeatBool, protocolStr);
                
                // Send a success response
                request->send(200, "text/plain", "IR command processed successfully");
            } else {
                request->send(400, "text/plain", "Missing required parameters for IR function");
                return;
            }

        } else if (functionStr == "BLE") {
            if (request->hasParam("modifier", true) && request->hasParam("code", true)) {

                String modifierStr = request->getParam("modifier", true)->value();
                String codeStr = request->getParam("code", true)->value();

                char* endPtr;   
                uint8_t modifier = static_cast<uint8_t>(strtoul(modifierStr.c_str(), &endPtr, 16));
                if (*endPtr != '\0') {
                    request->send(400, "text/plain", "Invalid modifier format");
                    return;
                }

                sendBle(modifier, codeStr, false);
                //dataProcessor("BLE", "NULL", modifierStr, codeStr, repeatStr);

                // Send a success response
                request->send(200, "text/plain", "BLE command processed successfully");
                Serial.print(modifier); + Serial.print(" - "); + Serial.println(codeStr);
            } else {
                request->send(400, "text/plain", "Missing required parameters for BLE function");
                return;
            }

        } else {
            request->send(400, "text/plain", "Invalid function parameter");
            return;
        }
    } else {
        request->send(400, "text/plain", "Missing function parameter");
    }
}
// ANCHOR Bluetooth beginns

// Funktion zum Senden eines BLE-Reports
void sendBle(uint8_t modifier, const String& keyString, bool repeat) {  
    if (bleKeyboard.isConnected()) {  
        // Modifier anwenden, falls vorhanden  
        if (modifier != 0) {  
            bleKeyboard.press(modifier);  
        }  
  
        // KeyString senden  
        bleKeyboard.print(keyString);  
  
        // Modifier loslassen, falls angewendet  
        if (modifier != 0) {  
            bleKeyboard.release(modifier);  
        }  
  
        // Debug-Ausgabe  
        Serial.print("Sending BLE Report: Modifier: ");  
        Serial.print(modifier, HEX);  
        Serial.print(" KeyString: ");  
        Serial.println(keyString);  
    } else {  
        Serial.println("BLE not connected, discarding key input.");  
    }  
} 

void sendBle(uint8_t modifier, const String& keyString) {  
    if (bleKeyboard.isConnected()) {  
        // Modifier anwenden, falls vorhanden  
        if (modifier != 0) {  
            bleKeyboard.press(modifier);  
        }  
  
        // KeyString senden  
        bleKeyboard.print(keyString);  
  
        // Modifier loslassen, falls angewendet  
        if (modifier != 0) {  
            bleKeyboard.release(modifier);  
        }  
  
        // Debug-Ausgabe  
        Serial.print("Sending BLE Report: Modifier: ");  
        Serial.print(modifier, HEX);  
        Serial.print(" KeyString: ");  
        Serial.println(keyString);  
    } else {  
        Serial.println("BLE not connected, discarding key input.");  
    }  
} 




void setup() {
    Serial.begin(115200);
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    setupWiFi();
    setupWebServer();  // Webserver initialisieren
    setupIRRecv();
    bleKeyboard.begin();
    bleKeyboard.begin();  // BLE Keyboard initialisieren

}

void loop() {
    readIR();
    // Falls IF-Daten existieren, kann hier `processAndRouteIFData()` eingebaut werden.
}
