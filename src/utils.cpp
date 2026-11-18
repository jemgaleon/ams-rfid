#include "utils.h"

#include <ctype.h>
#include <esp_system.h>

namespace utils {

String fitLine(const String& text, size_t maxChars) {
  if (text.length() <= maxChars) return text;
  if (maxChars < 2) return text.substring(0, maxChars);
  return text.substring(0, maxChars - 1) + "~";
}

String toHex(const uint8_t* bytes, size_t length) {
  String out;
  for (size_t i = 0; i < length; i++) {
    if (bytes[i] < 0x10) out += "0";
    out += String(bytes[i], HEX);
  }
  out.toUpperCase();
  return out;
}

String buildScanId(const char* scannerId, const String& cardUid) {
  return String(scannerId) + "-" + String(millis()) + "-" +
         String(esp_random(), HEX) + "-" + cardUid;
}

// Reads a quoted string value starting at the opening quote, handling \" escapes.
static String readQuoted(const String& json, int quotePos) {
  String out;
  bool escaped = false;
  for (int i = quotePos + 1; i < (int)json.length(); i++) {
    char c = json[i];
    if (escaped) {
      out += c;
      escaped = false;
    } else if (c == '\\') {
      escaped = true;
    } else if (c == '"') {
      break;
    } else {
      out += c;
    }
  }
  return out;
}

// Reads a bare value (number, true/false, null) up to the next ',' or '}'.
static String readBare(const String& json, int start) {
  int end = start;
  while (end < (int)json.length() && json[end] != ',' && json[end] != '}') end++;
  String value = json.substring(start, end);
  value.trim();
  return value;
}

String jsonGetString(const String& json, const char* key) {
  const String token = "\"" + String(key) + "\"";
  int keyPos = json.indexOf(token);
  if (keyPos < 0) return "";

  int colonPos = json.indexOf(':', keyPos + token.length());
  if (colonPos < 0) return "";

  int valuePos = colonPos + 1;
  while (valuePos < (int)json.length() && isspace((unsigned char)json[valuePos])) valuePos++;
  if (valuePos >= (int)json.length()) return "";

  return json[valuePos] == '"' ? readQuoted(json, valuePos) : readBare(json, valuePos);
}

bool jsonGetBool(const String& json, const char* key, bool& valueOut) {
  String value = jsonGetString(json, key);
  value.toLowerCase();

  if (value == "true" || value == "1") {
    valueOut = true;
    return true;
  }
  if (value == "false" || value == "0") {
    valueOut = false;
    return true;
  }
  return false;
}

}  // namespace utils
