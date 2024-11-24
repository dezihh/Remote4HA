#!/usr/bin/python3
from flask import Flask, jsonify, request, render_template
jsonify, request, render_template

from flask import Flask, render_template
from flask_sock import Sock
import threading  # Add this import
import time
import json


app = Flask(__name__)
sock = Sock(app)  # Initialize Sock instead of SocketIO

# Mock-Endpunkte

@app.route('/save', methods=['POST'])
def save():
    # Die Rohdaten (Text) empfangen
    raw_data = request.data.decode('utf-8')

    if raw_data:
        print(f"Daten empfangen: {raw_data}")
        # Hier könntest du die Daten weiter verarbeiten oder speichern
        return f"Daten erfolgreich gespeichert: {raw_data}", 200
    else:
        return "Fehler: Keine Daten empfangen.", 400

# WebSocket-Events
active_websocket = None

def send_periodic_data():
    global active_websocket
    while True:
        if active_websocket:
            try:
                # Example: Send some mock sensor or system data
                data = {
                    'timestamp': time.time(),
                    'temperature': 22.5,
                    'humidity': 45.3
                }
                active_websocket.send(json.dumps(data))
            except Exception as e:
                print(f"Error sending periodic data: {e}")
                active_websocket = None
        time.sleep(5)  # Send every 5 seconds

# Start the periodic data thread
periodic_thread = threading.Thread(target=send_periodic_data, daemon=True)
periodic_thread.start()

@sock.route('/ws')
def websocket(ws):
    global active_websocket
    try:
        # Set the active WebSocket
        active_websocket = ws

        # Initial connection message
        ws.send('Connection established')

        # Keep the connection open and listen for messages
        while True:
            data = ws.receive()
            if data:
                print(f"Received: {data}")
                # Optional: Process or respond to received data
    except Exception as e:
        print(f"WebSocket error: {e}")
    finally:
        # Clear the active WebSocket when connection closes
        active_websocket = None
###########

@app.route('/defaultRouteFields', methods=['GET', 'POST'])
def default_route_fields():
    if request.method == 'GET':
        return jsonify({"status": "success", "message": "GET response from /defaultRouteFields"}), 200
    elif request.method == 'POST':
        data = request.json
        return jsonify({"status": "success", "received": data}), 200

@app.route('/data', methods=['POST'])
def data():
    data = request.json
    return jsonify({"status": "success", "received": data}), 200

# curl -X POST -d "data=IR,NEC,0x1,0x9999,0,0x0,0,1,sendHttpToAPI,,0,0,0,0,0;IR,NEC,0x2,0x8888,0,0x0,0,1,sendIR,,0,0,0,0,0" http://192.168.10.167/save
#curl -X POST -H "Content-Type: text/plain" --data-binary @- http://127.0.0.1:5000/save <<EOF
#IR,1,0x12,0x0001,2,0,0,0,sendHttpToAPI,0,0,0,0,0,0,0
#IR,8,0x6,0x0002,0,0,0,0,sendIR,2,0x1,0x0002,1,0,0,1
#sendApiToTrue,true
#EOF
@app.route('/loadData', methods=['GET'])
def load_data():
    # CSV-Daten, wie sie vorher waren
    mock_data = """
    IR,1,0x12,0x0001,2,0,0,0,sendHttpToAPI,0,0,0,0,0,0,0
    IR,8,0x6,0x0002,0,0,0,0,sendIR,2,0x1,0x0002,1,0,0,1
    USB,8,0x6,0x0003,0,0x3,0x4,2,sendIR,2,0x1,0x0002,1,0,0,1
    """

    # Der SendToApi-Status (True oder False)
    send_to_api_value = True  # Zum Beispiel, True für aktiviert, False für deaktiviert

    # Wir fügen den Status als zusätzliche Zeile in der CSV hinzu
    send_to_api_row = f"sendToApi,{send_to_api_value}"

    # Gesamte Antwort: CSV-Daten und der Status als zusätzliche Zeile
    response_data = mock_data + "\n" + send_to_api_row
    #response_data = mock_data + "\n"

    return response_data, 200, {'Content-Type': 'text/plain'}


#curl "http://127.0.0.1:5000/sendBLE?modifier=Ctrl&keycode=A1&repeats=true"
@app.route('/sendBLE', methods=['GET'])
def send_ble():
    # URL-Parameter auslesen
    modifier = request.args.get('modifier')
    keycode = request.args.get('keycode')
    repeats = request.args.get('repeats')

    # Überprüfen, ob alle Parameter vorhanden sind
    if not all([modifier, keycode, repeats]):
        return jsonify({"status": "error", "message": "Missing parameters"}), 400

    # Boolean-Wert für 'repeats' konvertieren
    repeats_bool = repeats.lower() == 'true'

    # Mock-Antwort zurückgeben
    response = {
        "status": "success",
        "message": "BLE data received",
        "received": {
            "modifier": modifier,
            "keycode": keycode,
            "repeats": repeats_bool
        }
    }
    return jsonify(response), 200

#curl "http://127.0.0.1:5000/sendIR?protocol=NEC&address=1A2B&command=4D&repeats=true"
@app.route('/sendIR', methods=['GET'])
def send_ir():
    # URL-Parameter auslesen
    protocol = request.args.get('protocol')
    address = request.args.get('address')
    command = request.args.get('command')
    repeats = request.args.get('repeats')

    # Überprüfen, ob alle Parameter vorhanden sind
    if not all([protocol, address, command, repeats]):
        return jsonify({"status": "error", "message": "Missing parameters"}), 400

    # Boolean-Wert für 'repeats' konvertieren
    repeats_bool = repeats.lower() == 'true'

    # Mock-Antwort zurückgeben
    response = {
        "status": "success",
        "message": "IR data received",
        "received": {
            "protocol": protocol,
            "address": address,
            "command": command,
            "repeats": repeats_bool
        }
    }
    return jsonify(response), 200

# Webseite bereitstellen
@app.route('/')
def index():
    # Render die index.html im templates-Ordner
    return render_template('index.html')

if __name__ == '__main__':
 app.run(host='0.0.0.0', port=5000, debug=True)
