#include "MAX30102.h"

MAX30102::MAX30102() {}

bool MAX30102::begin() {
    writeRegister(0x09, 0x40);
    delay(100);

    uint8_t partId = readRegister(0xFF);
    Serial.print("[MAX30102] Phat hien Part ID: 0x");
    Serial.println(partId, HEX);
    
    // 0x15 là MAX30102 chuẩn, 0x11 là MAX30100 (chấp nhận cả hai để tương thích với cảm biến clone)
    if (partId != 0x15 && partId != 0x11) {
        Serial.println("[MAX30102] Loi: Part ID khong khop voi MAX30102 hoac MAX30100!");
        return false;
    }

    writeRegister(0x08, 0x50);
    writeRegister(0x09, 0x03);
    writeRegister(0x0A, 0x2F); // Khoi phuc ADC Range ve 4096nA de tranh bao hoa tin hieu (262143)
    writeRegister(0x0C, 0x24); // Giu dong LED RED o muc 7.2mA on dinh
    writeRegister(0x0D, 0x24); // Giu dong LED IR o muc 7.2mA on dinh
    writeRegister(0x11, 0x21); // Kích hoạt LED RED ở Slot 1 và LED IR ở Slot 2 để cảm biến phát sáng và đo đạc

    resetFifo();
    return true;
}

void MAX30102::resetFifo() {
    writeRegister(0x04, 0); 
    writeRegister(0x05, 0); 
    writeRegister(0x06, 0); 
}

bool MAX30102::getRawValues(uint32_t *ir, uint32_t *red) {
    uint8_t writePtr = readRegister(0x04);   // FIFO_WR_PTR
    uint8_t readPtr  = readRegister(0x06);   // FIFO_RD_PTR

    if (readPtr == writePtr) {
        return false;
    }

    Wire.beginTransmission(MAX30102_I2C_ADDRESS);
    Wire.write(0x07);
    Wire.endTransmission(false);

    Wire.requestFrom(MAX30102_I2C_ADDRESS, 6);
    if (Wire.available() == 6) {
        uint32_t tempRed = 0;
        uint32_t tempIr = 0;

        tempRed = Wire.read(); tempRed <<= 8;
        tempRed |= Wire.read(); tempRed <<= 8;
        tempRed |= Wire.read();

        tempIr = Wire.read(); tempIr <<= 8;
        tempIr |= Wire.read(); tempIr <<= 8;
        tempIr |= Wire.read();

        *red = tempRed & 0x03FFFF;
        *ir  = tempIr  & 0x03FFFF;
        readPtr = (readPtr + 1) & 0x1F;
        writeRegister(0x06, readPtr);
        // ================================================================

        return true;
    }
    return false;
}

void MAX30102::shutdown() {
    uint8_t mode = readRegister(0x09);
    writeRegister(0x09, mode | 0x80);
}

void MAX30102::resume() {
    uint8_t mode = readRegister(0x09);
    writeRegister(0x09, mode & ~0x80);
}

void MAX30102::writeRegister(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(MAX30102_I2C_ADDRESS);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

uint8_t MAX30102::readRegister(uint8_t reg) {
    Wire.beginTransmission(MAX30102_I2C_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(MAX30102_I2C_ADDRESS, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}
