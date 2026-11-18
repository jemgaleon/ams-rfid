#include "feedback.h"

#include <Arduino.h>

#include "config.h"

namespace feedback {

static void setLeds(bool redOn, bool greenOn) {
  digitalWrite(RED_LED_PIN, redOn ? HIGH : LOW);
  digitalWrite(GREEN_LED_PIN, greenOn ? HIGH : LOW);
}

void begin() {
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  showIdle();
}

void showIdle() { setLeds(true, false); }

void showValid() { setLeds(false, true); }

void beep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(BEEP_MS);
  digitalWrite(BUZZER_PIN, LOW);
}

}  // namespace feedback
