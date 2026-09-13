/*
 * GP8212S — set a fixed current and optionally store it in NVM
 *
 * After store(), the chip keeps that output after power-cycle until overwritten.
 * store() bit-bangs a special sequence; pass the same SDA/SCL pins used in begin().
 */

#include <Wire.h>
#include <GP8212S.h>

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
static const int PIN_SDA = 21;
static const int PIN_SCL = 22;
#else
static const int PIN_SDA = -1;
static const int PIN_SCL = -1;
#endif

GP8212S dac;

void setup() {
  Serial.begin(115200);
  delay(500);

  if (dac.begin(PIN_SDA, PIN_SCL) != 0) {
    Serial.println(F("GP8212S not found"));
    while (true) delay(1000);
  }

  // 50 % of 4–20 mA span → 12 mA
  uint16_t code = dac.setPercent4_20(50.0f);
  Serial.print(F("50% → "));
  Serial.print(dac.dacTo_mA(code), 3);
  Serial.print(F(" mA (ideal), DAC=0x"));
  Serial.println(code, HEX);

  // Uncomment to freeze this value in chip EEPROM-like storage:
  // dac.store(PIN_SDA, PIN_SCL);
  // Serial.println(F("Stored."));
}

void loop() {
  // Serial commands: type milliamps then Enter, e.g. "4", "12.5", "20"
  if (Serial.available()) {
    float mA = Serial.parseFloat();
    while (Serial.available()) {
      Serial.read();
    }
    if (mA >= 0.0f && mA <= dac.fullScale_mA()) {
      uint16_t code = dac.setCurrent_mA(mA);
      Serial.print(F("Set "));
      Serial.print(mA, 3);
      Serial.print(F(" mA → DAC 0x"));
      Serial.println(code, HEX);
    }
  }
}
