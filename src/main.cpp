#include <Arduino.h>
#include <WiFiManager.h>  
#include <USB.h>  
#include <usbhub.h>  
#include <hidboot.h>  
#include <WebServer.h>  
  
// Pin-Konfiguration für das USB Host Shield  
#define USB_CS_PIN 5  
#define USB_MOSI_PIN 23  
#define USB_MISO_PIN 19  
#define USB_SCK_PIN 18  
  
// Erstellen einer Klasse für HID-Geräte  
class MyHID : public HIDBoot<USB_HUB> {  
public:  
    MyHID(USB *usb) : HIDBoot<USB_HUB>(usb) {}  
  
    void OnHIDReport(uint8_t *report, int len) {  
        String data = "Daten empfangen: ";  
        for (int i = 0; i < len; i++) {  
            data += String(report[i], HEX) + " ";  
        }  
        Serial.println(data);  
        webData += data + "\n"; // Speichere die Daten für die Webseite  
    }  
};  
  
// Globale Variablen  
USB Usb;  
MyHID hid(&Usb);  
WebServer server(80);  
WiFiManager wifiManager;  
String webData; // Variable für die Webseitenausgabe  
  
void setup() {  
    Serial.begin(115200);  
  
    // WiFiManager einrichten  
    wifiManager.autoConnect("ESP32_HID_AP");  
    Serial.println("Verbunden mit WLAN");  
  
    // Konfiguration des USB Host Shields  
    if (Usb.Init() == -1) {  
        Serial.print(F("\r\nUSB Fehler"));  
        while (1); // Fehlerbehandlung  
    }  
    Serial.print(F("\r\nUSB bereit"));  
  
    // Webserver-Routen definieren  
    server.on("/", []() {  
        server.send(200, "text/html",   
            "<h1>HID Daten</h1><pre id='data'></pre>"  
            "<script>setInterval(() => fetch('/data').then(response => response.text()).then(data => document.getElementById('data').innerText = data), 1000);</script>");  
    });  
  
    server.on("/data", []() {  
        server.send(200, "text/plain", webData.c_str()); // Daten für die Webseite  
    });  
  
    server.begin();  
}  
  
void loop() {  
    Usb.Task();  
    server.handleClient(); // Webserver-Client-Anfragen verarbeiten  
}  

