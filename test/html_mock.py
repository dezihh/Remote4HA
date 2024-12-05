#!/usr/bin/python3
from flask import Flask, jsonify, request, render_template, Response
jsonify, request, render_template
from datetime import datetime

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


@app.route('/sendIR', methods=['POST'])
def send_ir():
    protocol = request.form.get('protocol')
    address = request.form.get('address')
    command = request.form.get('command')
    repeats = request.form.get('repeats')

    # Print the received data to the console
    print(f"Received data - Protocol: {protocol}, Address: {address}, Command: {command}, Repeats: {repeats}")

    response = {
        "status": "success",
        "message": f"IR command sent with protocol {protocol}, address {address}, command {command}, repeats {repeats}"
    }
    return jsonify(response)

@app.route('/sendBLE', methods=['POST'])
def send_ble():
    modifier = request.form.get('modifier')
    keycode = request.form.get('keycode')
    repeats = request.form.get('isRepeat')

    # Print the received data to the console
    print(f"Received data - Modifier: {modifier}, keycode: {keycode}, Repeat: {repeats}")

    response = {
        "status": "success",
        "message": f"BLE command sent with  Modifier: {modifier}, keycode: {keycode}, Repeat: {repeats}"
    }
    return jsonify(response)

#########################
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
#curl "http://127.0.0.1:5000/sendIR?protocol=NEC&address=1A2B&command=4D&repeats=true"

# Webseite bereitstellen
@app.route('/')
def index():
    # Render die index.html im templates-Ordner
    return render_template('index.html')

if __name__ == '__main__':
 app.run(host='0.0.0.0', port=5000, debug=True)
