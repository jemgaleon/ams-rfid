#include "connection.h"

#include <HTTPClient.h>
#include <WiFi.h>

#include "config.h"
#include "utils.h"

namespace connection {

// ---------------------------------------------------------------- Wi-Fi

static uint32_t lastRetryAtMs = 0;

// Lower = smaller current spikes but less range. Raise to WIFI_POWER_11dBm / 15dBm / 19_5dBm
// once the board has decoupling capacitors and soldered connections.
static constexpr wifi_power_t WIFI_TX_POWER = WIFI_POWER_8_5dBm;

// Full power (~19.5 dBm) pulls current spikes that brown out the RC522 on a weak supply.
// Applied after every begin(), since the driver can reset it on (re)connect.
static void startConnecting() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setTxPower(WIFI_TX_POWER);
}

void beginWiFi() {
  WiFi.mode(WIFI_STA);
  startConnecting();
  Serial.printf("[WiFi] Connecting to %s (tx power %.1f dBm)\n", WIFI_SSID,
                static_cast<int>(WiFi.getTxPower()) / 4.0f);  // driver reports quarter-dBm
}

bool isConnected() { return WiFi.status() == WL_CONNECTED; }

void keepWiFiAlive() {
  if (isConnected()) return;
  if (millis() - lastRetryAtMs < WIFI_RETRY_INTERVAL_MS) return;

  lastRetryAtMs = millis();
  Serial.println("[WiFi] Reconnecting...");
  WiFi.disconnect();
  startConnecting();
}

String ipText() { return isConnected() ? WiFi.localIP().toString() : String("No WiFi"); }

// ---------------------------------------------------------------- Heartbeat

static uint32_t lastHeartbeatAtMs = 0;

bool sendHeartbeat() {
  lastHeartbeatAtMs = millis();
  if (!isConnected()) return false;

  HTTPClient http;
  http.setTimeout(HEARTBEAT_TIMEOUT_MS);
  http.setConnectTimeout(HEARTBEAT_TIMEOUT_MS);
  if (!http.begin(HEARTBEAT_ENDPOINT)) {
    Serial.println("[HEARTBEAT] HTTP begin failed");
    return false;
  }
  http.addHeader("Content-Type", "application/json");

  const int code = http.POST(String("{\"scannerId\":\"") + SCANNER_ID + "\"}");
  http.end();

  if (code <= 0) {
    Serial.printf("[HEARTBEAT] POST failed: %s\n", HTTPClient::errorToString(code).c_str());
    return false;
  }
  // 404: AMS has no Scanner with this SCANNER_ID.
  Serial.printf("[HEARTBEAT] HTTP %d%s\n", code, code == 404 ? " (unknown scanner)" : "");
  return code >= 200 && code < 300;
}

void keepHeartbeat() {
  if (!isConnected()) return;
  if (millis() - lastHeartbeatAtMs < HEARTBEAT_INTERVAL_MS) return;
  sendHeartbeat();
}

// ---------------------------------------------------------------- Scans

static String buildPayload(const String& scanId, const String& cardUid) {
  return String("{") +
         "\"scannerId\":\"" + SCANNER_ID + "\"," +
         "\"scanId\":\"" + scanId + "\"," +
         "\"cardUid\":\"" + cardUid + "\"" +
         "}";
}

static void parseResponse(ScanResult& result) {
  const String& body = result.rawBody;
  // "valid" for Present and Late, "invalid" for the rest.
  result.accepted = utils::jsonGetString(body, "result") == "valid";
  result.name = utils::jsonGetString(body, "name");
  result.message = utils::jsonGetString(body, "message");
}

ScanResult postScan(const String& scanId, const String& cardUid) {
  ScanResult result;

  if (!isConnected()) {
    result.message = "WiFi disconnected";
    return result;
  }

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(API_ENDPOINT)) {
    result.message = "HTTP failed";
    return result;
  }
  http.addHeader("Content-Type", "application/json");

  const String payload = buildPayload(scanId, cardUid);
  Serial.printf("[HTTP] POST %s\n", API_ENDPOINT);
  Serial.printf("[HTTP] Payload: %s\n", payload.c_str());

  result.httpCode = http.POST(payload);
  if (result.httpCode <= 0) {
    Serial.printf("[HTTP] POST failed: %s\n", http.errorToString(result.httpCode).c_str());
    result.message = "POST failed";
    http.end();
    return result;
  }

  result.rawBody = http.getString();
  http.end();

  // A 400 or 500 is the server failing to answer the scan, not an answer to show.
  if (result.httpCode < 200 || result.httpCode >= 300) {
    result.message = "Server error " + String(result.httpCode);
    return result;
  }

  result.delivered = true;
  parseResponse(result);
  return result;
}

}  // namespace connection
