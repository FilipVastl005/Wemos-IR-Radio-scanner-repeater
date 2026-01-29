#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

// WiFi credentials - UPDATE THESE WITH YOUR NETWORK INFO
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Pin definitions
const uint16_t IR_SEND_PIN = D2;    // IR LED transmitter pin
const uint16_t IR_RECV_PIN = D5;    // IR receiver pin

// IR objects
IRsend IrSender(IR_SEND_PIN);
IRrecv IrReceiver(IR_RECV_PIN);
decode_results results;

// Web server
ESP8266WebServer server(80);

// State variables
bool irEnabled = false;
bool radioEnabled = false;
uint64_t lastIrCode = 0;
decode_type_t lastIrProtocol = UNKNOWN;
uint16_t lastIrBits = 0;

// Function declarations
void handleRoot();
void handleToggleIR();
void handleToggleRadio();
void handleRepeatSignal();
void handleStatus();

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=================================");
  Serial.println("Wemos IR Radio Scanner Repeater");
  Serial.println("=================================");

  // Initialize IR sender
  IrSender.begin();
  Serial.println("IR Sender initialized");
  
  // Initialize IR receiver
  IrReceiver.enableIRIn();
  Serial.println("IR Receiver initialized");
  
  // Connect to WiFi
  Serial.println("\nConnecting to WiFi...");
  Serial.print("SSID: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Access web interface at: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n\nWiFi Connection Failed!");
    Serial.println("Please check your credentials and try again.");
  }

  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggleIR", HTTP_GET, handleToggleIR);
  server.on("/toggleRadio", HTTP_GET, handleToggleRadio);
  server.on("/repeatSignal", HTTP_GET, handleRepeatSignal);
  server.on("/status", HTTP_GET, handleStatus);
  
  server.begin();
  Serial.println("Web server started");
  Serial.println("=================================");
  Serial.println("Ready to receive IR signals!");
  Serial.println("=================================\n");
}

void loop() {
  server.handleClient();
  
  // Check for IR signals if scanning is enabled
  if (irEnabled && IrReceiver.decode(&results)) {
    lastIrCode = results.value;
    lastIrProtocol = results.decode_type;
    lastIrBits = results.bits;
    
    Serial.println("---------------------------");
    Serial.print("IR Signal Received!");
    Serial.print("\n  Code: 0x");
    Serial.println(lastIrCode, HEX);
    Serial.print("  Protocol: ");
    Serial.println(typeToString(lastIrProtocol));
    Serial.print("  Bits: ");
    Serial.println(lastIrBits);
    Serial.println("---------------------------");
    
    IrReceiver.resume();  // Prepare for next signal
  }
}

// Handler for root page - serves the HTML interface
void handleRoot() {
  String html = F("<!DOCTYPE html>
<html>
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Wemos IR Radio Scanner Repeater</title>
        <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            background-color: #f5f5f5;
        }
        h1 {
            color: #333;
        }
        h2 {
            color: #555;
            margin-top: 20px;
        }
        button {
            padding: 10px 20px;
            margin: 5px;
            font-size: 14px;
            cursor: pointer;
            border: none;
            border-radius: 4px;
            background-color: #4CAF50;
            color: white;
            transition: background-color 0.3s;
        }
        button:hover {
            background-color: #45a049;
        }
        button:active {
            background-color: #3d8b40;
        }
        #controls {
            background-color: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            margin-bottom: 20px;
        }
        #logs {
            background-color: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        #log-messages {
            border: 1px solid #ccc;
            padding: 10px;
            height: 300px;
            overflow-y: scroll;
            background-color: #fafafa;
            font-family: 'Courier New', monospace;
            font-size: 12px;
        }
        #log-messages p {
            margin: 5px 0;
            padding: 2px 0;
        }
        .info {
            background-color: white;
            padding: 15px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            margin-bottom: 20px;
        }
        </style>
    </head>
    <body>
        <h1>Wemos IR Radio Scanner Repeater</h1>
        
        <div class="info">
            <p>This is the web interface for the Wemos IR Radio Scanner Repeater project. Use this interface to monitor and control the IR signals being scanned and repeated by the device.</p>
            
            <h2>Features</h2>
            <ul>
                <li>Scan for IR signals from various remote controls.</li>
                <li>Repeat captured IR signals to control devices.</li>
                <li>View real-time data on scanned IR signals.</li>
            </ul>
            
            <h2>Instructions</h2>
            <ol>
                <li>Connect your Wemos device to the network.</li>
                <li>Open this web interface in your browser.</li>
                <li>Use the controls provided to start scanning or repeating IR signals.</li>
            </ol>
        </div>

        <div id="controls">
            <h2>Controls</h2>
            <button id="start-scan">Start Scan</button>
            <button id="stop-scan">Stop Scan</button>
            <button id="repeat-signal">Repeat Last Signal</button>
        </div>
        
        <div id="logs">
            <h2>Logs</h2>
            <div id="log-messages">
                <p>No logs yet. Waiting for commands...</p>
            </div>
        </div>

        <script>
            const startScanBtn = document.getElementById('start-scan');
            const stopScanBtn = document.getElementById('stop-scan');
            const repeatSignalBtn = document.getElementById('repeat-signal');
            const logMessages = document.getElementById('log-messages');

            // Helper function to add log messages with timestamp
            function addLog(message, type = 'info') {
                const timestamp = new Date().toLocaleTimeString();
                const color = type === 'error' ? 'color: red;' : type === 'success' ? 'color: green;' : '';
                logMessages.innerHTML += `<p style="${color}">[${timestamp}] ${message}</p>`;
                logMessages.scrollTop = logMessages.scrollHeight;
            }

            // Start scan button handler
            startScanBtn.addEventListener('click', () => {
                addLog('Starting IR scan...');
                fetch('/toggleIR')
                    .then(response => response.text())
                    .then(text => {
                        addLog(text, 'success');
                    })
                    .catch(error => {
                        addLog('Error starting scan: ' + error, 'error');
                    });
            });

            // Stop scan button handler
            stopScanBtn.addEventListener('click', () => {
                addLog('Stopping IR scan...');
                fetch('/toggleIR')
                    .then(response => response.text())
                    .then(text => {
                        addLog(text, 'success');
                    })
                    .catch(error => {
                        addLog('Error stopping scan: ' + error, 'error');
                    });
            });

            // Repeat signal button handler (THIS WAS MISSING!)
            repeatSignalBtn.addEventListener('click', () => {
                addLog('Attempting to repeat last IR signal...');
                fetch('/repeatSignal')
                    .then(response => response.text())
                    .then(text => {
                        addLog(text, text.includes('✓') ? 'success' : 'error');
                    })
                    .catch(error => {
                        addLog('Error repeating signal: ' + error, 'error');
                    });
            });

            // Auto-update status every 3 seconds
            setInterval(() => {
                fetch('/status')
                    .then(response => response.json())
                    .then(data => {
                        // You can use this data to update UI elements
                        console.log('Status:', data);
                        if (data.lastIrCode && data.lastIrCode !== '0x0') {
                            // Log new IR signals detected
                            // addLog(`IR Signal: ${data.lastIrCode} (${data.lastIrProtocol})`);
                        }
                    })
                    .catch(error => {
                        console.error('Status update error:', error);
                    });
            }, 3000);

            // Initial connection check
            window.addEventListener('load', () => {
                addLog('Web interface loaded. Checking connection to device...');
                fetch('/status')
                    .then(response => response.json())
                    .then(data => {
                        addLog('✓ Connected to Wemos device successfully!', 'success');
                    })
                    .catch(error => {
                        addLog('✗ Could not connect to device. Please check connection.', 'error');
                    });
            });
        </script>
    </body>
</html>");

  server.send(200, "text/html", html);
}

// Handler to toggle IR scanning
void handleToggleIR() {
  irEnabled = !irEnabled;
  
  if (irEnabled) {
    IrReceiver.enableIRIn();
    Serial.println("[i:46:22 PM] Starting IR scan...");
    Serial.println("IR scanning ENABLED");
    server.send(200, "text/plain", "✓ IR scanning enabled");
  } else {
    IrReceiver.disableIRIn();
    Serial.println("IR scanning DISABLED");
    server.send(200, "text/plain", "✗ IR scanning disabled");
  }
}

// Handler to toggle radio (placeholder functionality)
void handleToggleRadio() {
  radioEnabled = !radioEnabled;
  
  if (radioEnabled) {
    Serial.println("Radio enabled");
    server.send(200, "text/plain", "✓ Radio enabled");
  } else {
    Serial.println("Radio disabled");
    server.send(200, "text/plain", "✗ Radio disabled");
  }
}

// Handler to repeat the last captured IR signal
void handleRepeatSignal() {
  if (lastIrCode != 0) {
    Serial.println("---------------------------");
    Serial.println("REPEATING SIGNAL");
    Serial.print("  Code: 0x");
    Serial.println(lastIrCode, HEX);
    Serial.print("  Protocol: ");
    Serial.println(typeToString(lastIrProtocol));
    Serial.println("---------------------------");
    
    // Send the IR signal based on protocol
    switch (lastIrProtocol) {
      case NEC:
        IrSender.sendNEC(lastIrCode, lastIrBits);
        break;
      case SONY:
        IrSender.sendSony(lastIrCode, lastIrBits);
        break;
      case RC5:
        IrSender.sendRC5(lastIrCode, lastIrBits);
        break;
      case RC6:
        IrSender.sendRC6(lastIrCode, lastIrBits);
        break;
      case SAMSUNG:
        IrSender.sendSAMSUNG(lastIrCode, lastIrBits);
        break;
      default:
        // For unknown protocols, try sending as raw
        IrSender.sendRaw((uint16_t*)&lastIrCode, 1, 38);
        break;
    }
    
    server.send(200, "text/plain", "✓ Last signal repeated");
  } else {
    Serial.println("No signal to repeat yet");
    server.send(200, "text/plain", "✗ No signal captured yet");
  }
}

// Handler to get current status
void handleStatus() {
  String json = "{";
  json += "\"irEnabled\":" + String(irEnabled ? "true" : "false") + ",";
  json += "\"radioEnabled\":" + String(radioEnabled ? "true" : "false") + ",";
  json += "\"lastIrCode\":\"0x" + String(lastIrCode, HEX) + "\",";
  json += "\"lastIrProtocol\":\"" + String(typeToString(lastIrProtocol)) + "\",";
  json += "\"lastIrBits\":" + String(lastIrBits);
  json += "}";
  
  server.send(200, "application/json", json);
}
