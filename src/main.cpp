// ESP32 RFID attendance scanner.
//
// Flow: boot -> connect Wi-Fi -> heartbeat -> idle (red LED)
//       card tapped -> beep -> POST to API -> show result
//       (green LED if accepted) -> back to idle.
// A heartbeat goes to AMS on every (re)connect and every HEARTBEAT_INTERVAL_MS after.

#include <Arduino.h>

#include "config.h"
#include "display.h"
#include "feedback.h"
#include "connection.h"
#include "rfid_reader.h"
#include "utils.h"

// Last card seen, so holding a card on the reader doesn't flood the API.
static String lastCardUid;
static uint32_t lastCardAtMs = 0;

static bool wasConnected = false;
static bool showingReaderFault = false;

// ---------------------------------------------------------------- screens

static String dots(uint8_t count) { return String("...").substring(0, count); }

static String cardLine(const String& cardUid) { return "Card ID: " + cardUid; }

static void showIdleScreen() { display::show(cardLine("-"), "Tap Card..."); }

static void showReaderFaultScreen() { display::show("RFID Error", "Check wiring..."); }

static void showReadyOrFaultScreen() {
  if (showingReaderFault) {
    showReaderFaultScreen();
  } else {
    showIdleScreen();
  }
}

static void showResultScreen(const String& cardUid, const connection::ScanResult& result) {
  if (!result.delivered) {
    display::show(cardLine(cardUid), "Unable to connect server", result.message);
    return;
  }
  display::show(cardLine(cardUid), result.name, result.message);
}

// Holds the current screen until `ms` have passed since `sinceMs`.
static void holdUntil(uint32_t sinceMs, uint32_t ms) {
  const uint32_t elapsed = millis() - sinceMs;
  if (elapsed < ms) delay(ms - elapsed);
}

// ---------------------------------------------------------------- logging

static void logResult(const connection::ScanResult& result) {
  if (!result.delivered) {
    Serial.printf("[ERROR] %s (HTTP %d) %s\n", result.message.c_str(), result.httpCode,
                  result.rawBody.c_str());
    return;
  }
  Serial.printf("[HTTP] Code: %d\n", result.httpCode);
  Serial.printf("[HTTP] Response: %s\n", result.rawBody.c_str());
  Serial.printf("[PARSED] result=%s name=%s message=%s\n", result.accepted ? "true" : "false",
                result.name.c_str(), result.message.c_str());
}

// ---------------------------------------------------------------- Wi-Fi

// Pings AMS first, so it marks this Scanner Online, while "WiFi Connected" is on screen.
static void onWiFiConnected() {
  Serial.printf("[WiFi] Connected, IP: %s\n", connection::ipText().c_str());
  const uint32_t shownAtMs = millis();
  display::show("WiFi Connected", "IP: " + connection::ipText());
  connection::sendHeartbeat();
  holdUntil(shownAtMs, STATUS_DISPLAY_MS);
}

// ---------------------------------------------------------------- boot steps

static void showLoading() {
  for (uint8_t frame = 0; frame < 8; frame++) {
    display::show("Loading" + dots(frame % 4));
    delay(150);
  }
}

static void startRfidReader() {
  if (rfid_reader::begin()) {
    Serial.println("[RFID] OK");
  } else {
    Serial.println("[RFID] NOT detected - check wiring/power");
    showReaderFaultScreen();
    delay(STATUS_DISPLAY_MS);
  }
}

// Blocks until Wi-Fi connects or WIFI_CONNECT_TIMEOUT_MS passes.
static void connectWiFi() {
  connection::beginWiFi();

  const uint32_t startMs = millis();
  uint8_t frame = 0;
  while (!connection::isConnected() && millis() - startMs < WIFI_CONNECT_TIMEOUT_MS) {
    display::show("WiFi Connecting" + dots(frame++ % 4));
    delay(300);
  }

  wasConnected = connection::isConnected();
  if (wasConnected) {
    onWiFiConnected();
  } else {
    Serial.println("[WiFi] Connection failed, will keep retrying");
    display::show("WiFi Error", "Retrying...");
    delay(STATUS_DISPLAY_MS);
  }
}

// ---------------------------------------------------------------- loop checks

// A reconnect gets the same "WiFi Connected" screen and heartbeat as the first connect.
static void checkWiFi() {
  connection::keepWiFiAlive();
  const bool connected = connection::isConnected();
  if (connected && !wasConnected) {
    wasConnected = true;
    onWiFiConnected();
    showReadyOrFaultScreen();
  } else if (!connected && wasConnected) {
    wasConnected = false;
    Serial.println("[WiFi] Disconnected");
  }
}

// Shows "RFID Error" while the reader doesn't answer, and the idle screen once it does again.
static void checkReader() {
  rfid_reader::checkHealth();
  const bool faulty = !rfid_reader::isHealthy();
  if (faulty == showingReaderFault) return;
  showingReaderFault = faulty;
  showReadyOrFaultScreen();
}

// ---------------------------------------------------------------- scan handling

static bool isRepeatTap(const String& cardUid) {
  return cardUid == lastCardUid && millis() - lastCardAtMs < CARD_REPEAT_GUARD_MS;
}

static void handleScan(const String& cardUid) {
  lastCardUid = cardUid;
  lastCardAtMs = millis();

  const String scanId = utils::buildScanId(SCANNER_ID, cardUid);
  Serial.println("---- CARD SCAN ----");
  Serial.printf("cardUid: %s\nscanId: %s\n", cardUid.c_str(), scanId.c_str());

  display::show(cardLine(cardUid), "Please wait...");
  const connection::ScanResult result = connection::postScan(scanId, cardUid);

  logResult(result);
  showResultScreen(cardUid, result);
  if (result.accepted) feedback::showValid();

  delay(RESULT_DISPLAY_MS);
}

// ---------------------------------------------------------------- Arduino entry points

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== RFID Attendance Scanner ===");
  Serial.printf("Scanner ID: %s\nAPI: %s\nHeartbeat: %s\n", SCANNER_ID, API_ENDPOINT,
                HEARTBEAT_ENDPOINT);

  feedback::begin();  // red LED on as soon as we have power
  display::begin();
  showLoading();

  startRfidReader();
  showingReaderFault = !rfid_reader::isHealthy();
  connectWiFi();
  showReadyOrFaultScreen();
}

void loop() {
  checkWiFi();
  connection::keepHeartbeat();
  checkReader();
  if (showingReaderFault) return;

  String cardUid;
  if (!rfid_reader::readCard(cardUid)) return;

  feedback::beep();  // every tap is acknowledged, even repeats

  if (!isRepeatTap(cardUid)) {
    handleScan(cardUid);
    feedback::showIdle();
    showIdleScreen();
  }

  rfid_reader::releaseCard();
}
