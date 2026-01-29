# Wemos IR Radio Scanner Repeater

## IMPORTANT: File Structure

**You only need to upload `main.cpp` to your Wemos!**

- **main.cpp** - This is the ONLY file you upload to Arduino IDE. It contains everything including the HTML interface embedded inside it.
- **index.html** - This is just for your reference/organization. Don't upload it separately.
- **main.ino** - If you have this, it's just for Arduino IDE compatibility (Arduino IDE likes .ino files)
- **README.md** - Documentation (this file)

The HTML interface is **embedded inside main.cpp**, so when you upload main.cpp to your Wemos, it automatically includes the web interface. You don't need to upload the HTML file separately.

## Hardware Requirements

- Wemos D1 Mini (or compatible ESP8266 board)
- IR Receiver module (connected to pin D5)
- IR LED transmitter (connected to pin D2)
- Appropriate resistors and power supply

## Software Requirements

### Arduino Libraries (Install via Library Manager)
1. ESP8266WiFi (included with ESP8266 board package)
2. ESP8266WebServer (included with ESP8266 board package)
3. IRremoteESP8266 (by David Conran, Sebastien Warin, Mark Szabo)

## Setup Instructions

### 1. Install ESP8266 Board Support

1. Open Arduino IDE
2. Go to **File → Preferences**
3. Add this URL to "Additional Board Manager URLs": 
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "esp8266" and click **Install**

### 2. Install IRremoteESP8266 Library

1. Go to **Sketch → Include Library → Manage Libraries**
2. Search for "IRremoteESP8266"
3. Install the library by David Conran

### 3. Configure WiFi Credentials

Open `main.cpp` and update lines 9-10:

```cpp
const char* ssid = "YOUR_WIFI_SSID";      // Replace with your WiFi name
const char* password = "YOUR_WIFI_PASSWORD";  // Replace with your WiFi password
```

### 4. Upload to Wemos

1. Connect your Wemos to computer via USB
2. In Arduino IDE, select:
   - **Tools → Board → ESP8266 Boards → LOLIN(WEMOS) D1 R2 & mini**
   - **Tools → Port →** (select your COM port)
3. Click the **Upload** button (→)
4. Wait for upload to complete

### 5. Find Your Device IP Address

1. Open **Tools → Serial Monitor**
2. Set baud rate to **115200**
3. Press the **reset button** on your Wemos
4. Look for output like:
   ```
   WiFi Connected!
   IP Address: 192.168.1.100
   Access web interface at: http://192.168.1.100
   ```

### 6. Access the Web Interface

1. Open a web browser on any device connected to the same WiFi
2. Navigate to the IP address from step 5
3. You should see the control interface!

## How to Use

### Web Interface Controls

1. **Start Scan** - Click to enable IR signal detection
2. **Stop Scan** - Click to disable IR signal detection  
3. **Repeat Last Signal** - Click to retransmit the last captured IR signal

### Usage Flow

1. Click **"Start Scan"**
2. Point your remote at the IR receiver and press a button
3. Check the Serial Monitor - you should see the signal details
4. Click **"Repeat Last Signal"** to retransmit it
5. The IR LED will send out the same signal

## Serial Monitor Output

The Serial Monitor will show detailed information:

```
=================================
Wemos IR Radio Scanner Repeater
=================================
IR Sender initialized
IR Receiver initialized

Connecting to WiFi...
SSID: YourWiFiName
....
WiFi Connected!
IP Address: 192.168.1.100
Access web interface at: http://192.168.1.100
Web server started
=================================
Ready to receive IR signals!
=================================

[i:46:22 PM] Starting IR scan...
IR scanning ENABLED

---------------------------
IR Signal Received!
  Code: 0xFF30CF
  Protocol: NEC
  Bits: 32
---------------------------
```

## API Endpoints

The device exposes these HTTP endpoints:

- `GET /` - Serves the web interface
- `GET /toggleIR` - Toggle IR scanning on/off
- `GET /toggleRadio` - Toggle radio functionality (placeholder)
- `GET /repeatSignal` - Repeat the last captured IR signal
- `GET /status` - Get current device status as JSON

You can test these directly in your browser:
```
http://192.168.1.100/status
http://192.168.1.100/toggleIR
http://192.168.1.100/repeatSignal
```

## Supported IR Protocols

The device automatically detects and can replay:
- NEC
- Sony
- RC5
- RC6
- Samsung
- Raw signals (for unknown protocols)

## Troubleshooting

### "File not found" Error

**This error means the HTML is not being served properly. Solution:**

The updated `main.cpp` I provided has the HTML embedded inside it. Make sure you're using the new version and re-upload it to your Wemos.

### Can't Connect to WiFi

- Verify SSID and password are correct in the code
- ESP8266 only supports 2.4GHz WiFi (not 5GHz)
- Move closer to your router
- Check Serial Monitor for connection status

### IR Signals Not Detected

- Verify IR receiver is connected to pin D5
- Check power connections (usually 3.3V or 5V + GND)
- Point remote directly at receiver
- Press remote buttons firmly
- Check Serial Monitor for debug messages

### Can't Repeat Signals

- Capture a signal first by clicking "Start Scan"
- Verify IR LED is connected to pin D2
- Check LED polarity (flat edge = cathode = -)
- Use appropriate current-limiting resistor (100-330Ω)
- Some devices need multiple sends to respond

### Web Interface Won't Load

- Verify device IP address in Serial Monitor
- Ensure you're on the same WiFi network
- Try `http://192.168.1.X` (replace X with your IP)
- Clear browser cache
- Try different browser

## Pin Configuration

Default pin assignments (change in code if needed):

```cpp
const uint16_t IR_SEND_PIN = D2;    // GPIO4 - IR LED transmitter
const uint16_t IR_RECV_PIN = D5;    // GPIO14 - IR receiver
```

## Wiring Diagram

### IR Receiver (e.g., TSOP38238)
```
IR Receiver:
  VCC → 3.3V or 5V
  GND → GND
  OUT → D5
```

### IR LED
```
IR LED:
  Anode (+) → D2 (through 100-330Ω resistor)
  Cathode (-) → GND
```

## Tips

- Keep the IR LED and receiver separated to avoid interference
- Use a clear/transparent LED case for IR LED if possible
- Point the IR LED in the direction of your target device
- Some remotes use different frequencies (38kHz is most common)
- You can monitor all activity via Serial Monitor at 115200 baud

## License

Open source - feel free to modify and use as needed.
