#pragma once

#include <Arduino.h>

// Wi-Fi connection and the AMS API.
namespace connection {

// ---- Wi-Fi ----

// Starts connecting. Use isConnected() to check progress.
void beginWiFi();
bool isConnected();
// Retries the connection every WIFI_RETRY_INTERVAL_MS while disconnected.
void keepWiFiAlive();
// Local IP, or "No WiFi".
String ipText();

// ---- Heartbeat ----

// POSTs `{ scannerId }` so AMS shows this Scanner Online. Returns true on a 2xx answer.
bool sendHeartbeat();
// Sends a heartbeat every HEARTBEAT_INTERVAL_MS while connected.
void keepHeartbeat();

// ---- Attendance API ----

// AMS answers a scan `{ result, name?, message }`.
struct ScanResult {
  bool delivered = false;  // the server answered with a 2xx status
  int httpCode = 0;
  bool accepted = false;   // `result` is "valid": green LED on, red off
  String name;             // the Student's name, if the card is theirs
  String message;          // server message, or why the server couldn't be reached
  String rawBody;
};

ScanResult postScan(const String& scanId, const String& cardUid);

}  // namespace connection
