#ifndef WEBPAGE_H
#define WEBPAGE_H
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
		<link rel="icon" href="data:,">
    <title>Remote4HAEddy</title>
</head>
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
	
	h1 {
	  font-size: 28px;
	  margin-bottom: -25px;
	  color: #262625;
	}
	
	h3 {
	  font-size: 16px;
	  margin-bottom: 0px;
	  color: #262625;
	}
	h4 {
	  font-size: 14px;
	  margin-bottom: 5px;
	  margin-top: -5px;
	  color: #262625;
	}

	.tooltip {
		position: relative;
		display: inline-block;
	}

	.tooltip .tooltiptext {
		visibility: hidden;
		width: 300px;
		max-height: 200px; /* Feste Höhe für das Scrollen */
		overflow-y: auto; /* Ermöglicht das vertikale Scrollen */
		background-color: #555;
		color: #fff;
		text-align: left;
		border-radius: 5px;
		padding: 10px;
		position: absolute;
		z-index: 1;
		top: 0; /* Position the tooltip at the same height as the input field */
		left: 105%; /* Position the tooltip to the right of the input field */
		opacity: 0;
		transition: opacity 0.3s;
	}

	.tooltip:hover .tooltiptext,
	.tooltip .tooltiptext:hover {
		visibility: visible;
		opacity: 1;
	}

	/* Input and Button Styles */
	input, select { 
		width: 100%; 
	}
	button { 
		margin: 10px 0; 
		transition: transform 0.3s ease; /* Smooth transition effect */
		background-color: #232b82; 
		color: white; 
		border: none; 
		padding: 5px 10px; 
		cursor: pointer;
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
	
	.scrollbar::-webkit-scrollbar { 
	   width: 16px; 
	  /* Breite der Scrollbar */
	  } 
	  
	  .scrollbar::-webkit-scrollbar-thumb  { 
		background-color: #20255e; 
		/* Farbe des Scrollbar-Thumbnails */ 
		border-radius: 10px; /* Abgerundete Ecken */ 
		border: 4px solid #fff; /* Innere Farbe */ 
		} 
		
	.scrollbar::-webkit-scrollbar-thumb:hover { 
	  background-color: #555; 
	  /* Farbe beim Hover */ 
	  }

	/* Log Box Styles */
	#logBox {
		width: 95%;
		height: 150px;
		overflow-y: auto;
		border: 1px solid #ccc;
		background-color: #f9f9f9;
		padding: 10px;
		font-family: monospace;
		text-align: right;
		margin: 0 auto;
		
	}

	/* Default Route Box Styles - Fixed Selector */
    .defRoutBox {
        border: 2px solid #d1d1d1;
        background-color: #f0f0f0;
        padding: 10px;
        margin-bottom: 10px;
        display: inline-block;
        color: #333;
        float: right;
    }
    .routHeader {
        font-weight: bold;
        margin-bottom: 5px;
        color: #333;
    }

		.routeField-container {
			display: flex;
			align-items: right;
			gap: 10px;
    }
		
		.routCheckBox {
				display: flex; /* Flexbox für einfache Ausrichtung */
				justify-content: flex-end; /* Elemente am rechten Rand ausrichten */
				align-items: center; /* Vertikale Ausrichtung */
		}

		.routCheckBox label {
				margin-right: 10px; /* Abstand von 10px vor der Checkbox */
				text-align: right; /* Text im Label rechtsbündig ausrichten (optional) */
		}
		
		.field-container {
				display: flex;
				align-items: center;
				gap: 10px;
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

<body>
  <h1>IR / BLE Route Editor</h1>

  <div class="defRoutBox"> 
    <div class="routHeader">Host Parameters:</div> 
    <div class="routeField-container">
        <label for="hostAddressInput">ServerURL:</label>
        <input type="text" id="hostAddressInput" placeholder="Enter Hostaddress">
		</div>
		<div class="routCheckBox">
        <label for="forwardCheckbox">Forward:</label>
        <input type="checkbox" id="forwardCheckbox">
    </div>
  </div>

  <div>
    <table id="dataTable">
      <thead>
        <tr>
          <th colspan="8">Eingangsbereich</th>
          <th colspan="8">Ausgangsbereich</th>
          <th></th>
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
        </tr>
      </thead>
      <tbody></tbody>
    </table>
    <button onclick="addRow()">Add Row</button>
    <button onclick="saveData()">Save Data</button>
  </div>
	<hr>
	<h3> Dircet call (IR / BLE): </h3>
    <div class="field-container">
        <!-- Modus-Auswahl -->
        <select id="modeSelect" onchange="toggleInputFields()">
            <option value="IR">IR</option>
            <option value="BLE">BLE</option>
        </select>

        <!-- Felder für IR -->
				<div id="irFields" class="field-container">
						<select id="protocolInput">
								<!-- Optionen werden dynamisch hinzugefügt -->
						</select>
						<input type="text" id="addressInput" placeholder="Address (Hex)">
						<input type="text" id="commandInput" placeholder="Command (Hex)">
						<select id="repeatIR">
								<option value="true">True</option>
								<option value="false" selected>False</option>
						</select>
						<button onclick="sendIRData()">Send</button>
				</div>

        <!-- Felder für BLE -->
        <div id="bleFields" class="field-container" style="display: none;">
            <select id="BLEprotocolInput">
                <!-- Optionen werden dynamisch hinzugefügt -->
            </select>
            <div class="tooltip">
				<input type="text" id="keycodeInput" placeholder="Hex (0x))">
				<div class="tooltiptext" id="tooltipContent"></div>
				</div>
            <select id="repeatBLE">
                <option value="true">True</option>
                <option value="false" selected>False</option>
            </select>
            <button onclick="sendBLEData()">Send</button>
        </div>
    </div>

  <hr>
  <h4> Messagebox: </h4>
  <div id="logBox"></div>
</body>
  <script>
    // BLE Modifier
    const VALID_KEYS = [
    { value: "0x00", text: "" },
    { value: "0x01", text: "KEY_LEFT_CTRL" },
    { value: "0x02", text: "KEY_LEFT_SHIFT" },
    { value: "0x04", text: "KEY_LEFT_ALT" },
    { value: "0x08", text: "KEY_LEFT_GUI" },
    { value: "0x10", text: "KEY_RIGHT_CTRL" },
    { value: "0x20", text: "KEY_RIGHT_SHIFT" },
    { value: "0x40", text: "KEY_RIGHT_ALT" },
	{ value: "0x80", text: "KEY_RIGHT_ALT" }
	];



    // Zentrale Konstante für verfügbare Protokolle
		const PROTOCOLS = [
			{value: "0x0", text: "UNKNOWN"},
			{value: "0x1", text: "PULSE_WIDTH"},
			{value: "0x2", text: "PULSE_DISTANCE"},
			{value: "0x3", text: "APPLE"},
			{value: "0x4", text: "DENON"},
			{value: "0x5", text: "JVC"},
			{value: "0x6", text: "LG"},
			{value: "0x7", text: "LG2"},
			{value: "0x8", text: "NEC"},
			{value: "0x9", text: "NEC2"},
			{value: "0xA", text: "ONKYO"},
			{value: "0xB", text: "PANASONIC"},
			{value: "0xC", text: "KASEIKYO"},
			{value: "0xD", text: "KASEIKYO_DENON"},
			{value: "0xE", text: "KASEIKYO_SHARP"},
			{value: "0xF", text: "KASEIKYO_JVC"},
			{value: "0x10", text: "KASEIKYO_MITSUBISHI"},
			{value: "0x11", text: "RC5"},
			{value: "0x12", text: "RC6"},
			{value: "0x13", text: "SAMSUNG"},
			{value: "0x14", text: "SAMSUNGLG"},
			{value: "0x15", text: "SAMSUNG48"},
			{value: "0x16", text: "SHARP"},
			{value: "0x17", text: "SONY"},
			{value: "0x18", text: "BANG_OLUFSEN"},
			{value: "0x19", text: "BOSEWAVE"},
			{value: "0x1A", text: "LEGO_PF"},
			{value: "0x1B", text: "MAGIQUEST"},
			{value: "0x1C", text: "WHYNTER"},
			{value: "0x1D", text: "FAST"}
		];

		const PROT_DEFAULT = "0"; // Standardwert für Protokolle
		const MOD_DEFAULT="";
		const BOOLS = [ { value: "0", text: "False" }, { value: "1", text: "True" } ];
		const MAYBE = [ { value: "0", text: "False" }, { value: "1", text: "True" }, { value: "2", text: "Beides" } ];
		        const hidUsageCodes = [
            "A=0x04", "B=0x05", "C=0x06", "D=0x07", "E=0x08", "F=0x09", "G=0x0A", "H=0x0B", "I=0x0C", "J=0x0D",
            "K=0x0E", "L=0x0F", "M=0x10", "N=0x11", "O=0x12", "P=0x13", "Q=0x14", "R=0x15", "S=0x16", "T=0x17",
            "U=0x18", "V=0x19", "W=0x1A", "X=0x1B", "Y=0x1C", "Z=0x1D", "1=0x1E", "2=0x1F", "3=0x20", "4=0x21",
            "5=0x22", "6=0x23", "7=0x24", "8=0x25", "9=0x26", "0=0x27", "Enter=0x28", "Escape=0x29", "Backspace=0x2A",
            "Tab=0x2B", "Space=0x2C", "-=0x2D", "=0x2E", "[=0x2F", "]=0x30", "\\=0x31", "#=0x32", ";=0x33", "'=0x34",
            "`=0x35", ",=0x36", ".=0x37", "/=0x38", "Caps Lock=0x39", "F1=0x3A", "F2=0x3B", "F3=0x3C", "F4=0x3D",
            "F5=0x3E", "F6=0x3F", "F7=0x40", "F8=0x41", "F9=0x42", "F10=0x43", "F11=0x44", "F12=0x45", "Print Screen=0x46",
            "Scroll Lock=0x47", "Pause=0x48", "Insert=0x49", "Home=0x4A", "Page Up=0x4B", "Delete=0x4C", "End=0x4D",
            "Page Down=0x4E", "Right Arrow=0x4F", "Left Arrow=0x50", "Down Arrow=0x51", "Up Arrow=0x52", "Num Lock=0x53",
            "Keypad / =0x54", "Keypad * =0x55", "Keypad - =0x56", "Keypad + =0x57", "Keypad Enter=0x58", "Keypad 1=0x59",
            "Keypad 2=0x5A", "Keypad 3=0x5B", "Keypad 4=0x5C", "Keypad 5=0x5D", "Keypad 6=0x5E", "Keypad 7=0x5F",
            "Keypad 8=0x60", "Keypad 9=0x61", "Keypad 0=0x62", "Keypad .=0x63", "Left Control=0xE0", "Left Shift=0xE1",
            "Left Alt=0xE2", "Left GUI=0xE3", "Right Control=0xE4", "Right Shift=0xE5", "Right Alt=0xE6", "Right GUI=0xE7",
            "Media Play=0xE8", "Media Pause=0xE9", "Media Record=0xEA", "Media Fast Forward=0xEB", "Media Rewind=0xEC",
            "Media Next Track=0xED", "Media Previous Track=0xEE", "Media Stop=0xEF", "Media Play/Pause=0xF0", "Media Mute=0xF1",
            "Media Volume Up=0xF2", "Media Volume Down=0xF3", "(=0x26", ")=0x27", "{=0x2F", "}=0x30"
        ];

        document.addEventListener("DOMContentLoaded", function() {
            const tooltipContent = document.getElementById("tooltipContent");
            tooltipContent.innerHTML = hidUsageCodes.join("<br>");

	 });// Tooltip Ende
	 
		// SSE beginns
		const eventSource = new EventSource("/events");
		const logElement = document.getElementById("logBox");

		eventSource.onmessage = function(event) {
				logElement.innerHTML += event.data + "<br>";
				logElement.scrollTop = logElement.scrollHeight;
		};

		eventSource.onerror = function() {
				logElement.innerHTML += "⚠️ Connection lost. Retrying...<br>";
		};

		eventSource.addEventListener("log", function(event) {
			logElement.innerHTML += "Log: " + event.data + "<br>";
			logElement.scrollTop = logElement.scrollHeight;
		});
			
			// Für IR Send Protocolle aufschlüsseln
			const protocolSelect = document.getElementById("protocolInput");
			PROTOCOLS.forEach(protocol => {
					const option = document.createElement("option");
					option.value = protocol.value; // Zahlenwert, der gesendet wird
					option.textContent = protocol.text; // Text, der angezeigt wird
					protocolSelect.appendChild(option);
			});
			
			const BLEprotocolSelect = document.getElementById("BLEprotocolInput");
			VALID_KEYS.forEach(protocol => {
					const option = document.createElement("option");
					option.value = protocol.value; // Zahlenwert, der gesendet wird
					option.textContent = protocol.text; // Text, der angezeigt wird
					BLEprotocolSelect.appendChild(option);
			});
				

	// Funktion: Neue Zeile hinzufügen
	function addRow(rowData = null) {
			const tableBody = document.querySelector("#dataTable tbody");
			const newRow = document.createElement("tr");

			// Wenn Daten existieren, diese verwenden, sonst Standardwerte setzen
			const defaultValues = rowData ? rowData.split(",") : [];
			newRow.innerHTML = `
				<td>
					<select onchange="toggleFields(this)">
						<option value="IR" ${defaultValues[0] === "IR" ? "selected" : ""}>IR</option>
						<option value="USB" ${defaultValues[0] === "USB" ? "selected" : ""}>USB</option>
					</select>
				</td>
				<td>${createProtocolDropdown(defaultValues[1])}</td>
				<td><input type="text" value="${defaultValues[2] || "0x00"}"></td>
				<td><input type="text" value="${defaultValues[3] || "0x0000"}"></td>
				<td>${createMaybeDropdown(defaultValues[4])}</td>
				<td><input type="text" value="${defaultValues[5] || "0x00"}" disabled></td>
				<td><input type="text" value="${defaultValues[6] || "0x00"}" disabled></td>
				<td>${createMaybeDropdown(defaultValues[7])}</td>
				<td>
					<select onchange="toggleRouteFields(this)">
						<option value="sendIR" ${defaultValues[8] === "sendIR" ? "selected" : ""}>sendIR</option>
						<option value="sendBLE" ${defaultValues[8] === "sendBLE" ? "selected" : ""}>sendBLE</option>
						<option value="sendHttpToAPI" ${defaultValues[8] === "sendHttpToAPI" ? "selected" : ""}>sendHttpToAPI</option>
					</select>
				</td>
				<td>${createProtocolDropdown(defaultValues[9])}</td>
				<td><input type="text" value="${defaultValues[10] || "0x00"}"></td>
				<td><input type="text" value="${defaultValues[11] || "0x0000"}"></td>
				<td>${createBooleanDropdown(defaultValues[12] === "false")}</td>
				<td>${createModifierDropdown(defaultValues[13])}</td>
				<td><input type="text" value="${defaultValues[14] || ""}"></td>
				<td>${createBooleanDropdown(defaultValues[15] === "false")}</td>
				<td><button class="delete-btn" onclick="deleteRow(this)">Delete</button></td>	`;
			tableBody.appendChild(newRow);
			toggleFields(newRow.querySelector("select"));
			toggleRouteFields(newRow.querySelector("select:nth-of-type(1)"));
		}

	// Select Feld PRotocol Text und Wert mappen 
	function createProtocolDropdown(selectedValue = "default") { 
		const isSelectedValueValid = PROTOCOLS.some(option => option.value === selectedValue); 
		const valueToUse = isSelectedValueValid ? selectedValue : PROT_DEFAULT; 
		return ` 
			<select> ${PROTOCOLS.map(option => ` 
				<option value="${option.value}" ${option.value === valueToUse ? "selected" : ""}> 
					${option.text} 
				</option> `).join("")} 
			</select> 
			`;
		}
		
		function createModifierDropdown(selectedValue = "default") { 
			const isSelectedValueValid = VALID_KEYS.some(option => option.value === selectedValue); 
			const valueToUse = isSelectedValueValid ? selectedValue : MOD_DEFAULT; 
			return ` 
				<select> ${VALID_KEYS.map(option => ` 
					<option value="${option.value}" ${option.value === valueToUse ? "selected" : ""}> 
						${option.text} 
					</option> `).join("")} 
				</select> 
				`;
		}

	//Select Feld Bool zu Zahl mappen 
	function createBooleanDropdown(selectedValue = "0") { 
		return ` 
			<select> 
				${BOOLS.map(option => ` <option value="${option.value}" ${option.value === selectedValue ? "selected" : ""}> ${option.text} </option> `).join("")} 
			</select>
		`; 
	}

	//Select Feld Bool zu Zahl mappen + beides (2)
	function createMaybeDropdown(selectedValue = "") { 
		return ` 
			<select> ${MAYBE.map(option => ` 
				<option value="${option.value}" ${option.value === selectedValue ? "selected" : ""}> ${option.text} 
				</option> `).join("")} 
			</select> 
			`; 
		}

    // Felder basierend auf Quelle (IR/USB) umschalten
    function toggleFields(select) {
      const row = select.closest("tr");
      const isIR = select.value === "IR";

      ["IR-Protocol", "IR-Code", "IR-Address", "IR-Repeat"].forEach((_, i) => {
        toggleField(row.cells[i + 1], isIR);
      });

      ["USB-Modifier", "USB-Command", "USB-LongPress"].forEach((_, i) => {
        toggleField(row.cells[i + 5], !isIR);
      });
    }
		
	// Abhängig von Route-Action Felder für Zielprotokolle deaktivieren
	function toggleRouteFields(select) {
		const row = select.closest("tr");
		if (!row) return;
		const action = row.querySelectorAll("select")[4].value.trim();
		const isIR = action === "sendIR";
		const isBLE = action === "sendBLE";

		["Target Protocol", "IR-Code", "IR-Address", "IR-Repeat"].forEach((_, i) => {
			toggleField(row.cells[i + 9], isIR);
		});

		["BLE-Modifier", "BLE-Code", "BLE-Long"].forEach((_, i) => {
			toggleField(row.cells[i + 13], isBLE);
		});
	}

	// Routendaten vom ESP laden
async function loadData() {
    try {
        const response = await fetch("/loadData");
        if (!response.ok) {
            throw new Error(`HTTP error! Status: ${response.status}`);
        }

        const data = await response.text();
        console.log("Received data:", data); // Debug-Ausgabe

        const rows = data
            .trim()
            .split("\n")
            .map(row => row.trim())
            .filter(row => row !== "");

        rows.forEach(row => {
            if (row.startsWith("APIHost=")) {
                const hostAddress = row.substring("APIHost=".length);
                console.log("Parsed hostAddress:", hostAddress); // Debug-Ausgabe
                document.getElementById("hostAddressInput").value = hostAddress;
            } else if (row.startsWith("sendToApi=")) {
                const forward = row.substring("sendToApi=".length) === "true";
                console.log("Parsed forward:", forward); // Debug-Ausgabe
                document.getElementById("forwardCheckbox").checked = forward;
            } else {
                addRow(row);
            }
        });

        // Aktualisiere die Felder, nachdem alle Zeilen geladen sind
        updateAllRows();

        logMessage("Data loaded successfully!");
    } catch (error) {
        logMessage(`Error loading data: ${error.message}`);
    }
}
	/////////////////////////////////
    // Unterfunktion für Felddeaktivierung (aktivieren/deaktivieren)
    function toggleField(cell, enable) {
      const input = cell.querySelector("input, select");
      if (input) {
        input.disabled = !enable;
        if (!enable) input.value = input.tagName === "SELECT" ? "UNKNOWN" : "0x0";
      }
    }

    // Zeile löschen
    function deleteRow(button) {
      button.closest("tr").remove();
    }
		

	// Daten auf ESP speichern
function saveData() {
    const rows = Array.from(document.querySelectorAll("#dataTable tbody tr"));

    // Generiere die Datenzeilen und entferne das letzte Komma, falls vorhanden
    const data = rows.map(row => {
        return Array.from(row.querySelectorAll("input, select"))
            .map(input => input.value)
            .join(",")
            .replace(/,$/, ""); // Entferne das letzte Komma
    });

    // Status der FoewardCheckbox auslesen
	const sendToApiEnabled = document.getElementById("forwardCheckbox").checked;
    const sendToApiRow = `sendToApi,${sendToApiEnabled}`; // Formatiere die Zeile
		// Füge den Status als vorletzte Zeile hinzu
    data.push(sendToApiRow);
		
		// API Host
	const APIHost = document.getElementById("hostAddressInput").value
    const sendApiHost = `APIHost,${APIHost}`; // Formatiere die Zeile
		// Füge den Status als letzte Zeile hinzu
    data.push(sendApiHost);

    // Verbinde alle Zeilen zu einem einzigen String mit Zeilenumbrüchen
    const formattedData = data.join("\n");

    console.log("Saved Data:", formattedData);

    // Sende die Daten an den Server
    fetch("/save", {
        method: "POST", // Verwende POST-Methode
        headers: {
            "Content-Type": "text/plain", // Setze den Content-Type
        },
        body: formattedData, // Alle Zeilen als eine Payload
    })
        .then(response => {
            if (!response.ok) {
                throw new Error(`HTTP error! Status: ${response.status}`);
            }
            return response.text(); // Optional: Verarbeite die Serverantwort
        })
        .then(result => {
            console.log("Server Response:", result);
            logMessage("Data saved successfully!");
        })
        .catch(error => {
            console.error("Error saving data:", error);
            logMessage(`Error saving data: ${error.message}`);
        });
}


	// Für Direct Call Felder aktivieren
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

////////////////////////////////////
function saveData() {
    const rows = Array.from(document.querySelectorAll("#dataTable tbody tr"));

    // Generiere die Datenzeilen und entferne das letzte Komma, falls vorhanden
    const data = rows.map(row => {
        return Array.from(row.querySelectorAll("input, select"))
            .map(input => input.value)
            .join(",")
            .replace(/,$/, ""); // Entferne das letzte Komma
    });

    // Status der ForwardCheckbox auslesen
    const sendToApiEnabled = document.getElementById("forwardCheckbox").checked ? "true" : "false";
    const sendToApiRow = `sendToApi=${sendToApiEnabled}`; // Formatiere die Zeile

    // API Host
    const APIHost = document.getElementById("hostAddressInput").value;
    const sendApiHost = `APIHost=${APIHost}`; // Formatiere die Zeile

    // Füge den Status und API Host als letzte Zeilen hinzu
    data.push(sendToApiRow);
    data.push(sendApiHost);

    // Verbinde alle Zeilen zu einem einzigen String mit Zeilenumbrüchen
    const formattedData = data.join("\n");

    // Füge "data=" vor den Nutzdaten hinzu
    const payload = `data=${formattedData}`;

    console.log("Saved Data:", payload);

    // Sende die Daten an den Server
    fetch("/save", {
        method: "POST", // Verwende POST-Methode
        headers: {
            "Content-Type": "application/x-www-form-urlencoded", // Setze den Content-Type
        },
        body: payload, // Alle Zeilen als eine Payload
    })
        .then(response => {
            if (!response.ok) {
                throw new Error(`HTTP error! Status: ${response.status}`);
            }
            return response.text(); // Optional: Verarbeite die Serverantwort
        })
        .then(result => {
            console.log("Server Response:", result);
            logMessage("Data saved successfully!");
        })
        .catch(error => {
            console.error("Error saving data:", error);
            logMessage(`Error saving data: ${error.message}`);
        });
}

function logMessage(message) {
    console.log(message);
}

// Direktaufruf für BLE
function sendBLEData() {
    logMessage("Direct Call (BLE) initiated...");

    // Werte aus den Eingabefeldern abrufen
    const modifier = document.getElementById('BLEprotocolInput').value;
    let keycode = document.getElementById('keycodeInput').value;
    const repeat = document.getElementById('repeatBLE').value;

    // Konvertiere keycode zu Kleinbuchstaben oder setze "None" wenn leer
    keycode = keycode ? keycode.toLowerCase() : "None";

    // Überprüfen, ob alle Felder ausgefüllt sind
    if (!modifier || !keycode || !repeat) {
        alert('Bitte füllen Sie alle Felder aus!');
        return; // Verhindert das Senden der Anfrage, falls ein Feld fehlt
    }

    // URL-Parameter erstellen
    const params = new URLSearchParams({
        modifier: modifier,
        keycode: keycode,
        isRepeat: repeat === 'true' ? 'true' : 'false' // Boolean in String umwandeln
    });

    // Fetch-Aufruf mit URL-Parametern
    fetch('/sendBLE', {
        method: 'POST', // Methode muss POST sein
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded'
        },
        body: params.toString() // URL-kodierte Formulardaten
    })
    .then(response => {
        if (!response.ok) {
            throw new Error('HTTP error! status: ' + response.status);
        }
        return response.text(); // Text-Antwort des ESP lesen
    })
    .then(result => {
        console.log('BLE Data Sent:', result);
    })
    .catch(error => {
        console.error('Error sending BLE data:', error);
        alert('Fehler beim Senden des BLE-Befehls.');
    });
}
//////////////////////////////////////////////////////////////////////////////
	// Direktaufruf für IR
  function sendIRData() {
		logMessage("Direct Call (IR) initiated...");

		// Werte aus den Eingabefeldern holen
		const protocol = document.getElementById('protocolInput').value;
		const address = document.getElementById('addressInput').value;
		const command = document.getElementById('commandInput').value;
		const repeat = document.getElementById('repeatIR').checked;

		// Überprüfen, ob alle Felder ausgefüllt sind
		if (!protocol || !address || !command) {
			alert('Bitte füllen Sie alle Felder aus!');
			return; // Verhindert das Senden der Anfrage, falls ein Feld fehlt
		}

		// URL-Parameter erstellen
		const params = new URLSearchParams({
			protocol: protocol,
			address: address,
			command: command,
			repeats: repeat ? 'true' : 'false' // Boolean in String umwandeln
		});
		

		// Fetch-Aufruf mit URL-Parametern
		fetch('/sendIR', {
			method: 'POST', // Methode muss POST sein
			headers: {
				'Content-Type': 'application/x-www-form-urlencoded'
			},
			body: params.toString() // URL-kodierte Formulardaten
		})
		.then(response => {
			if (!response.ok) {
				throw new Error('HTTP error! status: ' + response.status);
			}
			return response.text(); // Text-Antwort des ESP lesen
		})
		.then(result => {
			console.log('IR Data Sent:', result);
		})
		.catch(error => {
			console.error('Error sending IR data:', error);
			alert('Fehler beim Senden des IR-Befehls.');
		});
	}

	function logMessage(message) {
		console.log(message);
	}
	
    // Direktaufruf für BLE
    function sendBLEData() {
    logMessage("Direct Call (BLE) initiated...");

    // Werte aus den Eingabefeldern abrufen
    const modifier = document.getElementById('BLEprotocolInput').value;
    let keycode = document.getElementById('keycodeInput').value;
    const repeat = document.getElementById('repeatBLE').value;

	// Konvertiere keycode zu Kleinbuchstaben oder setze "None" wenn leer
    keycode = keycode ? keycode.toLowerCase() : "None";
		
    // Überprüfen, ob alle Felder ausgefüllt sind
    if (!modifier || !keycode || !repeat) {
        alert('Bitte füllen Sie alle Felder aus!');
        return; // Verhindert das Senden der Anfrage, falls ein Feld fehlt
    }

    // URL-Parameter erstellen
    const params = new URLSearchParams({
        modifier: modifier,
        keycode: keycode,
        isRepeat: repeat === 'true' ? 'true' : 'false' // Boolean in String umwandeln
    });
	// Fetch-Aufruf mit URL-Parametern
	fetch('/sendBLE', {
		method: 'POST', // Methode muss POST sein
		headers: {
			'Content-Type': 'application/x-www-form-urlencoded'
		},
		body: params.toString() // URL-kodierte Formulardaten
	})
	.then(response => {
		if (!response.ok) {
			throw new Error('HTTP error! status: ' + response.status);
		}
		return response.text(); // Text-Antwort des ESP lesen
	})
	.then(result => {
		console.log('BLE Data Sent:', result);
	})
	.catch(error => {
		console.error('Error sending BLE data:', error);
		alert('Fehler beim Senden des BLE-Befehls.');
	});
}
		
    // Funktion: Log-Nachricht hinzufügen
    function logMessage(message) {
      const logBox = document.getElementById("logBox");
      //logBox.innerHTML += "local: "+ message + "<br>";
      logBox.scrollTop = logBox.scrollHeight;
    }

	// Funktion: Zeile mit Initialdaten hinzufügen
	function addRowWithData(values) {
		const tableBody = document.querySelector("#dataTable tbody");
		const newRow = document.createElement("tr");

		// Zeile mit den übergebenen Werten erstellen
		newRow.innerHTML = `
			<td>
				<select onchange="toggleFields(this)">
					<option value="IR" ${values[0] === "IR" ? "selected" : ""}>IR</option>
					<option value="USB" ${values[0] === "USB" ? "selected" : ""}>USB</option>
				</select>
			</td>
			<td>${createProtocolDropdown(values[1])}</td>
			<td><input type="text" value="${values[2]}"></td>
			<td><input type="text" value="${values[3]}"></td>
			<td>${createMaybeDropdown(values[4])}</td>
			<td><input type="text" value="${values[5]}" ${values[0] === "IR" ? "disabled" : ""}></td>
			<td><input type="text" value="${values[6]}" ${values[0] === "IR" ? "disabled" : ""}></td>
			<td>${createMaybeDropdown(values[7])}</td>
			<td>
				<select onchange="toggleRouteFields(this)">
					<option value="sendIR" ${values[8] === "sendIR" ? "selected" : ""}>sendIR</option>
					<option value="sendBLE" ${values[8] === "sendBLE" ? "selected" : ""}>sendBLE</option>
					<option value="sendHttpToAPI" ${values[8] === "sendHttpToAPI" ? "selected" : ""}>sendHttpToAPI</option>
				</select>
			</td>
			<td>${createProtocolDropdown(values[9])}</td>
			<td><input type="text" value="${values[10]}"></td>
			<td><input type="text" value="${values[11]}"></td>
			<td>${createBooleanDropdown(values[12])}</td>
			<td>${createModifierDropdown(values[13])}</td>
			<td><input type="text" value="${values[14]}"></td>
			<td>${createBooleanDropdown(values[15])}</td>
			<td><button class="delete-btn" onclick="deleteRow(this)">Delete</button></td>
		`;

		tableBody.appendChild(newRow);

	}

	// Daten von /loadData laden

    // Initialisierung
    window.onload = function () {
        toggleInputFields();
        loadData();
    };
  </script>
	</body>
</html>)rawliteral";

#endif // WEBPAGE_H
