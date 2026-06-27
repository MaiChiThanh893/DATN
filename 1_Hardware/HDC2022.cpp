#include "HDC2022.h"

#define ADDR_TEMPERATURE_LOW  0x00
#define ADDR_TEMPERATURE_HIGH 0x01
#define ADDR_HUMIDITY_LOW     0x02
#define ADDR_HUMIDITY_HIGH    0x03
#define ADDR_DEVICE_CONFIGURATION 0x0E
#define ADDR_MEASUREMENT_CONFIGURATION 0x0F

void HDC2022_c::Init(uint8_t address) {
  _address = address;
  
  writeReg(ADDR_DEVICE_CONFIGURATION, 0x80); 
  delay(20);
  
  writeReg(ADDR_MEASUREMENT_CONFIGURATION, 0x00); 
}

float HDC2022_c::get_Temperature() {
  uint8_t config = readReg(ADDR_MEASUREMENT_CONFIGURATION);
  writeReg(ADDR_MEASUREMENT_CONFIGURATION, config | 0x01);
  
  delay(15);
  
  uint8_t tempLow = readReg(ADDR_TEMPERATURE_LOW);
  uint8_t tempHigh = readReg(ADDR_TEMPERATURE_HIGH);
  
  uint16_t rawTemp = (tempHigh << 8) | tempLow;
  return ((float)rawTemp / 65536.0) * 165.0 - 40.0;
}

float HDC2022_c::get_Humidity() {
  uint8_t humLow = readReg(ADDR_HUMIDITY_LOW);
  uint8_t humHigh = readReg(ADDR_HUMIDITY_HIGH);
  
  uint16_t rawHum = (humHigh << 8) | humLow;
  return ((float)rawHum / 65536.0) * 100.0;
}

void HDC2022_c::writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(_address);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

uint8_t HDC2022_c::readReg(uint8_t reg) {
  Wire.beginTransmission(_address);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(_address, (uint8_t)1);
  if (Wire.available()) {
    return Wire.read();
  }
  return 0;
}
