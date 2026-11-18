#include "display.h"

#include <U8g2lib.h>
#include <Wire.h>

#include "config.h"
#include "utils.h"

namespace display {

static U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// The title uses a 6px-wide font (21 chars); the body a 5px one (25 chars), so the longer
// screens ("Unable to connect server", long names, 7-byte card UIDs) fit without cutting.
static constexpr size_t TITLE_CHARS = 21;
static constexpr size_t BODY_CHARS = 25;

void begin() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  oled.setI2CAddress(OLED_I2C_ADDR << 1);
  oled.begin();
}

void show(const String& line1, const String& line2, const String& line3) {
  oled.clearBuffer();

  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(0, 10, utils::fitLine("Device ID: " + String(SCANNER_ID), TITLE_CHARS).c_str());
  oled.drawHLine(0, 12, 128);

  oled.setFont(u8g2_font_5x8_tf);
  oled.drawStr(0, 26, utils::fitLine(line1, BODY_CHARS).c_str());
  oled.drawStr(0, 40, utils::fitLine(line2, BODY_CHARS).c_str());
  oled.drawStr(0, 54, utils::fitLine(line3, BODY_CHARS).c_str());

  oled.sendBuffer();
}

}  // namespace display
