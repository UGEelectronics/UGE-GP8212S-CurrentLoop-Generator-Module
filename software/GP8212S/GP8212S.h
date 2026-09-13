/*!
 * @file GP8212S.h
 * @brief Arduino / ESP32 driver for Linearin GP8212S
 *        15-bit I2C DAC → 0/4–20 mA current loop output
 *
 * Ported from the STM32 bit-bang I2C example (MyI2C.c) to the Arduino Wire API.
 *
 * Datasheet formula (Rs = 100 Ω typical):
 *   IOUT = (2.5 V / Rs) * (DATA / 0x7FFF)
 *   With Rs = 100 Ω → 0 … 25 mA for DATA = 0 … 0x7FFF
 *
 * Default I2C address: 0x58
 */

#ifndef GP8212S_H
#define GP8212S_H

#include <Arduino.h>
#include <Wire.h>

class GP8212S {
 public:
  static const uint8_t DEFAULT_I2C_ADDR = 0x58;
  static const uint16_t DAC_MAX = 0x7FFF;  // 15-bit full scale
  static const float FULL_SCALE_MA;       // 25.0 mA with Rs = 100 Ω

  /*!
   * @param deviceAddr  7-bit I2C address (default 0x58)
   * @param wire        TwoWire instance (&Wire, &Wire1, …)
   * @param fullScaleMa Full-scale current in mA (25.0 when Rs = 100 Ω)
   */
  explicit GP8212S(uint8_t deviceAddr = DEFAULT_I2C_ADDR,
                   TwoWire *wire = &Wire,
                   float fullScaleMa = 25.0f);

  /*!
   * @brief Start I2C and probe the device.
   * @param sda  Optional SDA pin (-1 = board default). Useful on ESP32.
   * @param scl  Optional SCL pin (-1 = board default). Useful on ESP32.
   * @param freq I2C clock Hz (chip supports up to 400 kHz).
   * @return 0 on success (ACK), non-zero Wire error code on failure.
   */
  int begin(int sda = -1, int scl = -1, uint32_t freq = 100000);

  /*! @brief True if the last begin() / isConnected() saw an ACK. */
  bool isConnected();

  /*!
   * @brief Write raw 15-bit DAC code (0 … 0x7FFF).
   * @return true if I2C write succeeded.
   */
  bool setDAC(uint16_t code);

  /*!
   * @brief Set output current in milliamps (0 … fullScaleMa).
   * @return DAC code that was written, or 0xFFFF on I2C failure.
   */
  uint16_t setCurrent_mA(float mA);

  /*!
   * @brief Map 0.0 … 100.0 % of the 4–20 mA industrial span.
   *        0 % → 4 mA, 100 % → 20 mA.
   * @return DAC code written, or 0xFFFF on failure.
   */
  uint16_t setPercent4_20(float percent);

  /*!
   * @brief Convert milliamps to 15-bit DAC code using the configured full scale.
   */
  uint16_t mAToDAC(float mA) const;

  /*!
   * @brief Convert DAC code to milliamps (ideal, before board calibration).
   */
  float dacTo_mA(uint16_t code) const;

  /*!
   * @brief Two-point calibration for accurate 4–20 mA.
   *        Measure the DAC codes that actually produce 4 mA and 20 mA on your
   *        board, then call this. Subsequent setCurrent_mA / setPercent4_20
   *        use linear interpolation between those points for the 4–20 mA span.
   *        Outside that span (or if disabled), ideal scaling is used.
   */
  void calibrate4_20(uint16_t dacAt4mA, uint16_t dacAt20mA);
  void clearCalibration();

  /*!
   * @brief Store the last DAC value in chip NVM so it survives power-off.
   *        Uses the special store timing from the datasheet (bit-bang).
   *        Temporarily releases the Wire bus on the given SCL/SDA pins.
   * @param sda Pin used for SDA (must match begin()).
   * @param scl Pin used for SCL (must match begin()).
   */
  void store(int sda, int scl);

  uint16_t lastDAC() const { return _lastDac; }
  float fullScale_mA() const { return _fullScaleMa; }

 private:
  bool writeDACRegister(uint16_t shifted16);
  void storeStart(int scl, int sda);
  void storeStop(int scl, int sda);
  uint8_t storeSendByte(int scl, int sda, uint8_t data, uint8_t bits = 8,
                        bool expectAck = true);

  TwoWire *_wire;
  uint8_t _addr;
  float _fullScaleMa;
  uint16_t _lastDac;
  bool _calEnabled;
  uint16_t _cal4;
  uint16_t _cal20;
  int _sdaPin;
  int _sclPin;
};

#endif
