#pragma once

#include <Arduino.h>

// Small, hardware-independent helpers.
namespace utils {

// Shortens `text` to `maxChars`, marking the cut with "~".
String fitLine(const String& text, size_t maxChars);

// Formats raw UID bytes as uppercase hex, e.g. {0x0A, 0xFF} -> "0AFF".
String toHex(const uint8_t* bytes, size_t length);

// Builds a unique id for one scan: "<scanner>-<millis>-<random>-<cardUid>".
String buildScanId(const char* scannerId, const String& cardUid);

// Minimal JSON lookups for flat API responses (no nested objects).
// Return "" / false when the key is missing.
String jsonGetString(const String& json, const char* key);
bool jsonGetBool(const String& json, const char* key, bool& valueOut);

}  // namespace utils
