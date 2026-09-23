/*
 * GP8212S — two-point 4 mA / 20 mA calibration helper
 *
 * Fast path when the meter is way off the ideal code (e.g. ideal "20 mA"
 * code makes ~25.7 mA): type the measured value once —
 *
 *   t 20
 *   m 25.72
 *
 * That rescales the DAC in one step: newCode = code * (target / measured).
 * Then fine-trim with [ ] and save with s20.
 *
 * Serial Monitor @ 115200, line ending: Newline (or Both NL & CR).
 *
 * Commands:
 *   t 4 | t 20     set calibration target (default 4 at start)
 *   m <mA>         rescale DAC so output moves toward current target
 *                  example: meter shows 25.72 while targeting 20 → "m 25.72"
 *   +++ / ---      huge step  (±2000)
 *   ++ / --        large step (±500)
 *   + / -          medium     (±50)
 *   ] / [          fine       (±5)
 *   4 | 2          jump to ideal (or last saved) 4 mA / 20 mA code
 *   c <code>       set raw DAC 0..32767
 *   s4 | s20       save current DAC as 4 mA / 20 mA point
 *   apply          print calibrate4_20(...) for your sketch
 *   g <mA>         after apply, go to that current using calibration
 *   ?              help
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
float target_mA = 4.0f;  // used by "m <reading>"

String line;

static uint16_t clampCode(long c) {
  if (c < 0) return 0;
  if (c > (long)GP8212S::DAC_MAX) return GP8212S::DAC_MAX;
  return (uint16_t)c;
}

void printHelp() {
  Serial.println(F("--- GP8212S calibration ---"));
  Serial.println(F("Setup: 5V in (MT3608→~12V), I2C, load 220..330Ω, DMM on mA."));
  Serial.println();
  Serial.println(F("FAST calibrate (recommended):"));
  Serial.println(F("  t 4          then trim / or:  m <meter_mA>"));
  Serial.println(F("  s4"));
  Serial.println(F("  t 20"));
  Serial.println(F("  2            (jump near 20 mA code)"));
  Serial.println(F("  m 25.72      (if meter reads 25.72 — one-shot rescale!)"));
  Serial.println(F("  [ ]          fine trim to 20.000"));
  Serial.println(F("  s20"));
  Serial.println(F("  apply"));
  Serial.println();
  Serial.println(F("Steps: +++ --- (±2000)  ++ -- (±500)  + - (±50)  ] [ (±5)"));
  Serial.println(F("Other: 4  2  c <code>  g <mA>  ?"));
  Serial.print(F("Current target = "));
  Serial.print(target_mA, 1);
  Serial.println(F(" mA  (change with t 4 / t 20)"));
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

void rescaleFromMeter(float measured_mA) {
  if (measured_mA <= 0.01f) {
    Serial.println(F("Measured mA must be > 0.01"));
    return;
  }
  if (code == 0) {
    Serial.println(F("DAC is 0 — set a non-zero code first (try 2 or 4)."));
    return;
  }

  // I ∝ DAC  →  new = old * (want / got)
  long next = (long)((double)code * (double)target_mA / (double)measured_mA + 0.5);
  uint16_t before = code;
  applyCode(clampCode(next));
  Serial.print(F("Rescaled from meter "));
  Serial.print(measured_mA, 3);
  Serial.print(F(" mA → want "));
  Serial.print(target_mA, 1);
  Serial.print(F(" mA  ("));
  Serial.print(before);
  Serial.print(F(" → "));
  Serial.print(code);
  Serial.println(F(")"));
  Serial.println(F("Check meter, then fine-trim with [ ] and save s4/s20."));
}

void setup() {
  Serial.begin(115200);
  delay(500);

  if (dac.begin(PIN_SDA, PIN_SCL) != 0) {
    Serial.println(F("GP8212S not found on I2C — check wiring / 5V power / address 0x58"));
    while (true) delay(1000);
  }

  printHelp();
  target_mA = 4.0f;
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
    } else if (line == "+++") {
      stepCode(+2000);
    } else if (line == "---") {
      stepCode(-2000);
    } else if (line == "++") {
      stepCode(+500);
    } else if (line == "--") {
      stepCode(-500);
    } else if (line == "+") {
      stepCode(+50);
    } else if (line == "-") {
      stepCode(-50);
    } else if (line == "]") {
      stepCode(+5);
    } else if (line == "[") {
      stepCode(-5);
    } else if (line == "4") {
      target_mA = 4.0f;
      applyCode(have4 ? saved4 : IDEAL_4MA);
    } else if (line == "2") {
      target_mA = 20.0f;
      // Start a bit under ideal — many boards overshoot at the textbook code
      applyCode(have20 ? saved20 : (uint16_t)((IDEAL_20MA * 4L) / 5L));  // ~80% of ideal ≈ 16 mA hint
      Serial.println(F("Hint: if meter >> 20 mA, type: m <your_reading>"));
    } else if (line.startsWith("t ")) {
      float t = line.substring(2).toFloat();
      if (t > 0.0f && t <= dac.fullScale_mA()) {
        target_mA = t;
        Serial.print(F("Target set to "));
        Serial.print(target_mA, 3);
        Serial.println(F(" mA — now type m <meter_reading>"));
      } else {
        Serial.println(F("Bad target. Example: t 20"));
      }
    } else if (line.startsWith("m ")) {
      float measured = line.substring(2).toFloat();
      rescaleFromMeter(measured);
    } else if (line.startsWith("c ")) {
      applyCode(clampCode(line.substring(2).toInt()));
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
      // bare number = raw DAC code
      bool numeric = true;
      for (unsigned i = 0; i < line.length(); i++) {
        if (line[i] < '0' || line[i] > '9') {
          numeric = false;
          break;
        }
      }
      if (numeric) {
        applyCode(clampCode(line.toInt()));
      } else {
        Serial.println(F("Unknown cmd. Type ?"));
      }
    }

    line = "";
  }
}
