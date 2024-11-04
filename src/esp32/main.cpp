#include <Arduino.h>
#include <WiFiManager.h>
//#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <BLEUtils.h>
#include <HIDTypes.h>
#include <HIDKeyboardTypes.h>
#include "reportMap.h"
#include <IRremote.hpp>
#include <HTTPClient.h> 

// BLE HID service variables
BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;
bool is_ble_connected = false;

struct MyIRData {
    decode_type_t protocol;
    uint16_t address;
    uint16_t command;
    bool isRepeat;
    String getProtocolName() const { return getProtocolString(protocol);  }
};

// Pin Definitionen
const int RECV_PIN = 2;  // (15) IR Empfänger Pin
const int SEND_PIN = 3;   // (4)IR Sender Pin
#define NO_LED_FEEDBACK_CODE

#define DELAY_AFTER_SEND 200

const char* serverURL = "http://192.168.10.3:8125/api/webhook/myid";

// Server Instance
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// HTML Webpage
String getHTML() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 IR Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; }
        .container { max-width: 800px; margin: auto; }
        #logBox { 
            width: 100%;
            height: 200px;
            border: 1px solid #ccc;
            overflow-y: scroll;
            margin: 10px 0;
            padding: 10px;
        }
        .controls {
            display: flex;
            gap: 10px;
            margin: 10px 0;
            width: 100px;
        }
        select, input, button {
            padding: 5px;
            font-size: 16px;
        }
        
    </style>
</head>
<body>
  <div class="container">
    <h2>ESP32 IR Control</h2>
    <div id="logBox"></div>
      <div class="controls">
        <div>
          <label for="protocol">Protocol</label><br>
          <select id="protocol">
            <option value="NEC">NEC</option>
            <option value="SONY">SONY</option>
            <option value="LG">LG</option>
            <option value="NEC2">NEC2</option>
            <option value="SAMSUNG">SAMSUNG</option>
            <option value="KASEIKYO_DENON">KASEIKYO_DENON</option>
            <option value="DENON">DENON</option>
          </select>
        </div>
        <div>
          <label for="address">Adresse=0x:</label><br>
          <input type="text" id="address" size=4 placeholder="Enter IR address (hex)">
        </div>
        <div>
          <label for="code">Code: 0x</label><br>
          <input type="text" id="code" size=4 placeholder="Enter IR code (hex)">
        </div>
        <div>
          <label for="repeat">Repeat</label><br>
          <select id="repeat">
            <option value="true">True</option>
            <option value="false">False</option>
          </select>
        </div>
        <div>
          <button onclick="sendCode()">Send</button>
        </div>
      </div>

  </div>
    
    <script>
        var gateway = `ws://${window.location.hostname}/ws`;
        var websocket;
        
        function initWebSocket() {
            websocket = new WebSocket(gateway);
            websocket.onopen    = onOpen;
            websocket.onclose   = onClose;
            websocket.onmessage = onMessage;
        }
        
        function onOpen(event) {
            console.log('Connection opened');
        }
        
        function onClose(event) {
            console.log('Connection closed');
            setTimeout(initWebSocket, 2000);
        }
        
        function onMessage(event) {
            var logBox = document.getElementById('logBox');
            logBox.innerHTML += event.data + '<br>';
            logBox.scrollTop = logBox.scrollHeight;
        }
        
      function sendCode() {
          var protocol = document.getElementById('protocol').value;
          var address = document.getElementById('address').value;
          var code = document.getElementById('code').value;
          var repeat = document.getElementById('repeat').value;

          // Die Daten für die POST-Anfrage
          var data = new URLSearchParams();
          data.append("function", "IR");
          data.append("protocol", protocol);
          data.append("address", address);
          data.append("code", code);
          data.append("repeat", repeat);
          
          // Sende eine POST-Anfrage an den Server
          fetch("/sendCommand", {
                  method: "POST",
                  headers: {
                      "Content-Type": "application/x-www-form-urlencoded"
                  },
                  body: data.toString()
              })
              .then(response => {
                  if (!response.ok) {
                      return response.text().then(text => { throw new Error(text) });
                  }
                  return response.text();
              })
              .then(responseText => {
                  console.log("Server response:", responseText);
              })
              .catch(error => {
                  console.error("Error:", error);
                  alert(error.message);  // Zeige die Fehlermeldung im Browser an
              });
          }

        
        window.addEventListener('load', initWebSocket);
    </script>
</body>
</html>
)rawliteral";
}

// Lesen der IR Daten
MyIRData processIRData() {
    MyIRData data;  // Verwendung der eigenen Struktur
    // Protokoll und andere Daten ausgelesen, falls ein IR-Signal erkannt wurde
    data.protocol = IrReceiver.decodedIRData.protocol;
    data.address = IrReceiver.decodedIRData.address;
    data.command = IrReceiver.decodedIRData.command;
    data.isRepeat = (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) != 0;
    
      // We have debug enabled or an unknown protocol, print extended info
      if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
         Serial.println(F("D: Received noise or an unknown (or not yet enabled) protocol"));
         IrReceiver.printIRResultRawFormatted(&Serial, true);
           
        if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
          auto tDecodedRawData = IrReceiver.decodedIRData.decodedRawData; // uint32_t on 8 and 16 bit CPUs and uint64_t on 32 and 64 bit CPUs
            Serial.print(F("Raw data received are 0x"));
            Serial.println(tDecodedRawData);
        } 
      }
    return data;
}

// Hier wird IR gesendet
void sendIR(uint16_t sAddress, uint8_t sCommand, bool sRepeats, const char* Sprotocol) {
    if (strcmp(Sprotocol, "KASEIKYO_DENON") == 0 ) {
        // Sende das IR-Signal mit dem Denon-Protokoll
        IrSender.sendKaseikyo_Denon(sAddress & 0xFFF, sCommand, sRepeats);
        delay(DELAY_AFTER_SEND);
    } else if (strcmp(Sprotocol, "DENON") == 0 ) {
        // Sende das IR-Signal mit dem NEC-Protokoll
        IrSender.sendDenon(sAddress & 0xFF, sCommand, sRepeats);
        delay(DELAY_AFTER_SEND);
    } else if (strcmp(Sprotocol, "SONY") == 0 ) {
        // Sende das IR-Signal mit dem SONY-Protokoll - (tAddress, tCommand, aNumberOfRepeats, aIRSendData->numberOfBits);
        IrSender.sendSony(sAddress & 0xFF, sCommand, sRepeats, 12);
        delay(DELAY_AFTER_SEND);
    } else if (strcmp(Sprotocol, "LG") == 0 ) {
        // Sende das IR-Signal mit dem NEC-Protokoll
        IrSender.sendLG(sAddress & 0xFF, sCommand, sRepeats);
        delay(DELAY_AFTER_SEND);
    } else if (strcmp(Sprotocol, "NEC2") == 0 ) {
        // Sende das IR-Signal mit dem NEC-Protokoll
        IrSender.sendNEC2(sAddress & 0xFF, sCommand, sRepeats);
        delay(DELAY_AFTER_SEND);
    } else if (strcmp(Sprotocol, "NEC") == 0 ) {
        // Sende das IR-Signal mit dem NEC-Protokoll
        IrSender.sendNEC(sAddress & 0xFF, sCommand, sRepeats);
        
        delay(DELAY_AFTER_SEND);
    } else {
        Serial.println(F("Unknown IR protokoll requested!"));
    }
}

// Funktion zur Verarbeitung von Server-POST-Daten und Ausführung eines IR-Befehls
void handleServerCommand(AsyncWebServerRequest *request) {
  // Überprüfen, ob Funktion IR aufzurufen ist und ob alle Felder korrekt gefüllt sind
  if (request->hasParam("function", true) && request->getParam("function", true)->value() == "IR" &&
  request->hasParam("protocol", true) && request->hasParam("address", true) && request->hasParam("code", true) && request->hasParam("repeat", true)) {

    String protocolStr = request->getParam("protocol", true)->value();
    String addrStr = request->getParam("address", true)->value();
    String codeStr = request->getParam("code", true)->value();
    String repeatStr = request->getParam("repeat", true)->value();

    bool repeatBool = (repeatStr == "true");
    
    // Überprüfung auf Hex-Werte Addresse und Code
    char* endPtr;
    uint32_t address = strtoul(addrStr.c_str(), &endPtr, 16);
    if (*endPtr != '\0') {
        request->send(400, "text/plain", "Invalid address format: address");
        return;
    }

    uint32_t code = strtoul(codeStr.c_str(), &endPtr, 16);
    if (*endPtr != '\0') {
        request->send(400, "text/plain", "Invalid code format: code");
        return;
    }
      // IR-Code senden
    try {
        sendIR(address, code, repeatBool, protocolStr.c_str());
        
        ws.textAll("IR command sent: Protocol = " + protocolStr + 
                  ", Address = 0x" + addrStr + ", Code = 0x" + codeStr + " - Repeat: " + repeatBool);
        request->send(200, "text/plain", "IR command executed successfully");
    } catch (...) {
        request->send(500, "text/plain", "Error executing IR command");
        ws.textAll("IR command sent failed: Protocol = " + protocolStr + 
                  ", Address = 0x" + addrStr + ", Code = 0x" + codeStr + " - Repeat: " + repeatBool);
    }
  } else  {
    request->send(400, "text/plain", "Invalid function entered");
    return;
    }
}


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    Serial.println("In handleWebSocketMessage");
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        String message = String((char*)data);
        Serial.println("In der Nachricht WebSocket");
        // Aufteilen der Nachricht in ihre Bestandteile
        int firstComma = message.indexOf(',');
        int secondComma = message.indexOf(',', firstComma + 1);
        
        if(firstComma > 0 && secondComma > 0) {
            decode_type_t protocol = (decode_type_t)message.substring(0, firstComma).toInt();
            uint16_t address = strtoul(message.substring(firstComma + 1, secondComma).c_str(), NULL, 16);
            uint8_t command = strtoul(message.substring(secondComma + 1).c_str(), NULL, 16);
            
            // IR-Signal senden
        //    sendIR(address, command, 0, protocol);
            
            // Bestätigung über WebSocket senden
            //String confirmMsg = "IR-Signal gesendet - Adresse: 0x" + String(address, HEX) + 
            //                  ", Command: 0x" + String(command, HEX);
            //ws.textAll(confirmMsg);
        }
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
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

// Funktion zum Senden der Daten an die API von HA
void sendHttpToServer(String provider, String address, String code) {
    HTTPClient http;
    http.begin(serverURL);

    // Festlegen der Nutzdaten
    String payload = "provider=" + provider + "&address=" + address + "&code=" + code;

    // POST-Anfrage mit Nutzdaten senden
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST(payload);

    // Antwort prüfen
    if (httpResponseCode > 0) {
        Serial.printf("Server Response: %d\n", httpResponseCode);
    } else {
        Serial.printf("Error in sending request: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    // Verbindung beenden
    http.end();
} 
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

void setupIRRecv(){
    // IR Setup mit erweiterten Optionen
    IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);
    Serial.print(F("Ready to receive IR signals of protocols: "));
    printActiveIRProtocols(&Serial);

    IrSender.begin(SEND_PIN, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);
    
    // Zusätzliche IR-Empfänger Konfiguration
    IrReceiver.enableIRIn();  // Start the receiver
    IrReceiver.blink13(true); // LED blinkt bei IR-Empfang
}

void setupWebServer(){
    server.addHandler(&ws);
    
      // HTTP-POST-Endpunkt hinzufügen
    server.on("/sendCommand", HTTP_POST, handleServerCommand);
    
    // Web Server Setup
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", getHTML());
    });
    server.begin();
}


// Define a callback class for BLE Server connection events
class BLECallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        Serial.println("BLE Client connected.");
        ws.textAll("BLE Client connected."); 
        // If needed, adjust power management or other settings here
    }
    
    void onDisconnect(BLEServer* pServer) override {
        Serial.println("BLE Client disconnected.");
        ws.textAll("BLE advertising..."); 
        BLEDevice::startAdvertising();  // Restart advertising when client disconnects
    }
};
    // Funktion zum Senden der BLE Nachricht
void sendBleReport(uint8_t modifier, uint8_t keycode) {
    if (is_ble_connected) {
        uint8_t report[8] = {0};
        report[0] = modifier; // Setze das Modifier-Byte
        report[2] = keycode;  // Setze den Keycode
        input->setValue(report, sizeof(report)); // Setze den Wert des Reports
        input->notify(); // Sende den Report über BLE

        // Optional: Hier könnte eine Verzögerung und ein Reset des Reports erfolgen
        delay(100);
        memset(report, 0, sizeof(report)); // Setze den Report zurück
        input->setValue(report, sizeof(report)); // Leeren Report senden
        input->notify(); // Sende den leeren Report über BLE
    }else {
            Serial.println("BLE not connected, discarding key input.");
        }
}

void setupBLE() {
    BLEDevice::init("ESP32 BLE Keyboard");
    BLEServer* pServer = BLEDevice::createServer();
    pServer->setCallbacks(new BLECallbacks());

    hid = new BLEHIDDevice(pServer);
    input = hid->inputReport(1);
    output = hid->outputReport(1);

    hid->manufacturer()->setValue("ESP32");
    hid->pnp(0x01, 0x02e5, 0xabcd, 0x0110);
    hid->hidInfo(0x00, 0x01);

    BLESecurity* pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

    hid->reportMap((uint8_t*)reportMap, sizeof(reportMap));
    hid->startServices();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setAppearance(HID_KEYBOARD);
    pAdvertising->addServiceUUID(hid->hidService()->getUUID());
    pAdvertising->start();
    Serial.println("Waiting for a client connection to notify...");
}

void setup() {
    Serial.begin(115200);
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    
    // Start all Services
    setupWiFi();
    setupWebServer();
    ws.onEvent(onEvent);
    setupIRRecv();
    setupBLE();

}


void loop() {
    ws.cleanupClients();
    if (IrReceiver.decode()) {
      MyIRData irData = processIRData();  // IR-Daten auslesen und speichern
      String outputStr = "IR command received: Protokol: " + irData.getProtocolName() +
                         " (" + String(irData.protocol, HEX) + ")" +
                         " - Adresse: " + String(irData.address, HEX) +
                         " - Befehl: " + String(irData.command, HEX) +
                         " - Wiederholung: " + String(irData.isRepeat ? "Ja" : "Nein");
      Serial.println(outputStr);
      if (outputStr != 0) {  // Nur senden wenn ein gültiger Command vorliegt
        sendHttpToServer("IR", String(irData.address, HEX), String(irData.command, HEX));
        ws.textAll(outputStr);
      }
      IrReceiver.resume();
    }
    if (!is_ble_connected) {
        BLEDevice::startAdvertising();
        Serial.println("Re-starting BLE advertising...");
        ws.textAll("Re-starting BLE advertising...");
        delay(500);
    }

}
