#ifndef _HDC2022_H_
#define _HDC2022_H_

#include <Arduino.h>
#include <Wire.h>

class HDC2022_c {
public:
  void      Init(uint8_t address = 0x40);
  float     get_Temperature();
  float     get_Humidity();

private:
  uint8_t   _address;
  void      writeReg(uint8_t reg, uint8_t val);
  uint8_t   readReg(uint8_t reg);
};

#endif
