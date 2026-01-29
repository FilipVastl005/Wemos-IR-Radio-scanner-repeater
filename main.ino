/*
 * IR and Radio Controller for ESP8266
 * 
 * This project allows you to:
 * - Send and receive IR signals (like TV remotes)
 * - Send and receive 433MHz radio signals (like wireless outlets)
 * - Control everything via Serial commands or Web interface
 * 
 * Hardware needed:
 * - ESP8266 board (WeMos D1 Mini or similar)
 * - IR LED and IR receiver
 * - 433MHz transmitter and receiver modules
 * 
 * Libraries required:
 * - IRremote (by shirriff/Arduino-IRremote)
 * - RCSwitch (by sui77)
 */

#include <Arduino.h>
#include <IRremote.hpp>
#include <RCSwitch.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ============================================================================
// CONFIGURATION - Change these settings to match your setup
// ============================================================================

// WiFi Access Point settings (creates its own WiFi network)
#define APSSID "ESPap"
#define APPSK "thereisnospoon"

// Pin assignments - adjust these if your wiring is different
#define RADIO_RECEIVE_PIN 5   // GPIO5 (D1 on WeMos)
#define RADIO_SENDER_PIN  13  // GPIO13 (D7 on WeMos)
#define IR_RECEIVE_PIN  4     // GPIO4 (D2 on WeMos)
#define IR_SENDER_PIN 14      // GPIO14 (D5 on WeMos)

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Radio control object
RCSwitch mySwitch = RCSwitch();

// Web server running on port 80
ESP8266WebServer server(80);

// Feature toggles
bool IRReceiveEnabled = true;
bool radioReceiveEnabled = false;

// For storing received data
uint32_t lastIrCode = 0;
String command = "";

// ============================================================================
// WEB INTERFACE - HTML page stored in program memory
// ============================================================================

const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { 
      font-family: sans-serif; 
      text-align: center; 
      background: #222; 
      color: #eee; 
      margin: 0;
      padding: 20px;
    }
    h1 { color: #03A062; }
    .card { 
      background: #333; 
      padding: 20px; 
      margin: 20px auto; 
      border-radius: 10px; 
      box-shadow: 0 4px 8px rgba(0,0,0,0.5);
      max-width: 600px;
    }
    button { 
      padding: 15px 30px; 
      font-size: 16px; 
      cursor: pointer; 
      border: none; 
      border-radius: 5px; 
      background: #03A062; 
      color: white;
      margin: 10px;
    }
    button:active { background: #028050; }
    #data-log { 
      background: #000; 
      color: #0f0; 
      padding: 10px; 
      text-align: left; 
      height: 200px; 
      overflow-y: scroll; 
      font-family: monospace;
      font-size: 12px;
      border-radius: 5px;
    }
  </style>
</head>
<body>
  <h1>🎛️ IR/Radio Controller</h1>
  
  <div class="card">
    <h3>Scanner Controls</h3>
    <button onclick="sendCmd('/toggleIR')">Toggle IR Scanner</button>
    <button onclick="sendCmd('/toggleRadio')">Toggle Radio Scanner</button>
  </div>
  
  <div class="card">
    <h3>📡 Captured Signals:</h3>
    <div id="data-log">Waiting for signals...<br></div>
  </div>

  <script>
    function sendCmd(path) {
      fetch(path)
        .then(response => response.text())
        .then(txt => {
          alert(txt);
          addLog('Command sent: ' + txt);
        });
    }
    
    function addLog(msg) {
      const log = document.getElementById('data-log');
      const time = new Date().toLocaleTimeString();
      log.innerHTML += '[' + time + '] ' + msg + '<br>';
      log.scrollTop = log.scrollHeight;
    }
    
    // Update data every 2 seconds
    setInterval(() => {
      fetch('/status')
        .then(r => r.json())
        .then(data => {
          if(data.lastIR && data.lastIR !== '0') {
            addLog('IR Signal: 0x' + data.lastIR);
          }
        })
        .catch(err => console.log('Status update failed'));
    }, 2000);
  </script>
</body>
</html>
)=====";

// ============================================================================
// SETUP - Runs once when the board starts
// ============================================================================

void setup() {
  // Start serial communication for debugging and commands
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=================================");
  Serial.println("IR/Radio Controller Starting...");
  Serial.println("=================================\n");
  
  // Initialize IR transmitter and receiver
  Serial.println("Setting up IR...");
  IrReceiver.begin(IR_RECEIVE_PIN);
  IrSender.begin(IR_SENDER_PIN); 
  IrSender.enableIROut(38);  // 38kHz carrier frequency
  Serial.println("✓ IR ready");
  
  // Initialize radio transmitter and receiver
  Serial.println("Setting up Radio...");
  mySwitch.enableReceive(RADIO_RECEIVE_PIN);
  mySwitch.enableTransmit(RADIO_SENDER_PIN);
  Serial.println("✓ Radio ready");
  
  // Start WiFi Access Point
  Serial.println("Setting up WiFi...");
  WiFi.softAP(APSSID, APPSK);
  Serial.print("✓ WiFi AP started - IP: ");
  Serial.println(WiFi.softAPIP());
  
  // Configure web server routes
  server.on("/", handleRoot);
  server.on("/toggleIR", handleToggleIR);
  server.on("/toggleRadio", handleToggleRadio);
  server.on("/status", handleStatus);
  server.begin();
  Serial.println("✓ Web server started");
  
  Serial.println("\n=================================");
  Serial.println("Setup complete! Ready for commands.");
  Serial.println("=================================");
  Serial.println("\nAvailable commands:");
  Serial.println("  ir <protocol> <address> <command> <repeats>");
  Serial.println("  radio <pulseLength> <code> <bitLength> <protocol>");
  Serial.println("  help");
  Serial.println("\nConnect to WiFi: " + String(APSSID));
  Serial.println("Password: " + String(APPSK));
  Serial.println("Open browser to: http://" + WiFi.softAPIP().toString());
  Serial.println();
}

// ============================================================================
// MAIN LOOP - Runs continuously
// ============================================================================

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Check for serial commands
  handleSerialCommands();
  
  // Check for incoming IR signals
  if (IRReceiveEnabled) {
    checkIRReceiver();
  }
  
  // Check for incoming radio signals
  if (radioReceiveEnabled) {
    checkRadioReceiver();
  }
}

// ============================================================================
// SERIAL COMMAND HANDLING
// ============================================================================

void handleSerialCommands() {
  if (Serial.available()) {
    command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.startsWith("ir")) {
      // Parse IR command: ir <protocol> <address> <command> <repeats>
      command.remove(0, 2);
      command.trim(); 
      
      int protocolIndex = command.indexOf(' ');
      String protocol = command.substring(0, protocolIndex);

      int addressIndex = command.indexOf(' ', protocolIndex + 1);
      uint16_t address = command.substring(protocolIndex + 1, addressIndex).toInt();

      int commandIndex = command.indexOf(' ', addressIndex + 1);
      uint16_t commandIR = command.substring(addressIndex + 1, commandIndex).toInt();

      int repeatsIndex = command.indexOf(' ', commandIndex + 1);
      int_fast8_t repeats = command.substring(commandIndex + 1, repeatsIndex).toInt();
      
      Serial.println("Sending IR signal...");
      sendIR(protocol, address, commandIR, repeats);
      Serial.println("✓ IR signal sent");
      
    } else if (command.startsWith("radio")) {
      // Parse radio command: radio <pulseLength> <code> <bitLength> <protocol>
      command.remove(0, 5);
      command.trim();
      
      int pulseLengthIndex = command.indexOf(' ');
      int pulseLength = command.substring(0, pulseLengthIndex).toInt();
      
      int codeIndex = command.indexOf(' ', pulseLengthIndex + 1);
      int code = command.substring(pulseLengthIndex + 1, codeIndex).toInt();
      
      int bitLengthIndex = command.indexOf(' ', codeIndex + 1);
      int bitLength = command.substring(codeIndex + 1, bitLengthIndex).toInt();
      
      int protocol = command.substring(bitLengthIndex + 1).toInt();
      if (protocol == 0) protocol = 1; // Default to protocol 1
      
      Serial.println("Sending radio signal...");
      sendRadio(pulseLength, code, bitLength, protocol);
      Serial.println("✓ Radio signal sent");
      
    } else if (command.startsWith("help")) {
      showHelp();
      
    } else {
      Serial.println("❌ Unknown command");
      showHelp();
    }
  }
}

void showHelp() {
  Serial.println("\n=================================");
  Serial.println("COMMAND HELP");
  Serial.println("=================================");
  Serial.println("\nIR Commands:");
  Serial.println("  ir <protocol> <address> <command> <repeats>");
  Serial.println("  Protocols: NEC, NEC2, Samsung, Onkyo, NECext");
  Serial.println("  Example: ir NEC 61184 3 0");
  Serial.println("\nRadio Commands:");
  Serial.println("  radio <pulseLength> <code> <bitLength> <protocol>");
  Serial.println("  Example: radio 350 5393 24 1");
  Serial.println("\nOther:");
  Serial.println("  help - Show this help message");
  Serial.println("=================================\n");
}

// ============================================================================
// IR FUNCTIONS
// ============================================================================

void sendIR(String protocol, uint16_t address, uint16_t command, int_fast8_t repeats) {
  if (protocol.equalsIgnoreCase("NEC")) {
    IrSender.sendNEC(address, command, repeats);
  } else if (protocol.equalsIgnoreCase("NEC2")) {
    IrSender.sendNEC2(address, command, repeats);
  } else if (protocol.equalsIgnoreCase("Samsung")) {
    IrSender.sendSamsung(address, command, repeats);
  } else if (protocol.equalsIgnoreCase("Onkyo") || protocol.equalsIgnoreCase("NECext")) {
    IrSender.sendOnkyo(address, command, repeats);
  } else {
    Serial.println("❌ Unknown IR protocol: " + protocol);
  }
}

void checkIRReceiver() {
  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
      // Unknown protocol - show raw data
      auto tDecodedRawData = IrReceiver.decodedIRData.decodedRawData;
      Serial.print(F("📡 Raw IR data: 0x"));
      Serial.println(tDecodedRawData, HEX);
    } else {
      // Known protocol - show decoded info
      Serial.println("📡 IR Signal Received:");
      IrReceiver.printIRResultShort(&Serial);
      IrReceiver.printIRSendUsage(&Serial);
      
      // Store for web interface
      lastIrCode = IrReceiver.decodedIRData.decodedRawData;
    }
    IrReceiver.resume();  // Ready for next signal
  }
}

// ============================================================================
// RADIO FUNCTIONS
// ============================================================================

void sendRadio(int nPulseLength, int decimalCode, int bitLength, int protocol) {
  Serial.println("Using protocol: " + String(protocol));
  
  mySwitch.setProtocol(protocol);
  mySwitch.setPulseLength(nPulseLength);
  mySwitch.send(decimalCode, bitLength);
}

void checkRadioReceiver() {
  if (mySwitch.available()) {
    Serial.println("📻 Radio Signal Received:");
    Serial.print("  Code: ");
    Serial.println(mySwitch.getReceivedValue());
    Serial.print("  Bit length: ");
    Serial.println(mySwitch.getReceivedBitlength());
    Serial.print("  Pulse length: ");
    Serial.println(mySwitch.getReceivedDelay());
    Serial.print("  Protocol: ");
    Serial.println(mySwitch.getReceivedProtocol());
    
    mySwitch.resetAvailable();
  }
}

// ============================================================================
// WEB SERVER HANDLERS
// ============================================================================

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleToggleIR() {
  IRReceiveEnabled = !IRReceiveEnabled;
  String message = IRReceiveEnabled ? "✓ IR Scanner ON" : "✓ IR Scanner OFF";
  Serial.println(message);
  server.send(200, "text/plain", message);
}

void handleToggleRadio() {
  radioReceiveEnabled = !radioReceiveEnabled;
  String message = radioReceiveEnabled ? "✓ Radio Scanner ON" : "✓ Radio Scanner OFF";
  Serial.println(message);
  server.send(200, "text/plain", message);
}

void handleStatus() {
  // Send JSON status update to web interface
  String json = "{";
  json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"lastIR\":\"" + String(lastIrCode, HEX) + "\",";
  json += "\"irEnabled\":" + String(IRReceiveEnabled ? "true" : "false") + ",";
  json += "\"radioEnabled\":" + String(radioReceiveEnabled ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}
