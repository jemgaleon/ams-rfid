#pragma once

#include <stdint.h>

// -------------------- Wi-Fi --------------------
// WIFI_SSID and WIFI_PASSWORD live in secrets.h, which is git-ignored.
// Copy secrets.example.h to secrets.h and fill them in.
#include "secrets.h"

// -------------------- API ----------------------
// AMS dev server on this Mac's LAN IP (localhost on the ESP32 is the ESP32 itself).
constexpr const char* API_ENDPOINT = "http://192.168.0.100:3001/api/scans";
constexpr const char* HEARTBEAT_ENDPOINT = "http://192.168.0.100:3001/api/scanners/heartbeat";
constexpr const char* SCANNER_ID = "RFID-001";

// -------------------- RFID (MFRC522 / SPI) ----
constexpr uint8_t RFID_SS_PIN = 5;    // IO5 -> SDA/SS
constexpr uint8_t RFID_RST_PIN = 13;  // IO13 -> RST
constexpr uint8_t RFID_SCK_PIN = 18;  // IO18 -> SCK
constexpr uint8_t RFID_MISO_PIN = 19; // IO19 -> MISO
constexpr uint8_t RFID_MOSI_PIN = 23; // IO23 -> MOSI

// -------------------- OLED (SSD1306 / I2C) ----
constexpr uint8_t OLED_SDA_PIN = 21;   // IO21 -> SDA
constexpr uint8_t OLED_SCL_PIN = 22;   // IO22 -> SCL
constexpr uint8_t OLED_I2C_ADDR = 0x3C;

// -------------------- Feedback (LEDs / buzzer) --
constexpr uint8_t RED_LED_PIN = 25;    // IO25 -> 220R -> red LED    (idle / denied)
constexpr uint8_t GREEN_LED_PIN = 26;  // IO26 -> 220R -> green LED  (valid scan)
constexpr uint8_t BUZZER_PIN = 27;     // IO27 -> 1k -> NPN base, active buzzer on collector

// -------------------- Timings ------------------
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 10000;
constexpr uint32_t HTTP_TIMEOUT_MS = 7000;
// AMS shows a Scanner Offline after 3 minutes without a heartbeat (AMS ADR 0007).
constexpr uint32_t HEARTBEAT_INTERVAL_MS = 60000;
constexpr uint32_t HEARTBEAT_TIMEOUT_MS = 3000;  // short: taps aren't read while it runs
constexpr uint32_t CARD_REPEAT_GUARD_MS = 1500;
constexpr uint32_t RESULT_DISPLAY_MS = 3000;  // how long a scan result stays on screen
constexpr uint32_t STATUS_DISPLAY_MS = 3000;  // Wi-Fi connected / failed, RFID error at boot
constexpr uint32_t BEEP_MS = 80;              // buzzer pulse on every card scan
constexpr uint32_t RFID_HEALTH_CHECK_MS = 5000;  // re-checks the reader, re-inits it if it reset

// -------------------- Card ID format -----------
// true: send 4-byte UIDs as the decimal number the USB desk reader types (e.g. 3882434570).
// false: send uppercase hex (e.g. 0A4069E7).
constexpr bool CARD_UID_AS_DECIMAL = true;

// -------------------- Debug --------------------
constexpr bool RFID_DEBUG = true;  // logs reader registers and partial card reads
