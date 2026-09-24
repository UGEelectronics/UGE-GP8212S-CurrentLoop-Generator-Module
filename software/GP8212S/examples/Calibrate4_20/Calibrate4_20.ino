/*
 * GP8212S — two-point 4 mA / 20 mA calibration helper
 *
 * Simple workflow (Serial Monitor @ 115200, Newline):
 *   1) Upload this sketch, wire module to Arduino (see docs/images/arduino-wiring.jpg)
 *   2) Put ~330 Ω (1 W preferred) on the loop output; measure current with a DMM
 *   3) Type:  4
 *      Read meter (e.g. 8.08 mA) → type:  M 8.08
 *   4) Type:  20
 *      Read meter (e.g. 25.63 mA) → type:  M 25.63
 *   5) Type:  apply
 *      Copy dac.calibrate4_20(...) into your future sketches (once per board)
 *
 * M <reading> rescales the DAC toward the current target (4 or 20 mA) and
 * auto-saves that calibration point. Commands are case-insensitive.
 *
 * Extra: +++ --- (±2000)  ++ -- (±500)  + - (±50)  ] [ (±5)  c <code>  g <mA>  ?
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

static const uint16_t IDEAL_4MA  = 5243;   // 4/25 * 32767 (Rs=100 Ω hint)
static const uint16_t IDEAL_20MA = 26214;  // 20/25 * 32767

GP8212S dac;
uint16_t code = IDEAL_4MA;
uint16_t saved4 = 0;
uint16_t saved20 = 0;
bool have4 = false;
bool have20 = false;
float target_mA = 4.0f;

String line;

static uint16_t clampCode(long c) {
  if (c < 0) return 0;
  if (c > (long)GP8212S::DAC_MAX) return GP8212S::DAC_MAX;
  return (uint16_t)c;
}

static void toLowerInPlace(String &s) {
  for (unsigned i = 0; i < s.length(); i++) {
    s[i] = (char)tolower(s[i]);
  }
}

void printHelp() {
  Serial.println(F("--- GP8212S calibration ---"));
  Serial.println(F("Load: ~330 ohm (1W preferred) on IOUT. DMM in series (mA)."));
  Serial.println(F("Serial: 115200, Newline."));
  Serial.println();
  Serial.println(F("Steps:"));
  Serial.println(F("  4"));
  Serial.println(F("  M <meter_mA>     e.g. M 8.08   (auto-saves 4 mA point)"));
  Serial.println(F("  20"));
  Serial.println(F("  M <meter_mA>     e.g. M 25.63  (auto-saves 20 mA point)"));
  Serial.println(F("  apply            → copy dac.calibrate4_20(...) into your code"));
  Serial.println();
  Serial.println(F("Optional trim: +++ --- ++ -- + - ] [   |  c <dac>  g <mA>  ?"));
}

void applyCode(uint16_t c) {
  code = clampCode(c);
  dac.setDAC(code);
  Serial.print(F("DAC="));
  Serial.print(code);
  Serial.print(F(" (0x"));
  Serial.print(code, HEX);
  Serial.print(F(")  ideal~"));
  Serial.print(dac.dacTo_mA(code), 3);
  Serial.print(F(" mA | target="));
  Serial.print(target_mA, 1);
  Serial.println(F(" mA"));
}

void stepCode(long delta) {
  applyCode(clampCode((long)code + delta));
}

void savePointIfTarget() {
  if (target_mA >= 3.5f && target_mA <= 4.5f) {
    saved4 = code;
    have4 = true;
    Serial.print(F("Saved 4 mA calibration point: "));
    Serial.println(saved4);
  } else if (target_mA >= 19.0f && target_mA <= 21.0f) {
    saved20 = code;
    have20 = true;
    Serial.print(F("Saved 20 mA calibration point: "));
    Serial.println(saved20);
  }
}

void rescaleFromMeter(float measured_mA) {
  if (measured_mA <= 0.01f) {
    Serial.println(F("Measured mA must be > 0.01"));
    return;
  }
  if (code == 0) {
    Serial.println(F("DAC is 0 — type 4 or 20 first."));
    return;
  }

  // I ∝ DAC  →  new = old * (want / got)
  long next = (long)((double)code * (double)target_mA / (double)measured_mA + 0.5);
  uint16_t before = code;
  applyCode(clampCode(next));
  Serial.print(F("Rescaled meter "));
  Serial.print(measured_mA, 3);
  Serial.print(F(" mA → "));
  Serial.print(target_mA, 1);
  Serial.print(F(" mA  (DAC "));
  Serial.print(before);
  Serial.print(F(" → "));
  Serial.print(code);
  Serial.println(F(")"));

  savePointIfTarget();

  if (have4 && have20) {
    Serial.println(F("Both points saved. Type: apply"));
  } else if (have4 && !have20) {
    Serial.println(F("Next: type 20 , read meter, then M <reading>"));
  }
}

void jump4() {
  target_mA = 4.0f;
  applyCode(have4 ? saved4 : IDEAL_4MA);
  Serial.println(F("Read the meter, then type: M <reading>   e.g. M 8.08"));
}

void jump20() {
  target_mA = 20.0f;
  applyCode(have20 ? saved20 : IDEAL_20MA);
  Serial.println(F("Read the meter, then type: M <reading>   e.g. M 25.63"));
}

void setup() {
  Serial.begin(115200);
  delay(500);

  if (dac.begin(PIN_SDA, PIN_SCL) != 0) {
    Serial.println(F("GP8212S not found — check wiring / 5V power / address 0x58"));
    while (true) delay(1000);
  }

  printHelp();
  jump4();
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

    String cmd = line;
    toLowerInPlace(cmd);

    if (cmd == "?") {
      printHelp();
    } else if (cmd == "+++") {
      stepCode(+2000);
    } else if (cmd == "---") {
      stepCode(-2000);
    } else if (cmd == "++") {
      stepCode(+500);
    } else if (cmd == "--") {
      stepCode(-500);
    } else if (cmd == "+") {
      stepCode(+50);
    } else if (cmd == "-") {
      stepCode(-50);
    } else if (cmd == "]") {
      stepCode(+5);
    } else if (cmd == "[") {
      stepCode(-5);
    } else if (cmd == "4") {
      jump4();
    } else if (cmd == "20" || cmd == "2") {
      jump20();
    } else if (cmd.startsWith("t ")) {
      float t = cmd.substring(2).toFloat();
      if (t > 0.0f && t <= dac.fullScale_mA()) {
        target_mA = t;
        Serial.print(F("Target "));
        Serial.print(target_mA, 3);
        Serial.println(F(" mA — type M <meter_reading>"));
      } else {
        Serial.println(F("Bad target. Example: t 20"));
      }
    } else if (cmd.startsWith("m ")) {
      rescaleFromMeter(cmd.substring(2).toFloat());
    } else if (cmd.startsWith("c ")) {
      applyCode(clampCode(cmd.substring(2).toInt()));
    } else if (cmd == "s4") {
      saved4 = code;
      have4 = true;
      Serial.print(F("Saved 4 mA point: "));
      Serial.println(saved4);
    } else if (cmd == "s20") {
      saved20 = code;
      have20 = true;
      Serial.print(F("Saved 20 mA point: "));
      Serial.println(saved20);
    } else if (cmd == "apply") {
      if (!have4 || !have20) {
        Serial.println(F("Need both points first (4 → M … then 20 → M …)."));
      } else if (saved4 >= saved20) {
        Serial.println(F("Invalid: 4 mA code must be < 20 mA code. Re-run calibration."));
      } else {
        Serial.println(F("Paste into setup() after begin():"));
        Serial.print(F("  dac.calibrate4_20("));
        Serial.print(saved4);
        Serial.print(F(", "));
        Serial.print(saved20);
        Serial.println(F(");"));
        dac.calibrate4_20(saved4, saved20);
        Serial.println(F("Done. Re-use those numbers in every future sketch for this board."));
        Serial.println(F("You do NOT need to recalibrate each power-on."));
      }
    } else if (cmd.startsWith("g ")) {
      if (!have4 || !have20) {
        Serial.println(F("Finish 4 / 20 / apply first."));
      } else {
        float mA = cmd.substring(2).toFloat();
        dac.calibrate4_20(saved4, saved20);
        uint16_t c = dac.setCurrent_mA(mA);
        code = c;
        Serial.print(F("setCurrent_mA("));
        Serial.print(mA, 3);
        Serial.print(F(") → DAC "));
        Serial.println(c);
      }
    } else {
      bool numeric = true;
      for (unsigned i = 0; i < cmd.length(); i++) {
        if (cmd[i] < '0' || cmd[i] > '9') {
          numeric = false;
          break;
        }
      }
      if (numeric) {
        applyCode(clampCode(cmd.toInt()));
      } else {
        Serial.println(F("Unknown cmd. Type ?"));
      }
    }

    line = "";
  }
}
