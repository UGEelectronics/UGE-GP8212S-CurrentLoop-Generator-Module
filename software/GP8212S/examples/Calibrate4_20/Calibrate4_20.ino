/*
 * GP8212S — two-point 4 mA / 20 mA calibration helper
 *
 * Hardware (this board):
 *   - Power module from 5 V USB/bench → MT3608 boosts to ~12 V for GP8212S VCC
 *   - Connect I2C SDA/SCL/GND to MCU
 *   - Put a known load on the loop (220–330 Ω recommended at 12 V supply)
 *   - Measure loop current with a DMM in series, OR measure Vload and compute
 *     I = Vload / Rload
 *
 * Serial commands (open Serial Monitor @ 115200, "No line ending" or "Newline"):
 *   + / -     coarse step DAC  (±50)
 *   ] / [     fine step DAC    (±5)
 *   4         jump near ideal 4 mA code
 *   2         jump near ideal 20 mA code
 *   s4        save current DAC as "4 mA" calibration point
 *   s20       save current DAC as "20 mA" calibration point
 *   apply     print the calibrate4_20(...) line to paste into your sketch
 *   g <mA>    after both points saved, go to that current using calibration
 *   ?         help
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

// Ideal codes for Rs=100 Ω, 0–25 mA full scale (starting hints only)
static const uint16_t IDEAL_4MA  = 5243;   // 4/25 * 32767
static const uint16_t IDEAL_20MA = 26214;  // 20/25 * 32767

GP8212S dac;
uint16_t code = IDEAL_4MA;
uint16_t saved4 = 0;
uint16_t saved20 = 0;
bool have4 = false;
bool have20 = false;

String line;

void printHelp() {
  Serial.println(F("--- GP8212S calibration ---"));
  Serial.println(F("1) Power 5V into board (MT3608 → ~12V). Confirm ~12V on VCC."));
  Serial.println(F("2) Load IOUT with 220..330 ohm (12V limited compliance)."));
  Serial.println(F("3) DMM in series on mA range, or V across Rload → I=V/R."));
  Serial.println(F("4) Use +/- and ]/[ to trim until meter reads 4.000 mA, then 's4'."));
  Serial.println(F("5) Jump with '2', trim to 20.000 mA, then 's20'."));
  Serial.println(F("6) Type 'apply' and paste the printed line into your firmware."));
  Serial.println(F("Cmds: + - ] [  4  2  s4  s20  apply  g <mA>  ?"));
}

void applyCode(uint16_t c) {
  code = c;
  if (code > GP8212S::DAC_MAX) code = GP8212S::DAC_MAX;
  dac.setDAC(code);
  Serial.print(F("DAC=0x"));
  Serial.print(code, HEX);
  Serial.print(F(" ("));
  Serial.print(code);
  Serial.print(F(")  ideal~"));
  Serial.print(dac.dacTo_mA(code), 3);
  Serial.println(F(" mA"));
}

void setup() {
  Serial.begin(115200);
  delay(500);

  if (dac.begin(PIN_SDA, PIN_SCL) != 0) {
    Serial.println(F("GP8212S not found on I2C — check wiring / 5V power / address 0x58"));
    while (true) delay(1000);
  }

  printHelp();
  applyCode(IDEAL_4MA);
}

void loop() {
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\r') continue;
    if (ch != '\n') {
      line += ch;
      continue;
    }

    line.trim();
    if (line.length() == 0) {
      line = "";
      continue;
    }

    if (line == "?") {
      printHelp();
    } else if (line == "+") {
      applyCode(code + 50);
    } else if (line == "-") {
      applyCode(code > 50 ? code - 50 : 0);
    } else if (line == "]") {
      applyCode(code + 5);
    } else if (line == "[") {
      applyCode(code > 5 ? code - 5 : 0);
    } else if (line == "4") {
      applyCode(have4 ? saved4 : IDEAL_4MA);
    } else if (line == "2") {
      applyCode(have20 ? saved20 : IDEAL_20MA);
    } else if (line == "s4") {
      saved4 = code;
      have4 = true;
      Serial.print(F("Saved 4 mA point: "));
      Serial.println(saved4);
    } else if (line == "s20") {
      saved20 = code;
      have20 = true;
      Serial.print(F("Saved 20 mA point: "));
      Serial.println(saved20);
    } else if (line == "apply") {
      if (!have4 || !have20) {
        Serial.println(F("Need both s4 and s20 first."));
      } else if (saved4 >= saved20) {
        Serial.println(F("Invalid: 4 mA code must be < 20 mA code."));
      } else {
        Serial.println(F("Paste into setup() after begin():"));
        Serial.print(F("  dac.calibrate4_20("));
        Serial.print(saved4);
        Serial.print(F(", "));
        Serial.print(saved20);
        Serial.println(F(");"));
        dac.calibrate4_20(saved4, saved20);
        Serial.println(F("Calibration applied in this session too."));
      }
    } else if (line.startsWith("g ")) {
      if (!have4 || !have20) {
        Serial.println(F("Save s4 and s20, then 'apply', before g <mA>."));
      } else {
        float mA = line.substring(2).toFloat();
        dac.calibrate4_20(saved4, saved20);
        uint16_t c = dac.setCurrent_mA(mA);
        code = c;
        Serial.print(F("Target "));
        Serial.print(mA, 3);
        Serial.print(F(" mA → DAC "));
        Serial.println(c);
      }
    } else {
      // raw decimal DAC code
      long v = line.toInt();
      if (v >= 0 && v <= GP8212S::DAC_MAX) {
        applyCode((uint16_t)v);
      } else {
        Serial.println(F("Unknown cmd. Type ?"));
      }
    }

    line = "";
  }
}
