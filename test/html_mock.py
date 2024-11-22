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
    # Die Formulardaten (URL-kodiert) empfangen
    data = request.form.get('data')

    if data:
        print(f"Daten empfangen: {data}")
        # Hier könntest du die Daten weiter verarbeiten oder speichern
        return f"Daten erfolgreich gespeichert: {data}", 200
    else:
        return "Fehler: Keine Daten empfangen.", 400

#if __name__ == '__main__':
#    app.run(debug=True)

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

#@app.route('/loadData', methods=['GET'])
#def load_data():
#    return jsonify({"status": "success", "message": "Mock data loaded"}), 200
@app.route('/loadData', methods=['GET'])
def load_data():
    # CSV-Daten, wie sie vorher waren
    mock_data = """
    IR,1,0x12,0x0000,2,0,0,0,sendHttpToAPI,0,0,0,0,0,0,0
    IR,8,0x6,0x0000,2,0,0,0,sendHttpToAPI,0,0,0,0,0,0,0
    """
    
    # Der SendToApi-Status (True oder False)
    send_to_api_value = True  # Zum Beispiel, True für aktiviert, False für deaktiviert
    
    # Wir fügen den Status als zusätzliche Zeile in der CSV hinzu
    send_to_api_row = f"sendToApi,{send_to_api_value}"

    # Gesamte Antwort: CSV-Daten und der Status als zusätzliche Zeile
    response_data = mock_data + "\n" + send_to_api_row
    
    return response_data, 200, {'Content-Type': 'text/plain'}


@app.route('/sendBLE', methods=['POST'])
def send_ble():
    data = request.json
    return jsonify({"status": "success", "message": "BLE data received", "received": data}), 200

@app.route('/sendIR', methods=['POST'])
def send_ir():
    data = request.json
    return jsonify({"status": "success", "message": "IR data received", "received": data}), 200

# Webseite bereitstellen
@app.route('/')
def index():
    # Render die index.html im templates-Ordner
    return render_template('index.html')

#if __name__ == '__main__':
#    app.run(host='0.0.0.0', port=5000, debug=True)

if __name__ == '__main__':
 app.run(host='0.0.0.0', port=5000, debug=True)
