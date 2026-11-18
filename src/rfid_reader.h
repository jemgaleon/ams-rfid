#pragma once

#include <Arduino.h>

// MFRC522 reader on the VSPI bus.
namespace rfid_reader {

// Initialises the reader. Returns false if the chip does not respond.
bool begin();

// Call every loop: re-initialises the reader if a power dip reset it.
void checkHealth();

// False while the reader doesn't answer, even after re-initialising.
bool isHealthy();

// Returns true and fills `uidOut` when a new card has been read.
bool readCard(String& uidOut);

// Releases the current card so the next tap is detected.
void releaseCard();

}  // namespace rfid_reader
