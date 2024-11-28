#include <Arduino.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include "webpage.h"

// Webserver auf Port 80
AsyncWebServer server(80);

// Event-Handler für SSE
AsyncEventSource events("/events");

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

// Beispiel-HTML-Seite (wird als HTTP-Server ausgeliefert)
const char mywebpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 SSE Log Viewer</title>
    <style>
        body {
            font-family: Arial, sans-serif;
        }
        #log {
            white-space: pre-wrap;
            background: #f3f3f3;
            padding: 10px;
            border: 1px solid #ccc;
            max-height: 300px;
            overflow-y: auto;
        }
    </style>
</head>
<body>
    <h1>ESP32 Live Logs</h1>
    <div id="log">Waiting for log data...</div>

    <script>
        const eventSource = new EventSource("/events");
        const logElement = document.getElementById("log");

        eventSource.onmessage = function(event) {
            logElement.textContent += event.data + "\\n";
            logElement.scrollTop = logElement.scrollHeight;
        };

        eventSource.onerror = function() {
            logElement.textContent += "⚠️ Connection lost. Retrying...\\n";
        };
    </script>
</body>
</html>
)rawliteral";

// Simulierte Log-Daten
unsigned long previousMillis = 0;
const long interval = 1000; // Intervall (1 Sekunde)

void setup() {
  Serial.begin(115200);
  Serial.print("Free Heap: ");
  Serial.println(ESP.getFreeHeap());
  setupWiFi();
  Serial.println("Verbunden! IP-Adresse: ");
  Serial.println(WiFi.localIP());

  // SSE-Endpunkt registrieren
  server.addHandler(&events);

  // HTTP-Endpunkt für die Hauptseite
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", htmlPage);
  });

  // Starte den Server
  server.begin();
}

void loop() {
    unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Beispiel: Sende eine Log-Nachricht
    String message = "Log-Eintrag: Zeit = " + String(millis() / 1000) + " Sekunden";
    events.send(message.c_str(), "log", millis());
    Serial.println("Gesendet: " + message);   
  }
}
