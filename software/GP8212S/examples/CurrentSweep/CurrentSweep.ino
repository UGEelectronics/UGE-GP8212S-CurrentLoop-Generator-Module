/*
 * GP8212S — basic current sweep (Arduino Uno / Mega / Nano / ESP32)
 *
 * Wiring (this manufacturing board, typical):
 *   MCU 3.3 V or 5 V  →  logic side only if level-shifted; chip VCC is 9–36 V
 *   SDA  → GP8212S SDA   (ESP32 default often GPIO21)
 *   SCL  → GP8212S SCL   (ESP32 default often GPIO22)
 *   GND  → common ground with the loop supply return
 *
 * Power the GP8212S VCC from 9–36 V (24 V recommended). Do not power VCC from USB 5 V.
 *
 * Install: copy the GP8212S folder into Documents/Arduino/libraries/
 *   or Sketch → Include Library → Add .ZIP Library
 */

#include <Wire.h>
#include <GP8212S.h>

// ---- Pin overrides for ESP32 (leave -1 for board defaults / AVR) ----
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
static const int PIN_SDA = 21;
static const int PIN_SCL = 22;
#else
static const int PIN_SDA = -1;
static const int PIN_SCL = -1;
#endif

GP8212S dac;  // address 0x58, Wire, 25 mA full scale (Rs = 100 Ω)

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("GP8212S init..."));
  int err = dac.begin(PIN_SDA, PIN_SCL, 100000);
  if (err != 0) {
    Serial.print(F("I2C probe failed, Wire error "));
    Serial.println(err);
    Serial.println(F("Check wiring, address, and that chip VCC is powered."));
    while (true) {
      delay(1000);
    }
  }
  Serial.println(F("OK"));

  // Optional: after measuring real 4 mA / 20 mA DAC codes on your board:
  // dac.calibrate4_20(5200, 26200);

  uint16_t code = dac.setCurrent_mA(12.0f);  // mid-ish of 4–20 span
  Serial.print(F("12 mA → DAC 0x"));
  Serial.println(code, HEX);
}

void loop() {
  // Sweep 4 → 20 → 4 mA in 1 mA steps
  for (float mA = 4.0f; mA <= 20.0f; mA += 1.0f) {
    uint16_t code = dac.setCurrent_mA(mA);
    Serial.print(mA, 1);
    Serial.print(F(" mA  DAC=0x"));
    Serial.println(code, HEX);
    delay(1000);
  }
  for (float mA = 19.0f; mA >= 4.0f; mA -= 1.0f) {
    uint16_t code = dac.setCurrent_mA(mA);
    Serial.print(mA, 1);
    Serial.print(F(" mA  DAC=0x"));
    Serial.println(code, HEX);
    delay(1000);
  }
}
