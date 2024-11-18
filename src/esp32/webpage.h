#ifndef WEBPAGE_H
#define WEBPAGE_H

//const char htmlPage[] PROGMEM = R"rawliteral(
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Table Editor</title>
  <style>
    table { width: 100%; border-collapse: collapse; }
    th, td { border: 1px solid #ddd; padding: 8px; text-align: center; }
    th { background-color: #f4f4f4; }
    input, select { width: 100%; }
    button { margin: 10px 0; }
    .delete-btn { background-color: red; color: white; border: none; padding: 5px 10px; cursor: pointer; }
  </style>
</head>
<body>
<style>
    .region2 {
       // grid-column: 2 / 3;
       // grid-row: 1 / 2;
        text-align: center;
        background-color: #f0f0f0;
        max-height: 200px;
        overflow-y: auto;
    }
</style>
  <h1>ESP32 Table Editor</h1>
  <div style="border: 1px solid #000; padding: 10px; margin-bottom: 10px; display: inline-block;">
    <b>Send Data Default to API:</b><br>
    <label style="display: inline-block; margin-right: 10px;">
      <input type="radio" id="sendToApiTrue" name="sendToApi" value="true"> Enabled
    </label>
    <label style="display: inline-block;">
      <input type="radio" id="sendToApiFalse" name="sendToApi" value="false"> Disabled
    </label>
  </div>
  <div>
  <table id="dataTable">
    <thead>
      <!-- Zusätzliche Zeile für die Überschriften des Eingangs- und Ausgangsbereichs -->
      <tr>
        <th colspan="8">Eingangsbereich</th>
        <th colspan="8">Ausgangsbereich</th>
        <th></th> <!-- Leere Spalte für den Delete-Button -->
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
        <th>Delete</th> <!-- Spalte für den Löschen-Button -->
      </tr>
    </thead>
    <tbody>
    </tbody>
  </table>
  <button onclick="addRow()">Add Row</button>
  <button onclick="saveData()">Save Data</button>
  </div>
    <div class="region region2" id="logBox"></div>

  <script>
        var gateway = `ws://${window.location.hostname}/ws`;
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
    `;
    const tableBody = document.querySelector("#dataTable tbody");

    function addRow() {
      tableBody.insertAdjacentHTML("beforeend", defaultRow);
      // Neue Zeile hinzufügen, damit die Felder im Ausgangsbereich korrekt gesetzt werden
      const newRow = tableBody.lastElementChild;
      const routeActionSelect = newRow.querySelector('td:nth-child(9) select');
      toggleRouteFields(routeActionSelect);
    }

    function saveData() {
      const rows = tableBody.querySelectorAll("tr");
      const data = Array.from(rows).map(row => {
        return Array.from(row.querySelectorAll("input, select")).map(input => input.value);
      });
      const sendToApi = document.querySelector('input[name="sendToApi"]:checked').value;
      console.log(JSON.stringify(data));
      console.log('SendToApi:', sendToApi);
      fetch("/save", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ rows: data, sendToApi }),
      }).then(response => {
        if (response.ok) alert("Data saved!");
      });
    }

    function deleteRow(button) {
      // Entferne die Zeile, die den "Löschen"-Button enthält
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

    // Load data from ESP32 (if needed)
    fetch("/data").then(response => response.json()).then(data => {
      data.forEach(row => {
        const { rows, sendToApi } = data;

        rows.forEach(row => {
          const newRow = document.createElement("tr"); // Neue Tabellenzeile erstellen

          row.forEach((value, index) => {
            const cell = document.createElement("td");
            if (index === 0 || index === 1 || index === 8 || index === 9 || index === 13) {
              // Dropdowns
              const select = document.createElement("select");
              const options = index === 0 ? ["IR", "USB"] :
                              index === 1 ? ["NEC", "DENON", "SONY"] :
                              index === 8 ? ["sendIR", "sendBLE", "sendHttpToAPI"] :
                              index === 9 ? ["SONY", "DENON", "NEC", "UNKNOWN"] :
                              ["=2=", "KEY_LEFT_SHIFT"];
              options.forEach(option => {
                const opt = document.createElement("option");
                opt.value = option;
                opt.text = option;
                opt.selected = value === option;
                select.appendChild(opt);
              });
              cell.appendChild(select);
            } else if (index === 4 || index === 7 || index === 12 || index === 15) {
              // Spezielle Dropdowns für True, False, Always
              const select = document.createElement("select");
              const options = [
                { text: "True", value: "1" },
                { text: "False", value: "0" },
                { text: "Always", value: "2" }
              ];
              options.forEach(option => {
                const opt = document.createElement("option");
                opt.value = option.value;
                opt.text = option.text;
                opt.selected = value == option.value;
                select.appendChild(opt);
              });
              cell.appendChild(select);
            } else {
              // Input fields
              const input = document.createElement("input");
              input.type = ["2", "3"].includes(index) ? "text" : "number";
              input.value = value;
              cell.appendChild(input);
            }
            newRow.appendChild(cell);
          });

          // Füge einen Löschen-Button hinzu
          const deleteCell = document.createElement("td");
          const deleteButton = document.createElement("button");
          deleteButton.classList.add("delete-btn");
          deleteButton.textContent = "Delete";
          deleteButton.onclick = function () { deleteRow(deleteButton); };
          deleteCell.appendChild(deleteButton);
          newRow.appendChild(deleteCell);

          tableBody.appendChild(newRow);

          // Set radio button for "Send Data Default to API"
          document.querySelector(`#sendToApi${sendToApi === "true" ? "True" : "False"}`).checked = true;
        });
      });
    });
  </script>
</body>
</html>
)rawliteral";

#endif // WEBPAGE_H
