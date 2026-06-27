#ifndef SSD1306_128X64_H
#define SSD1306_128X64_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class SSD1306_Display {
public:
    SSD1306_Display();
    bool begin(uint8_t sda, uint8_t scl);
    void clear();
    void show();
    void drawText(int16_t x, int16_t y, const String &text, uint8_t size = 1, uint16_t color = SSD1306_WHITE);
    void drawHeader(const String &title);
    void drawBiometrics(float temp, float hum, float hr, float spo2);
    void drawStatus(const String &status, bool isAlert = false);
    void drawCardiacScreen(float hr, float spo2, bool heartBeatFlash);
    void drawEnvScreen(float temp, float hum);
    void drawSystemScreen(const String &patient, const String &deviceId);
    void drawSosAlert(bool showSosText);

private:
    Adafruit_SSD1306 _display;
};

#endif
