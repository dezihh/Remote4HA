//#include <Arduino.h>
#include <WiFiManager.h>    // WiFiManager Bibliothek
#include <AsyncTCP.h>       // Für den Webserver auf ESP32
#include <ESPAsyncWebServer.h>
#include <hidcomposite.h>
#include <usbhub.h>
#include <SPI.h>


String hidData = "";  // String, um empfangene HID-Daten zu speichern
AsyncWebServer server(80); // Webserver auf Port 80

// Override HIDComposite to be able to select which interface we want to hook into
class HIDSelector : public HIDComposite {
public:
    HIDSelector(USB *p) : HIDComposite(p) {}

protected:
    void ParseHIDData(USBHID *hid, uint8_t ep, bool is_rpt_id, uint8_t len, uint8_t *buf);
    bool SelectInterface(uint8_t iface, uint8_t proto);
};

// Return true for the interface we want to hook into
bool HIDSelector::SelectInterface(uint8_t iface, uint8_t proto) {
    if (proto != 0) return true;  // Hier wird die Protokoll-ID gefiltert
    return false;
}

// Will be called for all HID data received from the USB interface
void HIDSelector::ParseHIDData(USBHID *hid, uint8_t ep, bool is_rpt_id, uint8_t len, uint8_t *buf) {
    if (len && buf) {
        Serial.print("HID Data: ");
        for (uint8_t i = 0; i < len; i++) {
            char hexChar[5];
            sprintf(hexChar, "%02X ", buf[i]);
            Serial.print(hexChar);
            hidData += String(hexChar);  // Speichere die empfangenen Daten im String
        }
        Serial.println();
    }
}

USB Usb;
HIDSelector hidSelector(&Usb);

// Funktion zum Einrichten des Webservers
void setupWebServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = "<html><body><h1>USB HID Data</h1>";
        html += "<p>Data: " + hidData + "</p>";
        html += "</body></html>";
        request->send(200, "text/html", html);  // HTML-Seite mit HID-Daten senden
    });

    server.begin();  // Starte den Server
}

void setup() {
    Serial.begin(115200);
    while (!Serial);  // Warte auf serielle Verbindung
    Serial.println("Start");

    // SPI Pins definieren
    //SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_SS);
 
  #ifdef ESP32
  Serial.println("ESP32 basiertes Board erkannt!");
  #endif
    // WiFiManager initialisieren
    WiFiManager wm;

    // Automatisches Verbinden oder Setup-Seite anzeigen, wenn keine Verbindung hergestellt werden kann
    if (!wm.autoConnect("ESP_USB_Host")) {
        Serial.println("Failed to connect to WiFi");  // Diese Zeile wird nur ausgegeben, wenn die Verbindung fehlschlägt
        // Hier kein ESP.restart(), da der ESP nun im AP-Modus ist und die Setup-Seite bereitstellt
    }
    Serial.println("Connected to WiFi!");
    Serial.println(WiFi.localIP());  // Zeigt die IP-Adresse an

    // Webserver einrichten
    setupWebServer();

    // USB-Host initialisieren
    if (Usb.Init() == -1) {
        Serial.println("USB Host Shield initialization failed.");
        return;
    }
    Serial.println("USB Host Shield initialized.");

    // Set this to higher values to enable more debug information
    UsbDEBUGlvl = 0xff;

    delay(200);

}

void loop() {
    Usb.Task();  // USB-Task ausführen
}
