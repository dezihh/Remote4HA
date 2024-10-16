/*********************************
Platformio.ini Einträge:
Framewort: arduino
Libs:    
  espressif/arduino-esp32@^2.0.0  ; ESP32 Arduino Core (wird automatisch eingebunden)
  T-vK/HID-Project@^2.5.0         ; Für HID-Tastatur-Funktionalität (BLE HID)
***********************************/

#define US_KEYBOARD 1  // Definiert, dass eine US-amerikanische Tastatur verwendet wird

#include <Arduino.h>
#include "BLEDevice.h"        // Bibliothek für Bluetooth Low Energy (BLE)
#include "BLEHIDDevice.h"     // Bibliothek für BLE HID (Human Interface Device)
#include "HIDTypes.h"         // Bibliothek für HID-Typen (z.B. Tastatur)
#include "HIDKeyboardTypes.h" // Bibliothek für spezielle HID-Tastaturtypen

#define HID_HOME 0x4A
#define HID_LEFT_ARROW 0x50
#define HID_RIGHT_ARROW 0x4F
#define HID_UP_ARROW 0x52
#define HID_DOWN_ARROW 0x51

// Werte, die bei Bedarf angepasst werden können
#define DEVICE_NAME "ESP32 Keyboard"  // Der Name, unter dem das BLE-Gerät erscheint

// Vorwärtsdeklaration von Funktionen
void bluetoothTask(void*);
void typeText(const char* text);

// Variable, die angibt, ob eine BLE-Verbindung besteht
bool isBleConnected = false;

void setup() {
    Serial.begin(115200);  // Serielle Kommunikation mit 115200 Baudrate beginnen

    // Starte die Bluetooth-Task (multithreaded)
    xTaskCreate(bluetoothTask, "bluetooth", 20000, NULL, 5, NULL);
}

void loop() {
    if (isBleConnected && Serial.available() > 0) {
        String inputText = Serial.readStringUntil('\n');  // Eingabe bis Zeilenende lesen
        inputText.trim();  // Entferne unnötige Leerzeichen

        if (inputText.length() > 0) {
            Serial.println("Processing input: " + inputText);
            processInput(inputText);  // Übergib den eingegebenen Text zur Verarbeitung
        }
    }

    delay(100);
}

// Struktur für ein Eingangsreport, das gesendet wird, wenn eine Taste gedrückt oder losgelassen wird
struct InputReport {
    uint8_t modifiers;	     // Bitmaske für Steuerungstasten: CTRL = 1, SHIFT = 2, ALT = 4
    uint8_t reserved;        // Muss immer 0 sein
    uint8_t pressedKeys[6];  // Liste der gedrückten Tasten (maximal 6 gleichzeitig)
};

// Struktur für ein Ausgangsreport, das empfangen wird, wenn der Zustand der LEDs (NumLock, CapsLock usw.) geändert wird
struct OutputReport {
    uint8_t leds;            // Bitmaske für LED-Zustände: Num Lock = 1, Caps Lock = 2, Scroll Lock = 4
};

// Beschreibt, wie das HID-Gerät (in diesem Fall eine Tastatur) funktioniert und welche Daten gesendet/empfangen werden
static const uint8_t REPORT_MAP[] = {
    USAGE_PAGE(1), 0x01,       // Allgemeine Desktop-Steuerungen (Kategorie)
    USAGE(1), 0x06,            // Tastatur
    COLLECTION(1), 0x01,       // Anwendung (Tastatur ist eine Anwendung)
    REPORT_ID(1), 0x01,        // Report-ID für diese Anwendung
    USAGE_PAGE(1), 0x07,       // Seite für Tastatur-/Tastaturbefehle
    USAGE_MINIMUM(1), 0xE0,    // Mindestwert (Linke Steuerungstaste)
    USAGE_MAXIMUM(1), 0xE7,    // Maximalwert (Rechte Steuerungstaste)
    LOGICAL_MINIMUM(1), 0x00,  // Jeder Wert ist entweder 0 oder 1
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_COUNT(1), 0x08,     // 8 Bits für die Modifikatortasten
    REPORT_SIZE(1), 0x01,
    HIDINPUT(1), 0x02,         // Eingabedaten
    REPORT_COUNT(1), 0x01,     // 1 Byte (nicht genutzt)
    REPORT_SIZE(1), 0x08,
    HIDINPUT(1), 0x01,         // Konstante Eingabe
    REPORT_COUNT(1), 0x06,     // 6 Bytes für maximal 6 gleichzeitig gedrückte Tasten
    REPORT_SIZE(1), 0x08,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x65,  // 101 Tasten insgesamt
    USAGE_MINIMUM(1), 0x00,
    USAGE_MAXIMUM(1), 0x65,
    HIDINPUT(1), 0x00,         // Tastatureingaben
    REPORT_COUNT(1), 0x05,     // 5 Bits für LEDs (Num Lock, Caps Lock, Scroll Lock, Compose, Kana)
    REPORT_SIZE(1), 0x01,
    USAGE_PAGE(1), 0x08,       // LED-Seite
    USAGE_MINIMUM(1), 0x01,    // Num Lock
    USAGE_MAXIMUM(1), 0x05,    // Kana
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    HIDOUTPUT(1), 0x02,        // Ausgabedaten
    REPORT_COUNT(1), 0x01,     // 3 Bits für Auffüllung (Padding)
    REPORT_SIZE(1), 0x03,
    HIDOUTPUT(1), 0x01,        // Konstante Ausgabe
    END_COLLECTION(0)          // Ende der Anwendungssammlung
};

BLEHIDDevice* hid;              // HID-Gerät für die BLE-Tastatur
BLECharacteristic* input;       // Eingangscharakteristik (für die Eingabedaten)
BLECharacteristic* output;      // Ausgangscharakteristik (für LED-Zustände)

const InputReport NO_KEY_PRESSED = { };  // Keine Taste gedrückt (leerer Report)

/*
 * BLE Callbacks: Aktionen, wenn ein Client (z.B. PC oder Smartphone) eine Verbindung herstellt oder trennt.
 */
class BleKeyboardCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* server) {
        isBleConnected = true;  // Verbindung hergestellt
        BLE2902* cccDesc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
        cccDesc->setNotifications(true);  // Erlaube Benachrichtigungen
        Serial.println("Client has connected");  // Ausgabe auf der seriellen Schnittstelle
    }

    void onDisconnect(BLEServer* server) {
        isBleConnected = false;  // Verbindung getrennt
        BLE2902* cccDesc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
        cccDesc->setNotifications(false);  // Keine Benachrichtigungen mehr
        Serial.println("Client has disconnected");
    }
};

/*
 * Callback für das Schreiben auf der Ausgangscharakteristik, z.B. wenn der Client die LED-Zustände ändern will.
 */
class OutputCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) {
        OutputReport* report = (OutputReport*) characteristic->getData();
        Serial.print("LED state: ");  // Ausgabe des LED-Zustands auf der seriellen Schnittstelle
        Serial.print((int) report->leds);
        Serial.println();
    }
};

// Task, die BLE initialisiert und den Dienst startet
void bluetoothTask(void*) {
    BLEDevice::init(DEVICE_NAME);  // BLE initialisieren mit dem Gerätenamen
    BLEServer* server = BLEDevice::createServer();  // Erstelle einen BLE-Server
    server->setCallbacks(new BleKeyboardCallbacks());  // Setze Callbacks für die Verbindung

    hid = new BLEHIDDevice(server);  // Erstelle ein HID-Gerät
    input = hid->inputReport(1);     // Eingangsreport (Tastendaten)
    output = hid->outputReport(1);   // Ausgangsreport (LED-Daten)
    output->setCallbacks(new OutputCallbacks());  // Setze Callbacks für LED-Änderungen

    hid->manufacturer()->setValue("Maker Community");  // Herstellername setzen
    hid->pnp(0x02, 0xe502, 0xa111, 0x0210);            // USB Vendor ID und Produkt-ID setzen
    hid->hidInfo(0x00, 0x02);                          // HID-Informationen setzen

    BLESecurity* security = new BLESecurity();  // Sicherheitsmerkmale setzen
    security->setAuthenticationMode(ESP_LE_AUTH_BOND); // Bonding (Gerätekopplung) aktivieren

    hid->reportMap((uint8_t*)REPORT_MAP, sizeof(REPORT_MAP));  // Report-Map setzen
    hid->startServices();  // HID-Dienste starten

    hid->setBatteryLevel(100);  // Batterie-Level auf 100% setzen

    BLEAdvertising* advertising = server->getAdvertising();  // BLE-Werbung starten
    advertising->setAppearance(HID_KEYBOARD);  // Geräteerscheinungsbild als Tastatur
    advertising->addServiceUUID(hid->hidService()->getUUID());
    advertising->addServiceUUID(hid->deviceInfo()->getUUID());
    advertising->addServiceUUID(hid->batteryService()->getUUID());
    advertising->start();  // Werbung starten

    Serial.println("BLE ready");  // Ausgabe auf der seriellen Schnittstelle
    delay(portMAX_DELAY);  // Task bleibt unendlich lange aktiv
}

// Funktion zum Senden von Text über die BLE-Tastatur
void typeText(const char* text) {
    int len = strlen(text);  // Länge des Textes bestimmen
    for (int i = 0; i < len; i++) {
        uint8_t val = (uint8_t)text[i];  // ASCII-Wert des aktuellen Zeichens
        if (val > KEYMAP_SIZE) continue; // Überspringe nicht unterstützte Zeichen
        KEYMAP map = keymap[val];        // Entsprechende Tastenkombination abrufen

        InputReport report = {
            .modifiers = map.modifier,    // Modifikatoren (z.B. Shift)
            .reserved = 0,
            .pressedKeys = { map.usage, 0, 0, 0, 0, 0 }  // Gedrückte Taste
        };

        input->setValue((uint8_t*)&report, sizeof(report));  // Eingangsreport setzen
        input->notify();  // Report an den Client senden
        delay(5);  // Kurze Verzögerung

        input->setValue((uint8_t*)&NO_KEY_PRESSED, sizeof(NO_KEY_PRESSED));  // Alle Tasten loslassen
        input->notify();  // Leeren Report senden (keine Tasten gedrückt)
        delay(5);  // Kurze Verzögerung
    }
}
void processInput(const String& inputText) {
    InputReport report = NO_KEY_PRESSED;  // Start mit keinem Key gedrückt

    // Prüfe, ob es sich um eine Spezialtaste handelt
    if (inputText.equalsIgnoreCase("HOME")) {
        report.pressedKeys[0] = HID_HOME;
    } else if (inputText.equalsIgnoreCase("LEFT")) {
        report.pressedKeys[0] = HID_LEFT_ARROW;
    } else if (inputText.equalsIgnoreCase("RIGHT")) {
        report.pressedKeys[0] = HID_RIGHT_ARROW;
    } else if (inputText.equalsIgnoreCase("UP")) {
        report.pressedKeys[0] = HID_UP_ARROW;
    } else if (inputText.equalsIgnoreCase("DOWN")) {
        report.pressedKeys[0] = HID_DOWN_ARROW;
    } else if (isAlphanumeric(inputText)) {
        // Wenn der eingegebene Text alphanumerisch ist, sende ihn als normalen Text
        typeText(inputText.c_str());
        return;  // Nach dem Senden von alphanumerischem Text keine weitere Verarbeitung
    } else {
        Serial.println("Unrecognized input: " + inputText);
        return;  // Ungültige Eingabe, keine Aktion erforderlich
    }

    // Wenn eine Sondertaste erkannt wurde, sende den Report
    input->setValue((uint8_t*)&report, sizeof(report));
    input->notify();
    delay(5);

    // Taste loslassen
    input->setValue((uint8_t*)&NO_KEY_PRESSED, sizeof(NO_KEY_PRESSED));
    input->notify();
    delay(5);
}
// Prüfen, ob der eingegebene Text ein alphanumerisches Zeichen ist
bool isAlphanumeric(const String& inputText) {
    // Prüft, ob alle Zeichen im String alphanumerisch sind (Buchstaben oder Ziffern)
    for (size_t i = 0; i < inputText.length(); i++) {
        if (!isalnum(inputText[i])) {
            return false;  // Nicht alphanumerisch
        }
    }
    return true;  // Nur alphanumerische Zeichen
}




