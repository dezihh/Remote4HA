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
    <div class="routHeader">Send Data Default to API:</div> 
    <div class="routeField-container">
        <label for="hostAddressInput">Hostaddress:</label>
        <input type="text" id="hostAddressInput" placeholder="Enter Hostaddress">
		</div>
		<div class="routCheckBox">
        <label for="forwardCheckbox">Forward</label>
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
            <input type="text" id="keycodeInput" placeholder="Keycode (Hex)">
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
      { value: "0x80", text: "KEY_LEFT_CTRL" },
	    { value: "0x81", text: "KEY_LEFT_SHIFT" },
	    { value: "0x82", text: "KEY_LEFT_ALT" },
	    { value: "0x83", text: "KEY_LEFT_GUI" },
	    { value: "0x84", text: "KEY_RIGHT_CTRL" },
	    { value: "0x85", text: "KEY_RIGHT_SHIFT" },
	    { value: "0x86", text: "KEY_RIGHT_ALT" },
	    { value: "0x87", text: "KEY_RIGHT_GUI" },
	    { value: "0xDA", text: "KEY_UP_ARROW" },
	    { value: "0xD9", text: "KEY_DOWN_ARROW" },
	    { value: "0xD8", text: "KEY_LEFT_ARROW" },
	    { value: "0xD7", text: "KEY_RIGHT_ARROW" },
	    { value: "0xC2", text: "KEY_F1" },
	    { value: "0xC3", text: "KEY_F2" },
	    { value: "0xC4", text: "KEY_F3" },
	    { value: "0xC5", text: "KEY_F4" },
	    { value: "0xC6", text: "KEY_F5" },
	    { value: "0xC7", text: "KEY_F6" },
	    { value: "0xC8", text: "KEY_F7" },
	    { value: "0xC9", text: "KEY_F8" },
	    { value: "0xCA", text: "KEY_F9" },
	    { value: "0xCB", text: "KEY_F10" },
	    { value: "0xCC", text: "KEY_F11" },
	    { value: "0xCD", text: "KEY_F12" },
	    { value: "0xCE", text: "KEY_F13" },
	    { value: "0xCF", text: "KEY_F14" },
	    { value: "0xD0", text: "KEY_F15" },
	    { value: "0xD1", text: "KEY_F16" },
	    { value: "0xD2", text: "KEY_F17" },
	    { value: "0xD3", text: "KEY_F18" },
	    { value: "0xD4", text: "KEY_F19" },
	    { value: "0xD5", text: "KEY_F20" },
	    { value: "0xD6", text: "KEY_F21" },
	    { value: "0xD7", text: "KEY_F22" },
	    { value: "0xD8", text: "KEY_F23" },
	    { value: "0xD9", text: "KEY_F24" },
	    { value: "0xEA", text: "KEY_NUM_0" },
	    { value: "0xE1", text: "KEY_NUM_1" },
	    { value: "0xE2", text: "KEY_NUM_2" },
	    { value: "0xE3", text: "KEY_NUM_3" },
	    { value: "0xE4", text: "KEY_NUM_4" },
	    { value: "0xE5", text: "KEY_NUM_5" },
	    { value: "0xE6", text: "KEY_NUM_6" },
	    { value: "0xE7", text: "KEY_NUM_7" },
	    { value: "0xE8", text: "KEY_NUM_8" },
	    { value: "0xE9", text: "KEY_NUM_9" },
	    { value: "0xDC", text: "KEY_NUM_SLASH" },
	    { value: "0xDD", text: "KEY_NUM_ASTERISK" },
	    { value: "0xDE", text: "KEY_NUM_MINUS" },
	    { value: "0xDF", text: "KEY_NUM_PLUS" },
	    { value: "0xE0", text: "KEY_NUM_ENTER" },
	    { value: "0xEB", text: "KEY_NUM_PERIOD" },
	    { value: "0xB2", text: "KEY_BACKSPACE" },
	    { value: "0xB3", text: "KEY_TAB" },
	    { value: "0xB0", text: "KEY_RETURN" },
	    { value: "0xB1", text: "KEY_ESC" },
	    { value: "0xD1", text: "KEY_INSERT" },
	    { value: "0xCE", text: "KEY_PRTSC" },
	    { value: "0xD4", text: "KEY_DELETE" },
	    { value: "0xD3", text: "KEY_PAGE_UP" },
	    { value: "0xD6", text: "KEY_PAGE_DOWN" },
	    { value: "0xD2", text: "KEY_HOME" },
	    { value: "0xD5", text: "KEY_END" },
	    { value: "0xC1", text: "KEY_CAPS_LOCK" }
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
		const BOOLS = [ { value: "0", text: "False" }, { value: "1", text: "True" } ];
		const MAYBE = [ { value: "0", text: "False" }, { value: "1", text: "True" }, { value: "2", text: "Beides" } ];

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
				<td><input type="text" value="${defaultValues[13] || "None"}"></td>
				<td><input type="text" value="${defaultValues[14] || "0x00"}"></td>
				<td>${createBooleanDropdown(defaultValues[15] === "true")}</td>
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
			const rows = data
				.trim()
				.split("\n")
				.map(row => row.trim())
				.filter(row => row !== "");

			rows.forEach(row => {
				addRow(row);
			});

			// Aktualisiere die Felder, nachdem alle Zeilen geladen sind
			updateAllRows();

			logMessage("Data loaded successfully!");
		} catch (error) {
			logMessage(`Error loading data: ${error.message}`);
		}
}
		
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

    // Status der Radio-Buttons auslesen
    const sendToApiEnabled = document.getElementById("sendToApiTrue").checked;
    const sendToApiRow = `sendApiToTrue,${sendToApiEnabled}`; // Formatiere die Zeile

    // Füge den Status als letzte Zeile hinzu
    data.push(sendToApiRow);

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

	// Direktaufruf für IR
	function sendIRData() {
		logMessage("Direct Call (IR) initiated...");

		const protocol = document.getElementById('protocolInput').value;
		const address = document.getElementById('addressInput').value;
		const command = document.getElementById('commandInput').value;
		const repeat = document.getElementById('repeatIR').checked;

		if (!protocol || !address || !command) {
			alert('Bitte füllen Sie alle Felder aus!');
			return;
		}

		const params = new URLSearchParams({
			protocol: protocol,
			address: address,
			command: command,
			repeats: repeat ? '0' : '1' // Assuming 0 for false and 1 for true
		});

		fetch('http://192.168.10.167/sendIR', {
			method: 'POST',
			headers: {
				'Content-Type': 'application/x-www-form-urlencoded'
			},
			body: params.toString()
		})
		.then(response => {
			if (!response.ok) {
				throw new Error('HTTP error! status: ' + response.status);
			}
			return response.text();
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
    const modifier = document.getElementById('modifierInput').value;
    const keycode = document.getElementById('keycodeInput').value;
    const repeat = document.getElementById('repeatBLE').value;

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
			<td><input type="text" value="${values[13]}"></td>
			<td><input type="text" value="${values[14]}"></td>
			<td>${createBooleanDropdown(values[15])}</td>
			<td><button class="delete-btn" onclick="deleteRow(this)">Delete</button></td>
		`;

		tableBody.appendChild(newRow);

	}

	// Daten von /loadData laden
	async function loadData() {
		try {
			const response = await fetch("/loadData");
			if (!response.ok) {
				throw new Error(`HTTP error! Status: ${response.status}`);
			}

			const data = await response.text();
			// Trimme die Daten und filtere Leerzeilen
			const rows = data
				.trim()
				.split("\n")
				.map(row => row.trim()) // Entferne Leerzeichen vor und nach jeder Zeile
				.filter(row => row !== ""); // Entferne leere Zeilen

			let sendToApiValue = false; // Standardwert für sendToApi

			rows.forEach(row => {
				// Prüfen, ob die Zeile den sendToApi-Wert enthält
				if (row.startsWith("sendToApi")) {
					const value = row.split(",")[1]; // Extrahiere den Wert (True/False)
					sendToApiValue = value === "True"; // Konvertiere zu boolean
					return; // Überspringe diese Zeile, da sie nicht in die Tabelle eingefügt werden soll
				}

				// Füge nur die relevanten Datenzeilen in die Tabelle ein
				addRow(row);
			});

			// Setze den sendToApi-Status
			const sendToApiRadio = document.querySelector("input[id='sendToApiTrue']");
			if (sendToApiRadio) {
				sendToApiRadio.checked = sendToApiValue; // Setze den Status entsprechend
			}

			// Aktualisiere alle Zeilen nach dem Laden der Daten
			updateAllRows();

			logMessage("Data loaded successfully!");
		} catch (error) {
			logMessage(`Error loading data: ${error.message}`);
		}
	}

    // Initialisierung
    window.onload = function () {
        toggleInputFields();
        loadData();
    };
  </script>
	</body>
</html>
