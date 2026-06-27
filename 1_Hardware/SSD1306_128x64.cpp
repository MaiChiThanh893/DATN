#include "SSD1306_128x64.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

SSD1306_Display::SSD1306_Display() : _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {}

bool SSD1306_Display::begin(uint8_t sda, uint8_t scl) {
    Wire.begin(sda, scl);
    Wire.setTimeOut(100); 
    
    if (!_display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        return false;
    }
    
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    _display.setTextSize(1);
    _display.display();
    return true;
}

void SSD1306_Display::clear() {
    _display.clearDisplay();
}

void SSD1306_Display::show() {
    _display.display();
}

void SSD1306_Display::drawText(int16_t x, int16_t y, const String &text, uint8_t size, uint16_t color) {
    _display.setTextSize(size);
    _display.setTextColor(color, color == SSD1306_WHITE ? SSD1306_BLACK : SSD1306_WHITE);
    _display.setCursor(x, y);
    _display.print(text);
}

void SSD1306_Display::drawHeader(const String &title) {
    _display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
    _display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); 
    _display.setTextSize(1);
    _display.setCursor(4, 3);
    _display.print(title);
}

void SSD1306_Display::drawStatus(const String &status, bool isAlert) {
    if (isAlert) {
        _display.fillRect(0, 52, 128, 12, SSD1306_WHITE);
        _display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        _display.fillRect(0, 52, 128, 12, SSD1306_BLACK); 
        _display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    _display.setTextSize(1);
    _display.setCursor(4, 54);
    _display.print(status);
}

// =========================================================
// TRANG 0: MÀN HÌNH TỔNG HỢP (BIOMETRICS)
// =========================================================
void SSD1306_Display::drawBiometrics(float temp, float hum, float hr, float spo2) {
    _display.drawFastHLine(0, 15, 128, SSD1306_WHITE);
    _display.drawFastVLine(64, 16, 34, SSD1306_WHITE);
    _display.drawFastHLine(0, 50, 128, SSD1306_WHITE);

    _display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    _display.setTextSize(1);
    
    // --- CỘT TRÁI ---
    _display.setCursor(2, 18);
    _display.print("T:");
    _display.print(temp, 1);
    int16_t x = _display.getCursorX();
    int16_t y = _display.getCursorY();
    _display.drawCircle(x + 2, y + 1, 1, SSD1306_WHITE);
    _display.setCursor(x + 6, y);
    _display.print("C");

    _display.setCursor(2, 34);
    _display.print("H:");
    _display.print(hum, 1); 
    _display.print("%");

    // --- CỘT PHẢI ---
    _display.setCursor(66, 18);
    _display.print("HR:"); 
    if (hr > 0) {
        _display.print((int)hr); 
        _display.print(" bpm"); 
    } else {
        _display.print("--");
    }

    _display.setCursor(66, 34);
    _display.print("SpO2:"); 
    if (spo2 > 0) {
        if (spo2 >= 100.0) {
            _display.print("100%"); 
        } else {
            _display.print(spo2, 1); 
            _display.print("%");
        }
    } else {
        _display.print("--");
    }
}

// =========================================================
// TRANG 1: MÀN HÌNH TIM MẠCH (Màn hình thứ 2)
// =========================================================
void SSD1306_Display::drawCardiacScreen(float hr, float spo2, bool heartBeatFlash) {
    _display.drawFastHLine(0, 15, 128, SSD1306_WHITE);
    _display.drawFastHLine(0, 50, 128, SSD1306_WHITE);
    _display.drawFastVLine(64, 16, 34, SSD1306_WHITE);

    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE, SSD1306_BLACK); 
    
    // --- CỘT TRÁI (Nhịp tim) ---
    _display.setCursor(8, 18);
    _display.print("Nhip tim"); 
    if (heartBeatFlash) {
        _display.fillCircle(58, 21, 3, SSD1306_WHITE);
    }

    _display.setTextSize(2);
    _display.setCursor(4, 30);
    if (hr > 0) {
        _display.print((int)hr); 
        _display.setTextSize(1);
        int16_t xHR = _display.getCursorX();
        _display.setCursor(xHR + 2, 30);
        _display.print("bpm");
    } else {
        _display.print("--");
    }

    // --- CỘT PHẢI (Nồng độ bão hòa O2) ---
    _display.setTextSize(1);
    _display.setCursor(84, 18);
    _display.print("SpO2"); 

    _display.setTextSize(2);
    _display.setCursor(70, 30);
    if (spo2 > 0) {
        if (spo2 >= 100.0) {
            _display.print("100"); 
        } else {
            _display.print(spo2, 1); 
        }
        _display.setTextSize(1);
        int16_t xSpO2 = _display.getCursorX();
        _display.setCursor(xSpO2 + 2, 30);
        _display.print("%");
    } else {
        _display.print("--");
    }
}

// =========================================================
// TRANG 2: MÀN HÌNH MÔI TRƯỜNG (ENVIRONMENT)
// =========================================================
void SSD1306_Display::drawEnvScreen(float temp, float hum) {
    _display.drawFastHLine(0, 15, 128, SSD1306_WHITE);
    _display.drawFastHLine(0, 50, 128, SSD1306_WHITE);
    _display.drawFastVLine(64, 16, 34, SSD1306_WHITE); 

    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    
    // --- CỘT TRÁI (Nhiệt độ) ---
    _display.setCursor(8, 18);
    _display.print("Nhiet do");

    _display.setTextSize(2);
    _display.setCursor(4, 30);
    _display.print(temp, 1);   
    
    _display.setTextSize(1);
    int16_t xTemp = _display.getCursorX(); 
    _display.drawCircle(xTemp + 3, 31, 1, SSD1306_WHITE); 
    _display.setCursor(xTemp + 6, 30);
    _display.print("C"); 

    // --- CỘT PHẢI (Độ ẩm) ---
    _display.setTextSize(1);
    _display.setCursor(81, 18);
    _display.print("Do am");

    _display.setTextSize(2);
    _display.setCursor(70, 30);
    _display.print(hum, 1);

    _display.setTextSize(1);
    int16_t xHum = _display.getCursorX();
    _display.setCursor(xHum + 2, 30);
    _display.print("%");
}

// =========================================================
// TRANG 3: MÀN HÌNH HỆ THỐNG (SYSTEM)
// =========================================================
void SSD1306_Display::drawSystemScreen(const String &patient, const String &deviceId) {
    _display.drawFastHLine(0, 15, 128, SSD1306_WHITE);
    _display.drawFastHLine(0, 50, 128, SSD1306_WHITE);

    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE, SSD1306_BLACK); 

    _display.setCursor(4, 22);
    _display.print("BN: "); _display.print(patient);

    _display.setCursor(4, 34);
    _display.print("ID: "); _display.print(deviceId);
}

// =========================================================
// MÀN HÌNH SOS
// =========================================================
void SSD1306_Display::drawSosAlert(bool showSosText) {
    _display.drawFastHLine(0, 15, 128, SSD1306_WHITE);
    _display.drawFastHLine(0, 50, 128, SSD1306_WHITE);

    if (showSosText) {
        _display.fillRect(0, 16, 128, 34, SSD1306_WHITE);
        _display.setTextSize(2);
        _display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        const char* sosText = "SOS !!!";
        int16_t x1, y1;
        uint16_t w, h;
        _display.getTextBounds(sosText, 0, 0, &x1, &y1, &w, &h);
        int16_t cx = (128 - w) / 2 - x1;
        int16_t cy = 16 + (34 - h) / 2 - y1;
        _display.setCursor(cx, cy);
        _display.print(sosText);
    }
}