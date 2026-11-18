#pragma once

#include <Arduino.h>

// 128x64 SSD1306 OLED: a fixed title ("Device ID: <SCANNER_ID>") plus three body lines.
namespace display {

void begin();
void show(const String& line1, const String& line2 = "", const String& line3 = "");

}  // namespace display
