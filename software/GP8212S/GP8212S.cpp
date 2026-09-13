/*!
 * @file GP8212S.cpp
 * @brief Arduino / ESP32 driver for Linearin GP8212S
 */

#include "GP8212S.h"

const float GP8212S::FULL_SCALE_MA = 25.0f;

// Register / store timing constants (same GP8xxx family protocol as DFRobot)
static const uint8_t REG_DAC = 0x02;
static const uint8_t STORE_HEAD = 0x02;   // 3-bit preamble
static const uint8_t STORE_ADDR = 0x10;
static const uint8_t STORE_CMD1 = 0x03;
static const uint8_t STORE_CMD2 = 0x00;
static const uint8_t STORE_DELAY_MS = 10;

static const uint8_t I2C_HALF_US = 5;

GP8212S::GP8212S(uint8_t deviceAddr, TwoWire *wire, float fullScaleMa)
    : _wire(wire),
      _addr(deviceAddr),
      _fullScaleMa(fullScaleMa),
      _lastDac(0),
      _calEnabled(false),
      _cal4(0),
      _cal20(0),
      _sdaPin(-1),
      _sclPin(-1) {}

int GP8212S::begin(int sda, int scl, uint32_t freq) {
  _sdaPin = sda;
  _sclPin = scl;

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_ESP32)
  if (sda >= 0 && scl >= 0) {
    _wire->begin(sda, scl, freq);
  } else {
    _wire->begin();
    _wire->setClock(freq);
  }
#else
  (void)sda;
  (void)scl;
  _wire->begin();
  _wire->setClock(freq);
#endif

  _wire->beginTransmission(_addr);
  return _wire->endTransmission();
}

bool GP8212S::isConnected() {
  _wire->beginTransmission(_addr);
  return _wire->endTransmission() == 0;
}

uint16_t GP8212S::mAToDAC(float mA) const {
  if (mA <= 0.0f) {
    return 0;
  }
  if (mA >= _fullScaleMa) {
    return DAC_MAX;
  }
  return (uint16_t)((mA / _fullScaleMa) * (float)DAC_MAX + 0.5f);
}

float GP8212S::dacTo_mA(uint16_t code) const {
  if (code > DAC_MAX) {
    code = DAC_MAX;
  }
  return ((float)code / (float)DAC_MAX) * _fullScaleMa;
}

void GP8212S::calibrate4_20(uint16_t dacAt4mA, uint16_t dacAt20mA) {
  if (dacAt4mA >= dacAt20mA || dacAt20mA > DAC_MAX) {
    _calEnabled = false;
    return;
  }
  _cal4 = dacAt4mA;
  _cal20 = dacAt20mA;
  _calEnabled = true;
}

void GP8212S::clearCalibration() {
  _calEnabled = false;
}

bool GP8212S::writeDACRegister(uint16_t shifted16) {
  // Protocol: START | ADDR+W | REG=0x02 | DATA_LOW | DATA_HIGH | STOP
  // 15-bit payload is left-shifted by 1 before the two data bytes.
  uint8_t lo = (uint8_t)(shifted16 & 0xFF);
  uint8_t hi = (uint8_t)(shifted16 >> 8);

  _wire->beginTransmission(_addr);
  _wire->write(REG_DAC);
  _wire->write(lo);
  _wire->write(hi);
  return _wire->endTransmission() == 0;
}

bool GP8212S::setDAC(uint16_t code) {
  if (code > DAC_MAX) {
    code = DAC_MAX;
  }
  _lastDac = code;
  // Same packing as GP8211S / GP8xxx 15-bit parts
  return writeDACRegister((uint16_t)(code << 1));
}

uint16_t GP8212S::setCurrent_mA(float mA) {
  uint16_t code;

  if (_calEnabled && mA >= 4.0f && mA <= 20.0f) {
    // Linear map 4–20 mA onto measured DAC codes
    float t = (mA - 4.0f) / 16.0f;
    code = (uint16_t)(_cal4 + t * (float)(_cal20 - _cal4) + 0.5f);
  } else {
    code = mAToDAC(mA);
  }

  if (!setDAC(code)) {
    return 0xFFFF;
  }
  return code;
}

uint16_t GP8212S::setPercent4_20(float percent) {
  if (percent < 0.0f) {
    percent = 0.0f;
  }
  if (percent > 100.0f) {
    percent = 100.0f;
  }
  return setCurrent_mA(4.0f + percent * 0.16f);
}

// ---- NVM store (datasheet §3.2.4) — bit-bang on the same pins ----

void GP8212S::storeStart(int scl, int sda) {
  digitalWrite(scl, HIGH);
  digitalWrite(sda, HIGH);
  delayMicroseconds(I2C_HALF_US);
  digitalWrite(sda, LOW);
  delayMicroseconds(I2C_HALF_US);
  digitalWrite(scl, LOW);
  delayMicroseconds(I2C_HALF_US);
}

void GP8212S::storeStop(int scl, int sda) {
  digitalWrite(sda, LOW);
  delayMicroseconds(I2C_HALF_US);
  digitalWrite(scl, HIGH);
  delayMicroseconds(I2C_HALF_US);
  digitalWrite(sda, HIGH);
  delayMicroseconds(I2C_HALF_US);
}

uint8_t GP8212S::storeSendByte(int scl, int sda, uint8_t data, uint8_t bits,
                               bool expectAck) {
  for (int i = bits - 1; i >= 0; i--) {
    digitalWrite(sda, (data & (1 << i)) ? HIGH : LOW);
    delayMicroseconds(I2C_HALF_US);
    digitalWrite(scl, HIGH);
    delayMicroseconds(I2C_HALF_US);
    digitalWrite(scl, LOW);
    delayMicroseconds(I2C_HALF_US);
  }

  if (!expectAck) {
    digitalWrite(sda, LOW);
    digitalWrite(scl, HIGH);
    delayMicroseconds(I2C_HALF_US);
    digitalWrite(scl, LOW);
    return 0;
  }

  pinMode(sda, INPUT_PULLUP);
  delayMicroseconds(I2C_HALF_US);
  digitalWrite(scl, HIGH);
  delayMicroseconds(I2C_HALF_US);
  uint8_t ack = digitalRead(sda);
  digitalWrite(scl, LOW);
  delayMicroseconds(I2C_HALF_US);
  pinMode(sda, OUTPUT);
  return ack;
}

void GP8212S::store(int sda, int scl) {
  if (sda < 0) {
    sda = _sdaPin;
  }
  if (scl < 0) {
    scl = _sclPin;
  }
  if (sda < 0 || scl < 0) {
    // Fall back to Arduino default macros when pins were never passed to begin()
#if defined(SDA) && defined(SCL)
    sda = SDA;
    scl = SCL;
#else
    return;
#endif
  }

#if defined(ESP32)
  _wire->~TwoWire();
#elif !defined(ESP8266)
  _wire->end();
#endif

  pinMode(scl, OUTPUT);
  pinMode(sda, OUTPUT);
  digitalWrite(scl, HIGH);
  digitalWrite(sda, HIGH);

  // Sequence from datasheet / GP8xxx store timing
  storeStart(scl, sda);
  storeSendByte(scl, sda, STORE_HEAD, 3, false);
  storeStop(scl, sda);

  storeStart(scl, sda);
  storeSendByte(scl, sda, STORE_ADDR);
  storeSendByte(scl, sda, STORE_CMD1);
  storeStop(scl, sda);

  storeStart(scl, sda);
  storeSendByte(scl, sda, (uint8_t)(_addr << 1));
  for (int i = 0; i < 8; i++) {
    storeSendByte(scl, sda, STORE_CMD2);
  }
  storeStop(scl, sda);

  delay(STORE_DELAY_MS);

  storeStart(scl, sda);
  storeSendByte(scl, sda, STORE_HEAD, 3, false);
  storeStop(scl, sda);

  storeStart(scl, sda);
  storeSendByte(scl, sda, STORE_ADDR);
  storeSendByte(scl, sda, STORE_CMD2);
  storeStop(scl, sda);

  // Restore hardware I2C
#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_ESP32)
  if (_sdaPin >= 0 && _sclPin >= 0) {
    _wire->begin(_sdaPin, _sclPin);
  } else {
    _wire->begin();
  }
#else
  _wire->begin();
#endif
}
