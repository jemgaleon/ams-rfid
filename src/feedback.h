#pragma once

// Status LEDs and buzzer.
//
//   State    | Red | Green
//   ---------+-----+------
//   idle     | on  | off      (also shown for denied / failed scans)
//   valid    | off | on
//
// The buzzer beeps once for every card scan.
namespace feedback {

void begin();
void showIdle();
void showValid();
void beep();

}  // namespace feedback
