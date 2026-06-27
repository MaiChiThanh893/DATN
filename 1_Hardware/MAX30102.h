#ifndef MAX30102_H
#define MAX30102_H

#include <Arduino.h>
#include <Wire.h>

#define MAX30102_I2C_ADDRESS 0x57

class MAX30102 {
public:
    MAX30102();
    bool begin();
    bool getRawValues(uint32_t *ir, uint32_t *red);
    void resetFifo();
    void shutdown();
    void resume();
    uint8_t readRegister(uint8_t reg);

private:
    void writeRegister(uint8_t reg, uint8_t val);
};

#endif
