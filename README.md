# ESP32 RFID Attendance Scanner

Scans RFID cards, posts each scan to the attendance API, and shows the result on an OLED screen, with LEDs and a buzzer for feedback.

## What it does
*   **RFID Scanning:** Reads MIFARE cards with an RC522 reader.
*   **API Integration:** POSTs `{ scannerId, scanId, cardUid }` as JSON to `API_ENDPOINT` and shows the answer `{ result, name?, message }`, e.g. `{ "result": "valid", "name": "Juan Dela Cruz Santos", "message": "Late 8:20 AM" }`. `"valid"` (Present or Late) lights green; `"invalid"`, or a non-2xx answer, is red.
*   **Heartbeat:** POSTs `{ scannerId }` to `HEARTBEAT_ENDPOINT` as soon as Wi-Fi connects (and on every reconnect), then every 60 s, so AMS shows the Scanner Online. AMS marks it Offline after 3 minutes without one.
*   **OLED Feedback:** Every screen has the title `Device ID: <SCANNER_ID>` above three lines:
    | Screen | Line 1 | Line 2 | Line 3 |
    | :--- | :--- | :--- | :--- |
    | Boot | Loading... (animated) | | |
    | Reader fault | RFID Error | Check wiring... | |
    | Wi-Fi connecting | WiFi Connecting... (animated) | | |
    | Wi-Fi connected (3 s) | WiFi Connected | IP: 192.168.x.x | |
    | Wi-Fi failed (3 s) | WiFi Error | Retrying... | |
    | Idle | Card ID: - | Tap Card... | |
    | Sending | Card ID: xxx | Please wait... (animated) | |
    | Result (3 s) | Card ID: xxx | student name | message |
    | Server unreachable (3 s) | Card ID: xxx | Unable to connect server | WiFi disconnected / HTTP failed / POST failed / Server error NNN |
*   **LED / Buzzer Feedback:**
    | State | Red LED | Green LED | Buzzer |
    | :--- | :---: | :---: | :---: |
    | Powered on / idle | on | off | – |
    | Card tapped | – | – | short beep |
    | `result` "valid" (3 s) | off | on | – |
    | Denied / network error | on | off | – |
*   **Wi-Fi:** Connects at boot and keeps retrying if the connection drops.

## Hardware
*   **Microcontroller:** ESP32 DevKit V1, powered over USB
*   **RFID Module:** RC522 (SPI, **3.3 V only**)
*   **Display:** SSD1306 OLED 128x64 (I2C)
*   **Indicators:** red LED and green LED, each with a 220 Ω resistor
*   **Buzzer:** 5 V active buzzer, switched by an S8050/2N2222 NPN transistor (1 kΩ base resistor, 1N4148 flyback diode)

The full parts list is in `specs/bom.json`. For the schematic, see the published schematic artifact.

## Wiring

### Power
| Net | Connected to |
| :--- | :--- |
| **3V3** (ESP32 3V3) | RFID 3.3V, OLED VCC |
| **5V** (ESP32 VIN, from USB) | Buzzer + |
| **GND** (ESP32 GND) | RFID GND, OLED GND, LED cathodes, transistor emitter |

### RFID (SPI)
| RC522 Pin | ESP32 Pin |
| :--- | :--- |
| SDA (SS) | **IO5** |
| SCK | **IO18** |
| MOSI | **IO23** |
| MISO | **IO19** |
| RST | **IO13** |
| IRQ | *(not used)* |

### OLED (I2C, address 0x3C)
| OLED Pin | ESP32 Pin |
| :--- | :--- |
| SDA | **IO21** |
| SCL | **IO22** |

### LEDs and Buzzer
| Part | Wiring |
| :--- | :--- |
| Red LED | **IO25** → 220 Ω → LED anode, cathode → GND |
| Green LED | **IO26** → 220 Ω → LED anode, cathode → GND |
| Buzzer | **IO27** → 1 kΩ → NPN base. Emitter → GND. Collector → buzzer −. Buzzer + → 5V. 1N4148 across the buzzer (cathode to 5V) |

## How to use
1.  **Setup:** Install [PlatformIO](https://platformio.org/).
2.  **Configure:** Edit `src/config.h`:
    *   `WIFI_SSID` / `WIFI_PASSWORD`: your network, in `src/secrets.h` (git-ignored). Copy `src/secrets.example.h` to start.
    *   `API_ENDPOINT` / `HEARTBEAT_ENDPOINT`: the AMS scan and heartbeat URLs. Use the server's LAN IP, because `localhost` on the ESP32 points at the ESP32 itself.
    *   `SCANNER_ID`: a unique id for this device.
    *   The pin and timing constants live here too.
3.  **Upload:** Run `pio run -t upload`, then `pio device monitor` to see the logs.
4.  **Run:** Once it boots, the red LED turns on and the screen shows *Tap Card...*. Tap a card to scan it.

## Files
| File | Purpose |
| :--- | :--- |
| `platformio.ini` | PlatformIO build environment |
| `src/config.h` | Wi-Fi, API, pins and timings |
| `src/main.cpp` | Boot sequence and scan loop |
| `src/rfid_reader.*` | RC522 init and card reads |
| `src/display.*` | OLED screen output |
| `src/feedback.*` | Red and green LEDs and the buzzer |
| `src/connection.*` | Wi-Fi connection, the heartbeat and the attendance API call |
| `src/utils.*` | String, UID and minimal JSON helpers |
| `specs/bom.json` | Bill of materials |
