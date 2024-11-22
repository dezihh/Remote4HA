#ifndef WEBPAGE_H
#define WEBPAGE_H

//const char htmlPage[] PROGMEM = R"rawliteral(
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Remote4HAEddy</title>
</head>

<body>
    <style>
        /* Base Table Styles */
        table { 
            width: 100%; 
            border-collapse: collapse; 
        }
        th, td { 
            border: 1px solid #ddd; 
            padding: 8px; 
            text-align: center; 
        }
        th { 
            background-color: #f4f4f4; 
        }

        /* Input and Button Styles */
        input, select { 
            width: 100%; 
        }
        button { 
            margin: 10px 0; 
            transition: transform 0.3s ease; /* Smooth transition effect */
        }

        button:hover {
            transform: scale(1.05); /* Increases size by 5% */
        }

        .delete-btn { 
            background-color: red; 
            color: white; 
            border: none; 
            padding: 5px 10px; 
            cursor: pointer; 
        }

        /* Log Box Styles */
        #logBox {
            width: 100%;
            height: 150px;
            overflow-y: auto;
            border: 1px solid #ccc;
            background-color: #f9f9f9;
            padding: 10px;
            font-family: monospace;
        }

        /* Default Route Box Styles - Fixed Selector */
        .defRoutBox { 
            border: 2px solid #d1d1d1; /* Corrected selector from .defRouteBox to .defRoutBox */
            background-color: #f0f0f0; /* Lightened background for better visibility */
            padding: 10px;
            margin-bottom: 10px;
            display: inline-block;
            color: #333; /* Darkened text color for readability */
            float: right;
        }

        .routHeader {
            font-weight: bold;
            margin-bottom: 5px;
            color: #333;
        }

        /* Radio Button Styles */
        .radio-buttons {
            display: flex;
            gap: 10px;
        }

        .radio-label {
            font-weight: bold;
            display: flex;
            align-items: center;
        }

        /* Field Container Styles */
        .field-container {
            display: flex;
            flex-wrap: wrap;
            gap: 10px;
            align-items: center;
        }

        .field-container select {
            flex: 0;
            width: auto;
        }

        .field-container input {
            flex: 1;
            min-width: 120px;
        }

        .field-container button {
            flex: 0;
            padding: 5px 10px;
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 3px;
            cursor: pointer;
        }

        .field-container button:hover {
            background-color: #45a049;
        }

        /* Region Styles */
        .region2 {
            text-align: center;
            background-color: #f0f0f0;
            max-height: 200px;
            overflow-y: auto;
        }
    </style>

    <h1>IR / BLE Route Editor</h1>

    <div class="defRoutBox"> 
      <div class="routHeader">Send Data Default to API:</div> 
      <div class="radio-buttons"> 
        <label class="radio-label"> 
          <input type="radio" id="sendToApiTrue" name="sendToApi" value="true"> Enabled </label> 
        <label class="radio-label"> 
          <input type="radio" id="sendToApiFalse" name="sendToApi" value="false"> Disabled 
      </label> </div> </div>
    <div>
        <table id="dataTable">
            <thead>
                <!-- Zusätzliche Zeile für die Überschriften des Eingangs- und Ausgangsbereichs -->
                <tr>
                    <th colspan="8">Eingangsbereich</th>
                    <th colspan="8">Ausgangsbereich</th>
                    <th></th>
                    <!-- Leere Spalte für den Delete-Button -->
                </tr>
                <tr>
                    <th>Source</th>
                    <th>IR-Protocol</th>
                    <th>IR-Code</th>
                    <th>IR-Address</th>
                    <th>IR-Repeat</th>
                    <th>USB-Modifier</th>
                    <th>USB-Command</th>
                    <th>USB-LongPress</th>
                    <th>Route-Action</th>
                    <th>Target Protocol</th>
                    <th>IR-Code</th>
                    <th>IR-Address</th>
                    <th>IR-Repeat</th>
                    <th>BLE-Modifier</th>
                    <th>BLE-Code</th>
                    <th>BLE-Long</th>
                    <th>Delete</th>
                    <!-- Spalte für den Löschen-Button -->
                </tr>
            </thead>
            <tbody>
            </tbody>
        </table>
        <button onclick="addRow()">Add Row</button>
        <button onclick="saveData()">Save Data</button>
    </div>
    <hr>
    <div class="field-container">
        <!-- Modus-Auswahl -->
        <select id="modeSelect" onchange="toggleInputFields()">
            <option value="IR">IR</option>
            <option value="BLE">BLE</option>
        </select>

        <!-- Felder für IR -->
        <div id="irFields" class="field-container">
            <!--<input type="text" id="protocolInput" placeholder="Protocol"> --!>
            <select id="protocolInput">
                <option value="8">NEC</option>
                <option value="5">SONY</option>
                <option value="7">DENON</option>
            </select>
            <input type="text" id="addressInput" placeholder="Address (Hex)">
            <input type="text" id="commandInput" placeholder="Command (Hex)">
            <select id="repeatIR">
                <option value="true">True</option>
                <option value="false">False</option>
            </select>
            <button onclick="sendIRData()">Send</button>
        </div>

        <!-- Felder für BLE -->
        <div id="bleFields" class="field-container" style="display: none;">
            <input type="text" id="modifierInput" placeholder="Modifier (Hex)">
            <input type="text" id="keycodeInput" placeholder="Keycode (Hex)">
            <select id="repeatBLE">
                <option value="true">True</option>
                <option value="false">False</option>
            </select>
            <button onclick="sendBLEData()">Send</button>
        </div>
    </div>
    <hr>
    <div class="region region2" id="logBox"></div>
    <script>

        //var gateway = ws://192.168.10.167/ws;
        //var gateway = `ws://${window.location.hostname}:5000/socket.io/`;
        //var gateway = `ws://${window.location.hostname}/ws`;
        var gateway = `ws://${window.location.hostname}:5000/ws`;
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

        const defaultRow = `
      <tr>
        <td>
          <select onchange="toggleFields(this)">
            <option value="IR">IR</option>
            <option value="USB">USB</option>
          </select>
        </td>
        <td><select><option>NEC</option><option>DENON</option><option>SONY</option></select></td>
        <td><input type="text" value="0x0"></td>
        <td><input type="text" value="0x0000"></td>
        <td><select><option value="1">True</option><option value="0">False</option><option value="2">Always</option></select></td>
        <td><input type="text" value="0x0" disabled></td>
        <td><input type="number" value="0" disabled></td>
        <td><select><option value="1">True</option><option value="0">False</option><option value="2">Always</option></select></td>
        <td>
          <select onchange="toggleRouteFields(this)">
            <option value="sendIR">sendIR</option>
            <option value="sendBLE">sendBLE</option>
            <option value="sendHttpToAPI">sendHttpToAPI</option>
          </select>
        </td>
        <td><select><option>SONY</option><option>DENON</option><option>NEC</option><option>UNKNOWN</option></select></td>
        <td><input type="text" value="0x0"></td>
        <td><input type="text" value="0x0000"></td>
        <td><select><option value="1">True</option><option value="0">False</option></select></td>
        <td><select><option>=2=</option><option>None</option><option>KEY_LEFT_SHIFT</option></select></td>
        <td><input type="text" value="0x0"></td>
        <td><select><option value="1">True</option><option value="0">False</option></select></td>
        <td><button class="delete-btn" onclick="deleteRow(this)">Delete</button></td> <!-- Delete button -->
      </tr>
` ;
    const tableBody = document.querySelector("#dataTable tbody");
// Event-Delegation für Änderungen in der Tabelle
tableBody.addEventListener("change", function (event) {
  const target = event.target;

  //if (target.tagName === "SELECT") {
   // const cellIndex = target.closest("td").cellIndex;
  if (target.tagName === "SELECT") {
    const row = target.closest("tr");
    const cellIndex = target.closest("td").cellIndex;

    // Ausführen, wenn es sich um eine Spalte mit bestimmten Funktionen handelt
    if (cellIndex === 0) {
      toggleFields(target); // IR/USB-Logik
    } else if (cellIndex === 8) {
      toggleRouteFields(target); // Route-Logik
    }
  }
});

function addRow() {
  tableBody.insertAdjacentHTML("beforeend", defaultRow);

  // Neue Zeile hinzufügen, damit die Felder im Ausgangsbereich korrekt gesetzt werden
  const newRow = tableBody.lastElementChild;

  const sourceSelect = newRow.querySelector("td:nth-child(1) select");
  const routeActionSelect = newRow.querySelector("td:nth-child(9) select");

  // Wende `toggleFields` und `toggleRouteFields` auf die neue Zeile an
  if (sourceSelect) {
    sourceSelect.addEventListener("change", function () {
      toggleFields(sourceSelect);
    });
    toggleFields(sourceSelect); // Initiale Anwendung
  }

  if (routeActionSelect) {
    routeActionSelect.addEventListener("change", function () {
      toggleRouteFields(routeActionSelect);
    });
    toggleRouteFields(routeActionSelect); // Initiale Anwendung
  }
}
function checkAllRows() {
  const rows = document.querySelectorAll("#dataTable tbody tr"); // Alle Zeilen im Tabellenkörper

  rows.forEach((row) => {
    const sourceSelect = row.querySelector("td:nth-child(1) select");
    const routeActionSelect = row.querySelector("td:nth-child(9) select");

    if (sourceSelect) {
      toggleFields(sourceSelect); // IR/USB-Logik anwenden
    }
    if (routeActionSelect) {
      toggleRouteFields(routeActionSelect); // Route-Logik anwenden
    }
  });
}

// Diese Funktion liest die Tabelle und erstellt einen CSV-String
function gatherTableData() {
    let dataString = "";

    const rows = document.querySelectorAll("#dataTable tbody tr");
    rows.forEach(row => {
        let rowData = [];
        const cells = row.querySelectorAll("td");

        cells.forEach((cell, index) => {
            if (index < cells.length - 1) { // Das letzte 'td' ist der Löschen-Button, den wir nicht brauchen
                if (cell.querySelector("select")) {
                    rowData.push(cell.querySelector("select").value);  // Wenn es ein Dropdown ist, den Wert nehmen
                } else if (cell.querySelector("input")) {
                    rowData.push(cell.querySelector("input").value);  // Andernfalls den Wert des Eingabefeldes nehmen
                }
            }
        });

        dataString += rowData.join(",") + "\n";  // Die Daten der Zeile zu einem CSV-String zusammenführen
    });

    return dataString.trim();  // Entfernen von überflüssigen Leerzeichen am Ende
}


    // URL-kodierte Daten im Format "data=IR,8,14,0,2,0,0,0,..."
function saveData() {
    // Zuerst die Daten aus der Tabelle sammeln
    const dataString = gatherTableData();

    // Wert von sendToApi auslesen
    const sendToApiElement = document.querySelector('input[name="sendToApi"]');
    if (!sendToApiElement) {
        console.error('Checkbox für sendToApi nicht gefunden!');
        alert('Fehler: Checkbox "Send to API" nicht gefunden.');
        return;
    }
    const sendToApiValue = sendToApiElement.checked ? 'true' : 'false';
    console.log('sendToApiValue:', sendToApiValue);

    // Kombiniere Tabellendaten und den Wert von sendToApi
    const finalDataString = `${dataString},${sendToApiValue}`;  // Füge sendToApiValue am Ende hinzu

    // URL-kodierte Daten im Format "data=IR,8,14,0,2,0,0,0,...,sendToApiValue"
    const urlEncodedData = `data=${encodeURIComponent(finalDataString)}`;

    fetch('/save', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded', // content-type für form-encoded Daten
        },
        body: urlEncodedData // Daten im URL-encoded Format senden
    })
    .then(response => response.text())  // Antworte als Text, nicht als JSON
    .then(result => {
        console.log('Save result:', result);  // Speichere Ergebnis in der Konsole
        alert('Gespeichert: ' + result);  // Anzeige im Browser
    })
    .catch(error => console.error('Error:', error));
}

function gatherTableData() {
    let dataString = "";

    const tableRows = document.querySelectorAll("#dataTable tbody tr");
    tableRows.forEach((row) => {
        const cells = row.querySelectorAll("td");

        const rowData = [];
        cells.forEach((cell, index) => {
            if (cell.querySelector("select")) {
                // Dropdown-Wert
                rowData.push(cell.querySelector("select").value);
            } else if (cell.querySelector("input")) {
                // Textfeld- oder Checkbox-Wert
                rowData.push(cell.querySelector("input").value);
            }
        });

        // Füge die Zeile zum Datenstring hinzu
        dataString += rowData.join(",") + "\n";
    });

    console.log("Gesammelte Tabellendaten:", dataString.trim());
    return dataString.trim();
}

///////////////////////////////////
    function deleteRow(button) {
      // Entferne die Zeile, die den "Löschen"-Button enthält
      const row = button.closest("tr");
      row.remove();
    }
    // Daten vom ESP laden
function loadData() {
  fetch("/loadData") // URL zur API
    .then(response => response.text()) // Wir erwarten eine Textantwort im CSV-Format
    .then(data => {
      const rows = data.split('\n'); // Die Daten nach Zeilen aufteilen
      let sendToApiValue = false; // Standardmäßig auf false setzen

      // Die Zeilen durchgehen und die Daten in die Tabelle einfügen
      rows.forEach((row, rowIndex) => {
        if (row.trim() === "") return; // Leere Zeilen überspringen

        // Prüfen, ob es sich um die letzte Zeile handelt, die den sendToApi-Wert enthält
        if (row.startsWith('sendToApi')) {
          const value = row.split(',')[1]; // Der Wert von sendToApi (True/False)
          sendToApiValue = value === "True"; // Setze den Wert als boolean
          return; // Diese Zeile überspringen, da sie nicht in die Tabelle eingefügt werden soll
        }

        const values = row.split(','); // Jede Zeile nach Kommas aufteilen
        const newRow = document.createElement("tr"); // Neue Tabellenzeile erstellen

        values.forEach((value, index) => {
          const cell = document.createElement("td");

          // Dropdowns für bestimmte Spalten
          if (index === 0) {
            // Source: IR oder BLE
            const select = document.createElement("select");
            const options = ["IR", "BLE"];
            options.forEach(option => {
              const opt = document.createElement("option");
              opt.value = option;
              opt.text = option;
              opt.selected = value === option;
              select.appendChild(opt);
            });
            cell.appendChild(select);
          } else if (index === 1) {
            // IR Repeat: 0, 1, 2 (False, True, Always)
            const select = document.createElement("select");
            const options = [
              { text: "NEC", value: "8" },
              { text: "SONY", value: "1" },
              { text: "DENON", value: "2" },
              { text: "UNKOWN", value: "0" }
            ];

            options.forEach(option => {
              const opt = document.createElement("option");
              opt.value = option.value;
              opt.text = option.text;
              opt.selected = value === option.value;
              select.appendChild(opt);
            });
            cell.appendChild(select);
          } else if (index === 4) {
            // Protocol : 0, 1, 2 (False, True, Both)
            const select = document.createElement("select");
            const options = [
              { text: "False", value: "0" },
              { text: "True", value: "1" },
              { text: "Always", value: "2" }
            ];

            options.forEach(option => {
              const opt = document.createElement("option");
              opt.value = option.value;
              opt.text = option.text;
              opt.selected = value === option.value;
              select.appendChild(opt);
            });

            cell.appendChild(select);
          } else if (index === 7) {
            // Protocol : 0, 1, 2 (False, True, Both)
            const select = document.createElement("select");
            const options = [
              { text: "False", value: "0" },
              { text: "True", value: "1" },
              { text: "Always", value: "2" }
            ];

            options.forEach(option => {
              const opt = document.createElement("option");
              opt.value = option.value;
              opt.text = option.text;
              opt.selected = value === option.value;
              select.appendChild(opt);
            });

            cell.appendChild(select);
          } else if (index === 8) {
            // Route Action: sendIR, sendBLE, sendHttpToAPI
            const select = document.createElement("select");
            const options = ["sendIR", "sendBLE", "sendHttpToAPI"];
            options.forEach(option => {
              const opt = document.createElement("option");
              opt.value = option;
              opt.text = option;
              opt.selected = value === option;
              select.appendChild(opt);
            });

            cell.appendChild(select);
          } else if (index === 9) {
            // IR Repeat: 0, 1, 2 (False, True, Always)
            const select = document.createElement("select");
            const options = [
              { text: "NEC", value: "8" },
              { text: "SONY", value: "1" },
              { text: "DENON", value: "2" },
              { text: "UNKOWN", value: "0" }
            ];

            options.forEach(option => {
              const opt = document.createElement("option");
              opt.value = option.value;
              opt.text = option.text;
              opt.selected = value === option.value;
              select.appendChild(opt);
            });
            cell.appendChild(select);
          } else if (index === 12) {
            // Protocol : 0, 1 (False, True)
            const select = document.createElement("select");
            const options = [
              { text: "False", value: "0" },
              { text: "True", value: "1" }
            ];

            options.forEach(option => {
              const opt = document.createElement("option");
              opt.value = option.value;
              opt.text = option.text;
              opt.selected = value === option.value;
              select.appendChild(opt);
            });

            cell.appendChild(select);
          } else if (index === 15) {
            // Protocol : 0, 1 (False, True)
            const select = document.createElement("select");
            const options = [
              { text: "False", value: "0" },
              { text: "True", value: "1" }
            ];

            options.forEach(option => {
              const opt = document.createElement("option");
              opt.value = option.value;
              opt.text = option.text;
              opt.selected = value === option.value;
              select.appendChild(opt);
            });

            cell.appendChild(select);
          } else {
            // Für andere Felder: Normale Textfelder oder Zahleneingabe
            const input = document.createElement("input");
            input.type = "text"; // Für den Fall von Textwerten
            input.value = value;
            cell.appendChild(input);
          }

          newRow.appendChild(cell);
        });

        // Löschen-Button hinzufügen
        const deleteCell = document.createElement("td");
        const deleteButton = document.createElement("button");
        deleteButton.classList.add("delete-btn");
        deleteButton.textContent = "Delete";
        deleteButton.onclick = function () { deleteRow(deleteButton); };
        deleteCell.appendChild(deleteButton);
        newRow.appendChild(deleteCell);

        document.querySelector("#dataTable tbody").appendChild(newRow);
      });

      // Setze den SendToApi-Status (z. B. Checkbox oder andere Eingabe)
      const sendToApiCheckbox = document.querySelector("input[name='sendToApi']");
      if (sendToApiCheckbox) {
        sendToApiCheckbox.checked = sendToApiValue; // Setze den Status entsprechend
      }
  // **Prüfung nach dem Laden der Zeilen** RAUS?
      checkAllRows(); // Alle Zeilen prüfen

    })
    .catch(error => {
      console.error("Error loading data:", error);
      alert("Failed to load data.");
    });
}


// Löschen einer Zeile
function deleteRow(button) {
  const row = button.closest("tr");
  row.remove();
}

    function toggleFields(select) {
      const row = select.closest("tr");
      const source = select.value;
      const irFields = row.querySelectorAll("td:nth-child(3), td:nth-child(4), td:nth-child(5), td:nth-child(2)");
      const usbFields = row.querySelectorAll("td:nth-child(6), td:nth-child(7), td:nth-child(8)");

      if (source === "IR") {
        // IR-Felder aktivieren und USB-Felder deaktivieren
        irFields.forEach(field => {
          const input = field.querySelector("input, select");
          input.disabled = false;
          if (input.tagName === "SELECT") input.value = "NEC";
        });

        usbFields.forEach(field => {
          const input = field.querySelector("input, select");
          input.disabled = true;
          input.value = "0";
        });

      } else if (source === "USB") {

        // USB-Felder aktivieren und IR-Felder deaktivieren
        irFields.forEach(field => {
          const input = field.querySelector("input, select");
          input.disabled = true;
          input.value = input.tagName === "SELECT" ? "UNKNOWN" : "0";
        });

        usbFields.forEach(field => {
          const input = field.querySelector("input, select");
          input.disabled = false;
        });
      }
    }

    function toggleRouteFields(select) {
      const row = select.closest("tr");
      const action = select.value;
      const irFields = row.querySelectorAll("td:nth-child(10), td:nth-child(11), td:nth-child(12), td:nth-child(13)");
      const bleFields = row.querySelectorAll("td:nth-child(14), td:nth-child(15), td:nth-child(16)");

      if (action === "sendIR") {
        // IR-Felder aktivieren und BLE-Felder deaktivieren
        irFields.forEach(field => {
          const input = field.querySelector("input, select");
          input.disabled = false;
        });

        bleFields.forEach(field => {
          const input = field.querySelector("input, select");
          input.disabled = true;
          input.value = "0";
        });
      } else if (action === "sendBLE") {

        // BLE-Felder aktivieren und IR-Felder deaktivieren
        irFields.forEach(field => {
          const input = field.querySelector("input, select");
          input.disabled = true;
          input.value = "0";
        });
        bleFields.forEach(field => {
          const input = field.querySelector("input, select");
          input.disabled = false;
        });
      } else if (action === "sendHttpToAPI") {

        // Alle Felder deaktivieren und mit "0" belegen
        [...irFields, ...bleFields].forEach(field => {
          const input = field.querySelector("input, select");
          input.disabled = true;
          input.value = "0";
        });
      }
    }

        function toggleInputFields() {
  const mode = document.getElementById('modeSelect').value;
  const irFields = document.getElementById('irFields');
  const bleFields = document.getElementById('bleFields');

  if (mode === 'IR') {
    irFields.style.display = 'flex'; // Zeige IR-Felder
    bleFields.style.display = 'none'; // Verstecke BLE-Felder
  } else {
    irFields.style.display = 'none'; // Verstecke IR-Felder
    bleFields.style.display = 'flex'; // Zeige BLE-Felder
  }
}



function sendIRData() {
  const protocol = document.getElementById('protocolInput').value;
  const address = document.getElementById('addressInput').value;
  const command = document.getElementById('commandInput').value;
  const repeat = document.getElementById('repeatIR').value;

  const data = {
    protocol: protocol,
    address: address,
    command: command,
    repeat: repeat === 'true'
  };

  fetch('/sendIR', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data)
  })
    .then(response => response.json())
    .then(result => {
      console.log('IR Data Sent:', result);
      alert('IR Data sent successfully!');
    })
    .catch(error => {
      console.error('Error sending IR data:', error);
      alert('Error sending IR data.');
    });
}

function sendBLEData() {
  const modifier = document.getElementById('modifierInput').value;
  const keycode = document.getElementById('keycodeInput').value;
  const repeat = document.getElementById('repeatBLE').value;

  const data = {
    modifier: modifier,
    keycode: keycode,
    repeat: repeat === 'true'
  };

  fetch('/sendBLE', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data)
  })
    .then(response => response.json())
    .then(result => {
      console.log('BLE Data Sent:', result);
      alert('BLE Data sent successfully!');
    })
    .catch(error => {
      console.error('Error sending BLE data:', error);
      alert('Error sending BLE data.');
    });
}
        //default Route aktiv?
  function loadToggleRouteFields() {
  fetch("/defaultRouteFields")
    .then(response => response.text())
    .then(value => {
      const radioTrue = document.getElementById("toggleRouteTrue");
      const radioFalse = document.getElementById("toggleRouteFalse");

      if (value === "true") {
        radioTrue.checked = true;
      } else if (value === "false") {
        radioFalse.checked = true;
      } else {
        console.error("Invalid value received:", value);
      }
    })
    .catch(error => {
      console.error("Error loading toggleRouteFields:", error);
    });
}

    // Load data from ESP32 (if needed)
    window.onload = function () {
      loadData();      // Funktion zum Laden der Daten
      toggleInputFields();
      initWebSocket(); // WebSocket-Verbindung starten
};
    </script>
</body>

</html>


)rawliteral";

#endif // WEBPAGE_H
