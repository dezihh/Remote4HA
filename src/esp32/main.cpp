#include <Arduino.h>
#include <WiFiManager.h>
#include <AsyncTCP.h>
#include "webpage.h"
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <BleKeyboard.h> 
#include <ArduinoJson.h>
#include <Preferences.h>  // Für das Speichern und Laden von Daten
#include <LittleFS.h>


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

// BLE HID service variables
BleKeyboard bleKeyboard("ESP32 BLE Keyboard", "ESP32 BLE Keyboard", 100);
bool is_ble_connected = false;
void sendBle(uint8_t modifier, uint8_t keycode, bool isRepeat);
void sendIR(uint32_t address, uint8_t command, bool repeats, const String& protocol);
void handleServerCommand(AsyncWebServerRequest *request);
void sendHttpToAPI(String provider, String data);

// IR: Pin Definitionen
const int RECV_PIN = 15;  // (15)(2) IR Empfänger Pin
const int SEND_PIN = 4;   // (4)(3)IR Sender Pin
#define NO_LED_FEEDBACK_CODE

Preferences preferences;
bool sendDataDefaultToApi = false;

struct IRrcv {    //Eingang IR
    decode_type_t protocol; 
    uint8_t code;
    uint32_t address;
    bool isRepeat;
};

struct USBRecv {    // Eingang USB
    uint8_t modifier;  
    uint8_t command;  
    bool keyLong;  
};  


// Common variables
const char* serverURL = "http://192.168.10.3:8125/api/webhook/myid";
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Routingstruktur mit Zielwerten für Protokoll, Adresse, Command und Modifier
const size_t MAX_ROWS = 100;
struct Route {
    String source;               // IR, IF
    decode_type_t protocol;      // IR Prot
    uint8_t code;                // IR Command
    uint32_t address;            // IR Address/
    int isRepeat;                // IR Repeat 0, 1 oder 2  
    uint8_t modifier;            // USB Keycode
    uint8_t command;             // USB Modifier
    int keyLong;                 // USB 0, 1 oder 2  

    String actionFuncName;       // Sendefunction
    decode_type_t oIRprot;       // IR Prot
    uint8_t oIRcode;             // IR Code
    uint32_t oIRaddress;         // IR Address
    bool oIRisRepeat;            // IR Repeat 0 oder 1 
    uint8_t oBleMod;             // BLE Modifier
    uint8_t oBleCode;            // BLE Command
    bool oRBleRepeat;            // BLE Repeat
};


Route routeTable[MAX_ROWS];
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



const int routeCount = sizeof(routeTable) / sizeof(routeTable[0]);  

void route(String source, IRrcv irData) {
    for (int i = 0; i < sizeof(routeTable) / sizeof(Route); i++) {
        Route& route = routeTable[i];

        // Überprüfung für IR-Daten
        if (source == "IR" &&
            irData.protocol == route.protocol &&
            irData.code == route.code &&
            irData.address == route.address) {
            
            // Prüfbedingung für Repeat
            bool repeatCondition = (route.isRepeat == 2) || 
                                   (route.isRepeat == 1 && irData.isRepeat) || 
                                   (route.isRepeat == 0 && !irData.isRepeat);

            if (repeatCondition) {
                // Aktion ausführen basierend auf actionFuncName
                if (route.actionFuncName == "sendIR") {
                    bool finalRepeat = (route.isRepeat == 2) ? irData.isRepeat : route.oIRisRepeat;
                    // ANCHOR WS Log
                    ws.textAll("IR command send");
                    Serial.println("sendIR: Matched, Resend IR Data");
                    sendIR(route.oIRaddress, route.oIRcode, finalRepeat, String(route.oIRprot));
                } else if (route.actionFuncName == "sendBLE") {
                    bool finalRepeat = (route.isRepeat == 2) ? irData.isRepeat : route.oRBleRepeat;
                    Serial.println("sendIR: Matched, Sent BLE Data");
                    sendBle(route.oBleMod, route.oBleCode, finalRepeat);
                } else if (route.actionFuncName == "sendHttpToAPI") {
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

void route(String source, USBRecv usbData) {
    for (int i = 0; i < sizeof(routeTable) / sizeof(Route); i++) {
        Route& route = routeTable[i];

        // Überprüfung für USB-Daten
        if (source == "IF" &&
            usbData.modifier == route.modifier &&
            usbData.command == route.command) {
            
            // Prüfbedingung für keyLong
            bool keyLongCondition = (route.keyLong == 2) || 
                                     (route.keyLong == 1 && usbData.keyLong) || 
                                     (route.keyLong == 0 && !usbData.keyLong);

            if (keyLongCondition) {
                // Aktion ausführen basierend auf actionFuncName
                if (route.actionFuncName == "sendIR") {
                    bool finalRepeat = (route.keyLong == 2) ? usbData.keyLong : route.oIRisRepeat;
                    Serial.println("USB: Matched, Sent IR Data");
                    sendIR(route.oIRaddress, route.oIRcode, finalRepeat, String(route.oIRprot));
                } else if (route.actionFuncName == "sendBLE") {
                    bool finalRepeat = (route.keyLong == 2) ? usbData.keyLong : route.oRBleRepeat;
                    Serial.println("USB: Matched, Sent BLE Data");
                    sendBle(route.oBleMod, route.oBleCode, finalRepeat);
                } else if (route.actionFuncName == "sendHttpToAPI") {
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
        USBRecv data;
        data.command = (keycode, HEX);
        data.modifier = (modifier, HEX);
        data.keyLong = (duration > longKey); // Setze isRepeat auf true oder false 
        String keyEntry = String("Key: ") + String(keycode, HEX) + 
                            " (ASCII:" + String((char)ascii) + 
                            ") Modifier: " + String(modifier, HEX) + 
                            " Duration: " + String(duration) + " ms\n";

        //serialOutput = keyEntry + serialOutput;  // Füge die Ausgabe zur seriellen Ausgabe hinzu
        Serial.printf("%s", keyEntry.c_str());

        // Reset der Statusvariablen
        keyPressed = false;  // Setze den Status auf "Taste nicht gedrückt"
        
        // Routenaufruf mit den erstellten Daten  
        //route("IF", data);  // Rufe die Routingfunktion auf 
        
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
        data.code = IrReceiver.decodedIRData.command;
        data.isRepeat = (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) != 0;
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
        
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", htmlPage);  // send_P für PROGMEM
    });
    
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
}
// ANCHOR Bluetooth beginns
void handleServerCommand(AsyncWebServerRequest *request) {
    if (!request->hasParam("function", true)) {
        request->send(400, "text/plain", "Missing 'function' parameter");
        return;
    }

    String functionStr = request->getParam("function", true)->value();

    if (functionStr == "IR") {
        if (!(request->hasParam("protocol", true) &&
              request->hasParam("address", true) &&
              request->hasParam("code", true) &&
              request->hasParam("repeat", true))) {
            request->send(400, "text/plain", "Missing required parameters for IR function");
            return;
        }

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
        request->send(200, "text/plain", "IR command processed successfully");

    } else if (functionStr == "BLE") {
        // Ähnliche Struktur für BLE
    } else {
        request->send(400, "text/plain", "Unknown function");
    }
}

// Funktion zum Senden eines BLE-Reports
void sendBle(uint8_t modifier, uint8_t keycode, bool isRepeat) {
    if (bleKeyboard.isConnected()) {

        // Modifier anwenden, falls vorhanden
        if (modifier != 0) {
            bleKeyboard.press(modifier);
        }

        // Keycode drücken
        bleKeyboard.press(keycode);

        // Wenn isRepeat wahr ist, halte die Taste für xyz Millisekunden
        if (isRepeat) {
            delay(longKey);
        }else {
            delay(100);
        }

        // Keycode loslassen
        bleKeyboard.release(keycode);

        // Modifier loslassen, falls angewendet
        if (modifier != 0) {
            bleKeyboard.release(modifier);
        }

        // Debug-Ausgabe
        Serial.print("Sending BLE Report: Modifier: "); +  Serial.print(modifier, HEX); +  Serial.print(" Keycode: "); + Serial.println(keycode);
    } else {
        Serial.println("BLE not connected, discarding key input.");
        ws.textAll("BLE not connected, discarding key input");
    }
}



void setup() {
    Serial.begin(115200);
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    setupWiFi();
    setupWebServer();  // Webserver initialisieren
    setupIRRecv();
    bleKeyboard.begin();  // BLE Keyboard initialisieren
      // Speicher testen, indem statische JSON-Daten gespeichert werden
}

void loop() {
    readIR();
    // Falls IF-Daten existieren, kann hier `processAndRouteIFData()` eingebaut werden.
}
