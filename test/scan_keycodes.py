#!/usr/bin/env python
## Script for sending keycodes to IR / BLE Router
import time
import requests

def send_requests(start_hex, end_hex, modifier):
    # Konvertiere die Eingabe-Start- und Endnummern in Ganzzahlen
    start_num = int(start_hex, 16)
    end_num = int(end_hex, 16)

    # URL der Ziel-Webseite
    url = "http://192.168.10.167/sendBLE"

    # Schleife von Startnummer bis Endnummer
    for keycode in range(start_num, end_num + 1):
        # Konvertiere die aktuelle Nummer zurück in Hex-Format
        keycode_hex = hex(keycode)

        # Erstelle die Daten für den POST-Aufruf
        data = {
            "modifier": modifier,
            "keycode": keycode_hex,
            "isRepeat": "0"
        }

        # Sende den POST-Aufruf
        try:
            response = requests.post(url, data=data)
            print(f"Gesendet: {data}, Antwort: {response.status_code}, {response.text}")
        except requests.exceptions.RequestException as e:
            print(f"Fehler beim Senden der Anfrage: {e}")

        # Warte 0,4 Sekunden vor dem nächsten Aufruf
        time.sleep(0.4)

if __name__ == "__main__":
    # Eingabeaufforderung für die Start- und Endnummer sowie den Modifier
    start_hex = input("Gib die Startnummer in Hexadezimal ein (z. B. 0x10): ")
    end_hex = input("Gib die Endnummer in Hexadezimal ein (z. B. 0x20): ")
    modifier = input("Gib den Modifier ein (z. B. 0x0): ")

    # Funktion aufrufen
    send_requests(start_hex, end_hex, modifier)
