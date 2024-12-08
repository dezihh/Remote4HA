#!/usr/bin/env python

from flask import Flask, jsonify, request, render_template, Response
from flask_sock import Sock
from datetime import datetime
import threading
import time
import json
import re

# ANSI-Escape-Sequenzen für Farben
RED = '\033[91m'
GREEN = '\033[92m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
MAGENTA = '\033[95m'
CYAN = '\033[96m'
RESET = '\033[0m'


app = Flask(__name__)
sock = Sock(app)  # Initialize Sock instead of SocketIO

# ############### Mock-Endpunkte
## Save to ESP
""" Als curl-Befehl: curl -X POST http://127.0.0.1:5000/save -H "Content-Type: application/x-www-form-urlencoded"  --data-urlencode 'data=IR,0x2,0x01,0x0003,0,0x0,0x0,,sendIR,0x4,0x01,0x0001,0,,0x0
IR,0x5,0x05,0x0005,1,0x0,0x0,,sendIR,0x5,0x01,0x0001,0,,0x0
sendToApi=false
APIHost=http://192.168.10.3:8125/api/webhook/myid' """

@app.route('/save', methods=['POST'])
def save():
    # Die Rohdaten (Text) empfangen
    raw_data = request.form.get('data')

    if raw_data:
        print(f"Daten empfangen: {raw_data}")
        # Hier könntest du die Daten weiter verarbeiten oder speichern

        # Ausgabe der empfangenen Daten an der Konsole
        print("Empfangene Daten an der Konsole:")
        for line in raw_data.split('\n'):
            print(line)

        return f"Daten erfolgreich gespeichert: {raw_data}", 200
    else:
        return "Fehler: Keine Daten empfangen.", 400

##Direct Call IR
# curl -X POST -d "protocol=8&address=0x0&command=0x44&repeats=0" http://127.0.0.1:5000/sendIR
@app.route('/sendIR', methods=['POST'])
def send_ir():
    protocol = request.form.get('protocol')
    address = request.form.get('address')
    command = request.form.get('command')
    repeats = request.form.get('repeats')

    # Print the received data to the console
    print(f"{GREEN}Received IR Direct data - Protocol: {CYAN}{protocol}{GREEN}, Address: {CYAN}{address}{GREEN}, Command: {CYAN}{command}{GREEN}, Repeats: {CYAN}{repeats}{RESET}")

    response = {
        "status": "success",
        "message": f"IR command sent with protocol {protocol}, address {address}, command {command}, repeats {repeats}"
    }
    return jsonify(response)


## Direct call BLE
# curl -X POST -d "modifier=0x0&keycode=0x44&isRepeat=0" http://127.0.0.1:5000/sendIR
@app.route('/sendBLE', methods=['POST'])
def send_ble():
    modifier = request.form.get('modifier')
    keycode = request.form.get('keycode')
    repeats = request.form.get('isRepeat')

    # Print the received data to the console
    print(f"{GREEN}Received BLE data - Modifier: {CYAN}{modifier}{GREEN}, Keycode: {CYAN}{keycode}{GREEN}, Repeats: {CYAN}{repeats}{RESET}")

    response = {
        "status": "success",
        "message": f"IR command sent with, modifier {modifier}, keycode {keycode}, repeats {repeats}"
    }
    return jsonify(response)

# Time Message, every 5 seconds on Messagebox #########
def generate_time_messages():
    """Generator, der die aktuelle Uhrzeit im SSE-Format sendet."""
    while True:
        # Hol die aktuelle Uhrzeit
        current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        yield f"data: {current_time}\n\n"  # SSE-Nachricht
        time.sleep(5)  # Alle 5 Sekunden eine neue Nachricht senden
@app.route('/events')
def events():
    """SSE-Endpunkt, der Nachrichten an den Client streamt."""
    return Response(generate_time_messages(), content_type='text/event-stream')

@app.route('/data', methods=['POST'])
def data():
    data = request.json
    return jsonify({"status": "success", "received": data}), 200

## Anzeige der Mockdaten im Browser
@app.route('/loadData', methods=['GET'])
def load_data():
    # CSV-Daten, wie sie vorher waren
    mock_data = """
    IR,0x4,0x12,0x0001,2,0,0,0,sendHttpToAPI,0,0,0,0,0,0,0
    IR,0x8,0x6,0x0002,0,0,0,0,sendIR,0x2,0x6,0x0002,1,0,0,1
    USB,0x88,0x6,0x0003,0,0x3,0x4,2,sendIR,0x2,0x1,0x0002,1,0,0,1
    sendToApi=true
    APIHost=http://192.168.10.3:8125/api/webhook/myid
    """
    
    # Gesamte Antwort: CSV-Daten und der Status als zusätzliche Zeile
    response_data = mock_data
    return response_data, 200, {'Content-Type': 'text/plain'}

## Anzeige der Webpage  src/esp32webpage.h
def extract_html_from_header_file(header_file_path):
    with open(header_file_path, 'r') as file:
        content = file.read()

    # Regex to extract the HTML content within the R"rawliteral(...)" block
    match = re.search(r'R"rawliteral\((.*?)\)rawliteral"', content, re.DOTALL)
    if match:
        return match.group(1)
    else:
        raise ValueError("HTML content not found in the header file")

@app.route('/')
def index():
    try:
        html_content = extract_html_from_header_file('../src/esp32/webpage.h')
        return html_content
    except Exception as e:
        return str(e), 500

# Webseite lokal unter Templates
@app.route('/index/')
def localindex():
    # Render die index.html im templates-Ordner
    return render_template('index.html')

if __name__ == '__main__':
 app.run(host='0.0.0.0', port=5000, debug=True)
