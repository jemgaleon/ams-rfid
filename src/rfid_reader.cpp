#include "rfid_reader.h"

#include <MFRC522.h>
#include <SPI.h>

#include "config.h"
#include "utils.h"

namespace rfid_reader {

static MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
static uint32_t lastHealthCheckAtMs = 0;
static bool healthy = false;

static constexpr uint8_t MAX_FAILED_READS = 10;
static uint8_t failedReads = 0;

// 0x00 / 0xFF means nothing answered on the SPI bus.
static bool isResponding(byte version) { return version != 0x00 && version != 0xFF; }

// TX1/TX2 drive the antenna in opposite phase: TX2 inverted (bit 7), both outputs on (bits 0-1).
// The library only ORs in bits 0-1, so a clone that powers up with other bits set (seen: 0x7F,
// which puts TX2 in unmodulated-carrier mode) keeps them and never talks to a card.
static constexpr byte TX_CONTROL_EXPECTED = 0x83;

static bool isAntennaOn() { return (rfid.PCD_ReadRegister(MFRC522::TxControlReg) & 0x03) == 0x03; }

static bool isTxConfigured() {
  return rfid.PCD_ReadRegister(MFRC522::TxControlReg) == TX_CONTROL_EXPECTED;
}

static void logRegisters(const char* when) {
  Serial.printf("[RFID] %s: VersionReg=0x%02X TxControlReg=0x%02X antenna=%s gain=0x%02X\n", when,
                rfid.PCD_ReadRegister(MFRC522::VersionReg),
                rfid.PCD_ReadRegister(MFRC522::TxControlReg), isAntennaOn() ? "on" : "OFF",
                rfid.PCD_GetAntennaGain());
}

// ---- debug: raw SPI write/read-back test, independent of the library's 4 MHz clock ----

static byte rawRead(uint32_t hz, byte addr) {
  SPI.beginTransaction(SPISettings(hz, MSBFIRST, SPI_MODE0));
  digitalWrite(RFID_SS_PIN, LOW);
  SPI.transfer(0x80 | (addr << 1));
  byte value = SPI.transfer(0x00);
  digitalWrite(RFID_SS_PIN, HIGH);
  SPI.endTransaction();
  return value;
}

static void rawWrite(uint32_t hz, byte addr, byte value) {
  SPI.beginTransaction(SPISettings(hz, MSBFIRST, SPI_MODE0));
  digitalWrite(RFID_SS_PIN, LOW);
  SPI.transfer((addr << 1) & 0x7E);
  SPI.transfer(value);
  digitalWrite(RFID_SS_PIN, HIGH);
  SPI.endTransaction();
}

static void spiReadBackTest() {
  const uint32_t speeds[] = {4000000, 1000000, 400000};
  const byte regs[] = {0x24 /* ModWidthReg */, 0x14 /* TxControlReg */};
  const byte patterns[] = {0x00, 0x55, 0xAA, 0x83};

  for (uint32_t hz : speeds) {
    Serial.printf("[SPI TEST] %lu Hz, VersionReg=0x%02X\n", (unsigned long)hz, rawRead(hz, 0x37));
    for (byte reg : regs) {
      const byte original = rawRead(hz, reg);
      String line = "  reg 0x" + String(reg, HEX) + " was 0x" + String(original, HEX) + ":";
      for (byte pattern : patterns) {
        rawWrite(hz, reg, pattern);
        const byte got = rawRead(hz, reg);
        line += " w" + String(pattern, HEX) + "->r" + String(got, HEX) + (got == pattern ? "" : "!");
      }
      rawWrite(hz, reg, original);
      Serial.println(line);
    }
  }
}

static void initReader() {
  rfid.PCD_Init();
  rfid.PCD_WriteRegister(MFRC522::TxControlReg, TX_CONTROL_EXPECTED);
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);  // clone chips often need the extra range
}

bool begin() {
  SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
  initReader();
  if (RFID_DEBUG) {
    spiReadBackTest();
    initReader();  // the test leaves registers as it found them, but start clean anyway
  }

  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.printf("[RFID] VersionReg = 0x%02X\n", version);
  rfid.PCD_DumpVersionToSerial();
  if (RFID_DEBUG) logRegisters("after init");

  healthy = isResponding(version);
  return healthy;
}

bool isHealthy() { return healthy; }

// A supply dip (e.g. Wi-Fi starting) can reset the reader, which turns its antenna off
// while SPI keeps working. Re-initialise when that happens.
void checkHealth() {
  if (millis() - lastHealthCheckAtMs < RFID_HEALTH_CHECK_MS) return;
  lastHealthCheckAtMs = millis();

  if (RFID_DEBUG) logRegisters("health");

  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  if (!isResponding(version) || !isTxConfigured()) {
    Serial.println("[RFID] Reader reset or not responding - re-initialising");
    initReader();
    version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  }
  healthy = isResponding(version);
}

// The USB desk reader used to register cards types a 4-byte UID as a decimal number with the
// bytes reversed (0A4069E7 -> 3882434570). Send the same form so scans match registered cards.
// Longer UIDs (7/10 bytes) have no such convention, so they stay hex.
static String uidText(const MFRC522::Uid& uid) {
  if (!CARD_UID_AS_DECIMAL || uid.size != 4) return utils::toHex(uid.uidByte, uid.size);

  uint32_t value = 0;
  for (int i = 3; i >= 0; i--) value = (value << 8) | uid.uidByte[i];
  return String(value);
}

bool readCard(String& uidOut) {
  if (!rfid.PICC_IsNewCardPresent()) return false;

  if (!rfid.PICC_ReadCardSerial()) {
    // Seen a card's answer but couldn't read its UID. A few in a row is a bad tap; a long run
    // means the reader is in a bad state (e.g. after a supply dip), so restart it.
    if (++failedReads >= MAX_FAILED_READS) {
      Serial.printf("[RFID] %u UID reads failed in a row - re-initialising\n", failedReads);
      failedReads = 0;
      initReader();
    }
    return false;
  }

  failedReads = 0;
  uidOut = uidText(rfid.uid);
  if (RFID_DEBUG) {
    Serial.printf("[RFID] Read %s (type: %s)\n", uidOut.c_str(),
                  (const char*)MFRC522::PICC_GetTypeName(rfid.PICC_GetType(rfid.uid.sak)));
  }
  return true;
}

void releaseCard() {
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

}  // namespace rfid_reader
