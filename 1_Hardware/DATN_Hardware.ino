#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h"
#include "HDC2022.h"
#include "MAX30102.h"
#include "SSD1306_128x64.h"

// Đối tượng quản lý bộ nhớ Flash và máy chủ cấu hình
Preferences preferences;
WebServer server(80);
DNSServer dnsServer;

bool apPortalActive = false;

// Kết nối STA không chặn (tránh treo AP portal lúc khởi động)
bool staInitialConnectDone = false;
bool staConnectInProgress = false;
unsigned long staConnectStartMs = 0;
int staConnectTryIdx = 0;
int staConnectTriesLeft = 0;
unsigned long apPortalStartMs = 0;

// Danh sách WiFi đã lưu trong Flash (ưu tiên mạng đầu tiên = kết nối gần nhất)
struct SavedWifiNetwork {
    String ssid;
    String password;
};
SavedWifiNetwork savedWifiNetworks[MAX_SAVED_WIFI_NETWORKS];
int savedWifiCount = 0;
int lastConnectedWifiIndex = -1;
String connectedSSID = "";

String apiServerIP = "192.168.1.5"; // Mặc định
int apiServerPort = 8000;          // Mặc định
const char* DEFAULT_PATIENT_NAME = "Mai Chí Thanh";
String patientName = DEFAULT_PATIENT_NAME;
String deviceID = "";
const char* API_POST_URL = "https://mcthank.xyz/api.php?action=post_data";
const char* API_SERVER_LABEL = "mcthank.xyz (HTTPS)";

// Theo dõi nút MODE
unsigned long modeBtnPressStart = 0;
bool modeBtnPressedLast = false;
unsigned long modeBtnBeepOffAt = 0;
int currentDisplayMode = 0;
const int MAX_DISPLAY_MODES = 4;

// HTTP POST chạy nền — tránh chặn loop (nút MODE, OLED, MAX30102)
static TaskHandle_t httpTaskHandle = NULL;
static volatile bool httpPostBusy = false;
static volatile bool sosHttpPending = false;
static volatile bool periodicHttpPending = false;

// Trạng thái SOS (nút GPIO6)
volatile bool sosActive = false;
unsigned long sosBtnPressStart = 0;
bool sosBtnPressedLast = false;
bool sosOffHoldTriggered = false;
unsigned long sosBlinkEpochMs = 0;

// Khởi tạo các đối tượng thiết bị phần cứng
HDC2022_c hdc;
MAX30102 particleSensor;
SSD1306_Display display;

// Các biến lưu trữ chỉ số sinh trắc học
float currentTemperature = 0.0;
float currentHumidity = 0.0;
float currentHeartRate = 0.0;
float currentSpO2 = 0.0;

// Các biến lưu trữ dữ liệu thô phục vụ debug chi tiết
uint32_t lastRawIR = 0;
uint32_t lastRawRed = 0;
bool lastSampleReady = false;
unsigned long last_beat_time = 0;

// =========================================================================
// THUẬT TOÁN ĐO NHỊP TIM PBA (MAXIM INTEGRATED / SPARKFUN)
// =========================================================================
int16_t IR_AC_Max = 20;
int16_t IR_AC_Min = -20;
int16_t IR_AC_Signal_Current = 0;
int16_t IR_AC_Signal_Previous = 0;
int16_t IR_AC_Signal_min = 0;
int16_t IR_AC_Signal_max = 0;
int16_t IR_Average_Estimated = 0;
int16_t RED_Average_Estimated = 0;

int16_t positiveEdge = 0;
int16_t negativeEdge = 0;
int32_t ir_avg_reg = 0;
int32_t red_avg_reg = 0;

int16_t cbuf_ir[32] = {0};
uint8_t offset_ir = 0;
int16_t cbuf_red[32] = {0};
uint8_t offset_red = 0;

static const uint16_t FIRCoeffs[12] = {172, 321, 579, 927, 1360, 1858, 2390, 2916, 3391, 3768, 4012, 4096};

int32_t mul16(int16_t x, int16_t y) {
    return ((long)x * (long)y);
}

int16_t averageDCEstimator(int32_t *p, uint16_t x) {
    *p += ((((long) x << 15) - *p) >> 4);
    return (*p >> 15);
}

int16_t lowPassFIRFilter_IR(int16_t din) {  
    cbuf_ir[offset_ir] = din;
    int32_t z = mul16(FIRCoeffs[11], cbuf_ir[(offset_ir - 11) & 0x1F]);
    for (uint8_t i = 0 ; i < 11 ; i++) {
        z += mul16(FIRCoeffs[i], cbuf_ir[(offset_ir - i) & 0x1F] + cbuf_ir[(offset_ir - 22 + i) & 0x1F]);
    }
    offset_ir++;
    offset_ir %= 32;
    return (z >> 15);
}

int16_t lowPassFIRFilter_RED(int16_t din) {  
    cbuf_red[offset_red] = din;
    int32_t z = mul16(FIRCoeffs[11], cbuf_red[(offset_red - 11) & 0x1F]);
    for (uint8_t i = 0 ; i < 11 ; i++) {
        z += mul16(FIRCoeffs[i], cbuf_red[(offset_red - i) & 0x1F] + cbuf_red[(offset_red - 22 + i) & 0x1F]);
    }
    offset_red++;
    offset_red %= 32;
    return (z >> 15);
}

void resetPBAAlgorithm() {
    IR_AC_Max = 20;
    IR_AC_Min = -20;
    IR_AC_Signal_Current = 0;
    IR_AC_Signal_Previous = 0;
    IR_AC_Signal_min = 0;
    IR_AC_Signal_max = 0;
    IR_Average_Estimated = 0;
    RED_Average_Estimated = 0;
    positiveEdge = 0;
    negativeEdge = 0;
    ir_avg_reg = 0;
    red_avg_reg = 0;
    memset(cbuf_ir, 0, sizeof(cbuf_ir));
    memset(cbuf_red, 0, sizeof(cbuf_red));
    offset_ir = 0;
    offset_red = 0;
}

// Trạng thái phần cứng thực tế
bool hasHDC = false;
bool hasMAX = false;
bool hasOLED = false;

// Biến điều khiển thời gian gửi dữ liệu và cập nhật màn hình
unsigned long lastMeasureTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastWifiCheckTime = 0;

// =========================================================================
// QUÉT ĐỊA CHỈ THIẾT BỊ TRÊN BUS I2C (DEBUG PHẦN CỨNG)
// =========================================================================
void scanI2CBus() {
    Serial.println("\n[I2C] Dang quet cac thiet bi tren bus...");
    byte error, address;
    int nDevices = 0;

    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.print("[I2C] Tim thay thiet bi tai dia chi: 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            
            // Chú thích thiết bị phát hiện
            if (address == 0x3C) Serial.println(" (Man hinh OLED SSD1306)");
            else if (address == 0x40) Serial.println(" (Cam bien HDC2022 Temp/Hum)");
            else if (address == 0x57) Serial.println(" (Cam bien MAX30102 HR/SpO2)");
            else Serial.println(" (Thiet bi la/Khong xac dinh)");
            
            nDevices++;
        }
        else if (error == 4) {
            Serial.print("[I2C] Loi chap bus tai dia chi: 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
        }
    }
    if (nDevices == 0) {
        Serial.println("[I2C] NGUY HIEM: Khong tim thay thiet bi I2C nao! Kiem tra day SDA/SCL va nguon 3.3V.");
    } else {
        Serial.println("[I2C] Hoan thanh quet bus I2C.\n");
    }
}

// =========================================================================
// KHỞI TẠO CÁC CHÂN ĐIỀU KHIỂN CẢNH BÁO (LED / BUZZER)
// =========================================================================
void initAlertPins() {
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    
    // Tắt toàn bộ khi khởi động
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, LOW);
}

// Điều khiển còi và LED cảnh báo dựa trên các ngưỡng an toàn
void handleAlerts(float hr, float spo2, float temp, bool isWifiConnecting = false) {
    if (sosActive) {
        digitalWrite(LED_GREEN_PIN, LOW);
        digitalWrite(LED_RED_PIN, (millis() / 300) % 2);
        digitalWrite(BUZZER_PIN, (millis() / 600) % 2);
        return;
    }

    if (isWifiConnecting) {
        // Nhấp nháy xanh lá cây chậm khi đang kết nối WiFi
        digitalWrite(LED_GREEN_PIN, (millis() / 500) % 2);
        digitalWrite(LED_RED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        return;
    }

    // Đã bỏ cảnh báo nguy hiểm theo yêu cầu
    bool hasAlert = false;

    if (hasAlert) {
        // Bật LED Đỏ nhấp nháy nhanh, tắt LED Xanh
        digitalWrite(LED_GREEN_PIN, LOW);
        digitalWrite(LED_RED_PIN, (millis() / 200) % 2); // Nhấp nháy chu kỳ 200ms
        
        // Kêu còi bíp bíp chu kỳ nhanh
        digitalWrite(BUZZER_PIN, (millis() / 200) % 2);
    } else {
        // Trạng thái bình thường: Bật LED Xanh, tắt LED Đỏ và còi
        digitalWrite(LED_GREEN_PIN, HIGH);
        digitalWrite(LED_RED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
    }
}

// =========================================================================
// QUẢN LÝ NHIỀU MẠNG WIFI ĐÃ LƯU
// =========================================================================
void saveWifiNetworksToFlash() {
    StaticJsonDocument<2048> doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < savedWifiCount; i++) {
        JsonObject obj = arr.createNestedObject();
        obj["ssid"] = savedWifiNetworks[i].ssid;
        obj["pass"] = savedWifiNetworks[i].password;
    }

    String json;
    serializeJson(doc, json);

    preferences.begin("health-cfg", false);
    preferences.putString("wifi_networks", json);
    preferences.putInt("last_wifi_idx", lastConnectedWifiIndex);
    preferences.remove("ssid");
    preferences.remove("pass");
    preferences.end();

    Serial.println("[FLASH] Da luu danh sach WiFi vao Flash.");
}

void loadWifiNetworksFromFlash() {
    savedWifiCount = 0;
    lastConnectedWifiIndex = -1;

    preferences.begin("health-cfg", true);
    String wifiJson = preferences.getString("wifi_networks", "");
    lastConnectedWifiIndex = preferences.getInt("last_wifi_idx", -1);

    // Migrate cấu hình WiFi cũ (1 mạng duy nhất) sang định dạng mới
    if (wifiJson.length() == 0) {
        String legacySsid = preferences.getString("ssid", "");
        String legacyPass = preferences.getString("pass", "");
        preferences.end();
        if (legacySsid.length() > 0) {
            savedWifiNetworks[0].ssid = legacySsid;
            savedWifiNetworks[0].password = legacyPass;
            savedWifiCount = 1;
            lastConnectedWifiIndex = 0;
            saveWifiNetworksToFlash();
            Serial.println("[FLASH] Da chuyen doi cau hinh WiFi cu sang dinh dang moi.");
        }
        return;
    }
    preferences.end();

    StaticJsonDocument<2048> doc;
    DeserializationError err = deserializeJson(doc, wifiJson);
    if (err) {
        Serial.println("[FLASH] Loi doc danh sach WiFi, bo qua.");
        return;
    }

    JsonArray arr = doc.as<JsonArray>();
    for (size_t i = 0; i < arr.size(); i++) {
        if (savedWifiCount >= MAX_SAVED_WIFI_NETWORKS) break;
        JsonObject obj = arr[i].as<JsonObject>();
        const char* ssid = obj["ssid"];
        const char* pass = obj["pass"];
        if (ssid && strlen(ssid) > 0) {
            savedWifiNetworks[savedWifiCount].ssid = String(ssid);
            savedWifiNetworks[savedWifiCount].password = pass ? String(pass) : "";
            savedWifiCount++;
        }
    }

    if (lastConnectedWifiIndex >= savedWifiCount) {
        lastConnectedWifiIndex = savedWifiCount > 0 ? 0 : -1;
    }
}

void addOrUpdateWifiNetwork(const String& ssid, const String& pass) {
    if (ssid.length() == 0) return;

    int existingIdx = -1;
    for (int i = 0; i < savedWifiCount; i++) {
        if (savedWifiNetworks[i].ssid == ssid) {
            existingIdx = i;
            break;
        }
    }

    SavedWifiNetwork entry;
    entry.ssid = ssid;
    entry.password = pass;

    if (existingIdx >= 0) {
        for (int i = existingIdx; i > 0; i--) {
            savedWifiNetworks[i] = savedWifiNetworks[i - 1];
        }
        savedWifiNetworks[0] = entry;
    } else {
        if (savedWifiCount >= MAX_SAVED_WIFI_NETWORKS) {
            savedWifiCount = MAX_SAVED_WIFI_NETWORKS - 1;
        }
        for (int i = savedWifiCount; i > 0; i--) {
            savedWifiNetworks[i] = savedWifiNetworks[i - 1];
        }
        savedWifiNetworks[0] = entry;
        savedWifiCount++;
    }

    lastConnectedWifiIndex = 0;
    saveWifiNetworksToFlash();
}

bool removeWifiNetwork(const String& ssid) {
    int removeIdx = -1;
    for (int i = 0; i < savedWifiCount; i++) {
        if (savedWifiNetworks[i].ssid == ssid) {
            removeIdx = i;
            break;
        }
    }
    if (removeIdx < 0) return false;

    for (int i = removeIdx; i < savedWifiCount - 1; i++) {
        savedWifiNetworks[i] = savedWifiNetworks[i + 1];
    }
    savedWifiCount--;

    if (lastConnectedWifiIndex == removeIdx) {
        lastConnectedWifiIndex = -1;
        connectedSSID = "";
    } else if (lastConnectedWifiIndex > removeIdx) {
        lastConnectedWifiIndex--;
    }

    saveWifiNetworksToFlash();
    return true;
}

void serviceWifiPortal() {
    if (!apPortalActive) return;
    dnsServer.processNextRequest();
    server.handleClient();
}

void ensureApStaMode() {
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        delay(50);
    }
}

bool connectToNetwork(const String& ssid, const String& pass, unsigned long timeoutMs) {
    Serial.print("[WIFI] Dang thu ket noi: ");
    Serial.println(ssid);

    ensureApStaMode();
    WiFi.disconnect(false);
    delay(50);
    WiFi.begin(ssid.c_str(), pass.c_str());

    unsigned long start = millis();
    while (millis() - start < timeoutMs) {
        serviceWifiPortal();
        if (WiFi.status() == WL_CONNECTED) {
            connectedSSID = ssid;
            Serial.print("[WIFI] Ket noi thanh cong! IP: ");
            Serial.println(WiFi.localIP());
            return true;
        }
        delay(50);
    }

    Serial.println("[WIFI] Ket noi that bai.");
    return false;
}

bool tryConnectSavedNetworks() {
    if (savedWifiCount == 0) {
        connectedSSID = "";
        return false;
    }

    int startIdx = (lastConnectedWifiIndex >= 0 && lastConnectedWifiIndex < savedWifiCount)
        ? lastConnectedWifiIndex : 0;

    for (int attempt = 0; attempt < savedWifiCount; attempt++) {
        int idx = (startIdx + attempt) % savedWifiCount;
        if (connectToNetwork(
                savedWifiNetworks[idx].ssid,
                savedWifiNetworks[idx].password,
                WIFI_CONNECT_TIMEOUT_MS)) {
            lastConnectedWifiIndex = idx;
            saveWifiNetworksToFlash();
            return true;
        }
    }

    connectedSSID = "";
    return false;
}

bool shouldDeferStaConnect() {
    if (millis() - apPortalStartMs < WIFI_STA_DEFER_MS) return true;
#if WIFI_STA_PAUSE_WHEN_AP_CLIENT
    if (WiFi.softAPgetStationNum() > 0) return true;
#endif
    return false;
}

void checkWifiReconnect() {
    if (savedWifiCount == 0) return;
    if (shouldDeferStaConnect()) return;

    if (WiFi.status() == WL_CONNECTED) {
        connectedSSID = WiFi.SSID();
        return;
    }

    static unsigned long lastReconnectAttempt = 0;
    unsigned long now = millis();
    if (now - lastReconnectAttempt < WIFI_RECONNECT_INTERVAL_MS) return;

    lastReconnectAttempt = now;
    connectedSSID = "";
    Serial.println("[WIFI] Mat ket noi, dang thu ket noi lai cac mang da luu...");
    tryConnectSavedNetworks();
}

// =========================================================================
// LOGIC KẾT NỐI WIFI
// =========================================================================
void setupWiFi() {
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_11dBm);
    configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov", "asia.pool.ntp.org");
    Serial.println("[NTP] Da thiet lap NTP client (GMT+7).");

    if (savedWifiCount == 0) {
        staInitialConnectDone = true;
        Serial.println("[WIFI] Chua co mang WiFi nao duoc luu. Vao http://192.168.4.1 de cau hinh.");
        if (hasOLED) {
            display.clear();
            display.drawHeader("IoT HEALTH MONITOR");
            display.drawText(4, 20, "Chua co WiFi", 1);
            display.drawText(4, 32, "Ket noi AP setup:", 1);
            display.drawText(4, 44, deviceID, 1);
            display.drawStatus("192.168.4.1", false);
            display.show();
        }
        return;
    }

    if (hasOLED) {
        display.clear();
        display.drawHeader("IoT HEALTH MONITOR");
        display.drawText(4, 20, "Khoi dong WiFi...", 1);
        display.drawText(4, 32, savedWifiNetworks[0].ssid, 1);
        display.drawStatus("Dang ket noi...", false);
        display.show();
    }

    // Khong ket noi dong trong setup() de AP portal san sang ngay
    staInitialConnectDone = false;
    staConnectInProgress = false;
}

void beginStaConnectAttempt(int networkIdx) {
    if (networkIdx < 0 || networkIdx >= savedWifiCount) return;

    ensureApStaMode();
    WiFi.disconnect(false);
    delay(20);
    WiFi.begin(
        savedWifiNetworks[networkIdx].ssid.c_str(),
        savedWifiNetworks[networkIdx].password.c_str());

    staConnectTryIdx = networkIdx;
    staConnectStartMs = millis();
    staConnectInProgress = true;

    Serial.print("[WIFI] STA dang thu (non-blocking): ");
    Serial.println(savedWifiNetworks[networkIdx].ssid);
}

void processStaConnectNonBlocking() {
    if (staInitialConnectDone || savedWifiCount == 0) return;
    if (shouldDeferStaConnect()) return;

    if (!staConnectInProgress) {
        staConnectTryIdx = (lastConnectedWifiIndex >= 0 && lastConnectedWifiIndex < savedWifiCount)
            ? lastConnectedWifiIndex : 0;
        staConnectTriesLeft = savedWifiCount;
        beginStaConnectAttempt(staConnectTryIdx);
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        connectedSSID = WiFi.SSID();
        lastConnectedWifiIndex = staConnectTryIdx;
        staConnectInProgress = false;
        staInitialConnectDone = true;
        Serial.print("[WIFI] STA ket noi thanh cong! IP: ");
        Serial.println(WiFi.localIP());
        return;
    }

    if (millis() - staConnectStartMs < WIFI_CONNECT_TIMEOUT_MS) return;

    staConnectTriesLeft--;
    if (staConnectTriesLeft <= 0) {
        staConnectInProgress = false;
        staInitialConnectDone = true;
        connectedSSID = "";
        Serial.println("[WIFI] Het thoi gian thu ket noi STA. AP portal van hoat dong.");
        return;
    }

    staConnectTryIdx = (staConnectTryIdx + 1) % savedWifiCount;
    beginStaConnectAttempt(staConnectTryIdx);
}

void checkWiFiConnection() {
    checkWifiReconnect();
}

// =========================================================================
// XỬ LÝ FIFO MAX30102 (gọi liên tục, không được vứt mẫu sau HTTPS)
// =========================================================================
void processMax30102Fifo() {
    if (!hasMAX) {
        currentHeartRate = 0.0;
        currentSpO2 = 0.0;
        return;
    }

    uint32_t irValue = 0;
    uint32_t redValue = 0;

    static float dc_ir = 0;
    static float dc_red = 0;
    static float ac_ir = 0;
    static float ac_red = 0;
    static float ac_ir_max = -99999;
    static float ac_ir_min = 99999;
    static float ac_red_max = -99999;
    static float ac_red_min = 99999;
    static float dynamic_threshold = 0;
    static bool was_above_threshold = false;
    static float avg_beat_period = 600.0;
    static bool first_beat = true;
    static float spo2_buffer[5] = {0};
    static int spo2_index = 0;

    bool hasSamples = false;

    while (particleSensor.getRawValues(&irValue, &redValue)) {
        hasSamples = true;
        lastRawIR = irValue;
        lastRawRed = redValue;
        lastSampleReady = true;

        if (irValue > 15000) {
            if (dc_ir == 0) {
                dc_ir = irValue;
                dc_red = redValue;
            } else {
                dc_ir = dc_ir * 0.995 + irValue * 0.005;
                dc_red = dc_red * 0.995 + redValue * 0.005;
            }

            float raw_ac_ir = irValue - dc_ir;
            float raw_ac_red = redValue - dc_red;

            ac_ir = ac_ir * 0.88 + raw_ac_ir * 0.12;
            ac_red = ac_red * 0.88 + raw_ac_red * 0.12;

            if (ac_ir > ac_ir_max) ac_ir_max = ac_ir;
            if (ac_ir < ac_ir_min) ac_ir_min = ac_ir;
            if (ac_red > ac_red_max) ac_red_max = ac_red;
            if (ac_red < ac_red_min) ac_red_min = ac_red;

            unsigned long now_beat = millis();
            if (last_beat_time == 0) last_beat_time = now_beat;

            if (now_beat - last_beat_time > 3000) {
                // Reset peak tracker (khong gan max=min de tranh amplitude=0)
                ac_ir_max = -99999;
                ac_ir_min = 99999;
                ac_red_max = -99999;
                ac_red_min = 99999;
                last_beat_time = now_beat;
                first_beat = true;
                was_above_threshold = false;
            }

            float amplitude = ac_ir_max - ac_ir_min;
            if (amplitude > 8) {
                dynamic_threshold = ac_ir_min + amplitude * 0.52;

                if (ac_ir > dynamic_threshold) {
                    if (!was_above_threshold) {
                        unsigned long duration = now_beat - last_beat_time;

                        if (first_beat) {
                            first_beat = false;
                            last_beat_time = now_beat;
                            ac_ir_max = -99999;
                            ac_ir_min = 99999;
                            ac_red_max = -99999;
                            ac_red_min = 99999;
                        } else {
                            if (duration > 400 && duration < 1330) {
                                avg_beat_period = avg_beat_period * 0.5 + duration * 0.5;
                                float calculated_hr = 60000.0 / avg_beat_period;

                                if (currentHeartRate == 0) {
                                    currentHeartRate = calculated_hr;
                                } else {
                                    currentHeartRate = currentHeartRate * 0.7 + calculated_hr * 0.3;
                                }

                                float red_ac_amplitude = ac_red_max - ac_red_min;
                                float ir_ac_amplitude = ac_ir_max - ac_ir_min;

                                if (ir_ac_amplitude > 0 && dc_ir > 0 && dc_red > 0) {
                                    float ratio = (red_ac_amplitude / dc_red) / (ir_ac_amplitude / dc_ir);

                                    float spo2Calc = 0;
                                    if (amplitude < 25.0) {
                                        spo2Calc = 115.0 - 18.0 * ratio;
                                        if (spo2Calc < 93.0) {
                                            spo2Calc = 94.0 + (spo2Calc - 93.0) * 0.15;
                                        }
                                    } else {
                                        spo2Calc = 110.0 - 25.0 * ratio;
                                        if (spo2Calc < 75.0) spo2Calc = 75.0;
                                    }

                                    if (spo2Calc > 100.0) spo2Calc = 100.0;

                                    spo2_buffer[spo2_index] = spo2Calc;
                                    spo2_index = (spo2_index + 1) % 5;

                                    float spo2_sum = 0;
                                    int spo2_count = 0;
                                    for (int i = 0; i < 5; i++) {
                                        if (spo2_buffer[i] > 0) {
                                            spo2_sum += spo2_buffer[i];
                                            spo2_count++;
                                        }
                                    }
                                    float averaged_spo2 = (spo2_count > 0) ? (spo2_sum / spo2_count) : spo2Calc;

                                    if (currentSpO2 == 0) {
                                        currentSpO2 = averaged_spo2;
                                    } else {
                                        currentSpO2 = currentSpO2 * 0.75 + averaged_spo2 * 0.25;
                                    }
                                }

                                ac_ir_max = -99999;
                                ac_ir_min = 99999;
                                ac_red_max = -99999;
                                ac_red_min = 99999;
                                last_beat_time = now_beat;
                            } else if (duration > 1330) {
                                last_beat_time = now_beat;
                                first_beat = true;
                            }
                        }
                        was_above_threshold = true;
                    }
                } else if (ac_ir < (dynamic_threshold - amplitude * 0.1)) {
                    was_above_threshold = false;
                }
            }
        } else {
            currentHeartRate = 0.0;
            currentSpO2 = 0.0;
            ac_ir_max = -99999;
            ac_ir_min = 99999;
            ac_red_max = -99999;
            ac_red_min = 99999;
            was_above_threshold = false;
            dc_ir = 0;
            dc_red = 0;
            last_beat_time = 0;
            first_beat = true;
            for (int i = 0; i < 5; i++) spo2_buffer[i] = 0;
            spo2_index = 0;
        }
    }

    if (!hasSamples) {
        lastSampleReady = false;
    }
}

// =========================================================================
// THU THẬP DỮ LIỆU CẢM BIẾN (THẬT HOẶC GIẢ LẬP)
// =========================================================================
void readBiometrics() {
    if (USE_SIMULATOR) {
        // CHẾ ĐỘ GIẢ LẬP: Tạo dữ liệu ngẫu nhiên tiệm cận y khoa
        // Nhiệt độ: 36.2 đến 38.8 °C
        currentTemperature = 36.5 + (random(-30, 230) / 100.0);
        // Độ ẩm: 55% đến 75%
        currentHumidity = 60.0 + (random(-50, 150) / 10.0);
        // Nhịp tim: 68 đến 125 bpm
        currentHeartRate = 72 + random(-4, 4);
        // SpO2: 92% đến 99%
        currentSpO2 = 98 - random(0, 3);
        
        // Mô phỏng thỉnh thoảng có chỉ số bất thường để test cảnh báo
        if (random(0, 100) < 12) {
            currentHeartRate = 126; // Nhịp tim cao bất thường
            currentSpO2 = 88;      // SpO2 thấp nguy hiểm
            currentTemperature = 38.9; // Sốt cao
        }
        return;
    }

    // CHẾ ĐỘ ĐỌC PHẦN CỨNG THẬT
    // 1. Đọc từ cảm biến HDC2022
    if (hasHDC) {
        currentTemperature = hdc.get_Temperature();
        currentHumidity = hdc.get_Humidity();
        // Kiểm tra lỗi đọc giá trị
        if (currentTemperature < -39.0 || currentTemperature > 120.0) {
            currentTemperature = 0.0;
            currentHumidity = 0.0;
        }
    } else {
        currentTemperature = 0.0;
        currentHumidity = 0.0;
    }

    // 2. Đọc từ cảm biến MAX30102 (FIFO cần xử lý liên tục)
    processMax30102Fifo();
}

// =========================================================================
// GỬI DỮ LIỆU LÊN SERVER API NODE.JS / PHP
// =========================================================================
String urlEncodeFormValue(const String& value) {
    String encoded = "";
    encoded.reserve(value.length() * 3);

    for (size_t i = 0; i < value.length(); i++) {
        unsigned char c = static_cast<unsigned char>(value[i]);
        bool isSafe =
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~';

        if (isSafe) {
            encoded += static_cast<char>(c);
        } else if (c == ' ') {
            encoded += '+';
        } else {
            char hex[4];
            snprintf(hex, sizeof(hex), "%%%02X", c);
            encoded += hex;
        }
    }

    return encoded;
}

void sendDataToServer() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[HTTP] WiFi chua ket noi");
        return;
    }

    static bool tlsReady = false;
    static WiFiClientSecure secureClient;
    if (!tlsReady) {
        secureClient.setInsecure();
        tlsReady = true;
    }

    String requestBody;
    requestBody.reserve(200);
    requestBody += "deviceId=" + urlEncodeFormValue(deviceID);
    requestBody += "&patientName=" + urlEncodeFormValue(patientName);
    requestBody += "&heartRate=" + String(currentHeartRate > 0 ? (round(currentHeartRate * 10) / 10.0) : 0, 1);
    requestBody += "&spo2=" + String(currentSpO2 > 0 ? (round(currentSpO2 * 10) / 10.0) : 0, 1);
    requestBody += "&temperature=" + String(currentTemperature > 0 ? (round(currentTemperature * 10) / 10.0) : 0, 1);
    requestBody += "&humidity=" + String(currentHumidity > 0 ? (round(currentHumidity * 10) / 10.0) : 0, 1);
    requestBody += "&sos=" + String(sosActive ? "true" : "false");

    Serial.print("[HTTP] POST ");
    Serial.println(API_POST_URL);
    Serial.print("[HTTP] Payload: ");
    Serial.println(requestBody);

    for (int attempt = 1; attempt <= 2; attempt++) {
        HTTPClient http;
        http.setConnectTimeout(8000);
        http.setTimeout(8000);
        http.setReuse(false);

        if (!http.begin(secureClient, API_POST_URL)) {
            Serial.println("[HTTP] http.begin() that bai");
            secureClient.stop();
            delay(300);
            continue;
        }

        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        http.addHeader("Connection", "close");

        int httpCode = http.POST(requestBody);
        String response = http.getString();
        http.end();
        secureClient.stop();

        Serial.print("[HTTP] code: ");
        Serial.println(httpCode);
        Serial.print("[HTTP] body: ");
        Serial.println(response);

        if (httpCode == 200) {
            Serial.println("[HTTP] THANH CONG");
            return;
        }

        Serial.print("[HTTP] THAT BAI lan ");
        Serial.println(attempt);
        delay(500);
    }

    Serial.println("[HTTP] Het so lan thu");
}

void httpPostTask(void* pvParameters) {
    (void)pvParameters;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        for (;;) {
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("[HTTP] WiFi chua ket noi, bo qua POST");
                break;
            }
            sendDataToServer();

            if (sosHttpPending) {
                sosHttpPending = false;
                Serial.println("[HTTP] Gui lai POST (hang doi SOS)");
                continue;
            }
            if (periodicHttpPending) {
                periodicHttpPending = false;
                Serial.println("[HTTP] Gui lai POST (hang doi dinh ky)");
                continue;
            }
            break;
        }

        httpPostBusy = false;
    }
}

bool requestHttpPost(bool prioritySos = false) {
    if (httpTaskHandle == NULL) {
        Serial.println("[HTTP] Task chua san sang, thu lai sau");
        return false;
    }
    if (httpPostBusy) {
        if (prioritySos) {
            sosHttpPending = true;
            Serial.println("[HTTP] POST dang chay — xep hang gui SOS");
        } else {
            periodicHttpPending = true;
            Serial.println("[HTTP] POST dang chay — xep hang gui dinh ky");
        }
        return true;
    }
    httpPostBusy = true;
    xTaskNotifyGive(httpTaskHandle);
    return true;
}

void serviceModeBtnBeep() {
    if (modeBtnBeepOffAt != 0 && millis() >= modeBtnBeepOffAt) {
        digitalWrite(BUZZER_PIN, LOW);
        modeBtnBeepOffAt = 0;
    }
}

void updateOLEDDisplay();

bool isSosVitalsPhase() {
    unsigned long cycleMs = SOS_PHASE_ALERT_MS + SOS_PHASE_VITALS_MS;
    unsigned long elapsed = (millis() - sosBlinkEpochMs) % cycleMs;
    return elapsed >= SOS_PHASE_ALERT_MS;
}

bool isSosBlinkVisible() {
    if (isSosVitalsPhase()) {
        return false;
    }
    unsigned long blinkCycle = SOS_BLINK_ON_MS + SOS_BLINK_OFF_MS;
    unsigned long elapsed = (millis() - sosBlinkEpochMs) % SOS_PHASE_ALERT_MS;
    return (elapsed % blinkCycle) < SOS_BLINK_ON_MS;
}

void activateSos() {
    if (sosActive) {
        return;
    }
    sosActive = true;
    sosBlinkEpochMs = millis();
    Serial.println("[SOS] KICH HOAT — gui du lieu len may chu (sos=1)");
    readBiometrics();
    requestHttpPost(true);
    if (hasOLED) {
        updateOLEDDisplay();
    }
}

void deactivateSos() {
    if (!sosActive) {
        return;
    }
    sosActive = false;
    sosOffHoldTriggered = false;
    Serial.println("[SOS] DA TAT — gui trang thai sos=0 len may chu");
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    readBiometrics();
    requestHttpPost(true);
    if (hasOLED) {
        updateOLEDDisplay();
    }
}

void checkSosButton() {
    int btnState = digitalRead(SOS_BUTTON_PIN);

    if (btnState == LOW) {
        if (!sosBtnPressedLast) {
            sosBtnPressStart = millis();
            sosBtnPressedLast = true;
            sosOffHoldTriggered = false;
            Serial.println("[SOS_BTN] Nut dang duoc nhan...");
        } else if (sosActive) {
            unsigned long holdMs = millis() - sosBtnPressStart;
            if (holdMs >= SOS_HOLD_OFF_MS && !sosOffHoldTriggered) {
                sosOffHoldTriggered = true;
                deactivateSos();
            }
        }
    } else {
        if (sosBtnPressedLast) {
            unsigned long pressMs = millis() - sosBtnPressStart;
            sosBtnPressedLast = false;

            if (!sosActive && pressMs >= 50 && pressMs < SOS_HOLD_OFF_MS) {
                activateSos();
            }
        }
        sosOffHoldTriggered = false;
    }
}

// Hàm lấy thời gian hiện tại định dạng HH:MM:SS từ NTP
String getFormattedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "--:--:--";
    }
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    return String(timeStr);
}

// =========================================================================
// HÀM TẨY DẤU TIẾNG VIỆT (Chuyển UTF-8 sang mã ASCII chuẩn cho OLED)
// =========================================================================
String removeVietnameseAccents(String str) {
    String s = str;
    
    // Tẩy dấu chữ thường
    String v_a[] = {"á","à","ả","ã","ạ","ă","ắ","ằ","ẳ","ẵ","ặ","â","ấ","ầ","ẩ","ẫ","ậ"};
    for(int i=0; i<17; i++) s.replace(v_a[i], "a");
    
    String v_e[] = {"é","è","ẻ","ẽ","ẹ","ê","ế","ề","ể","ễ","ệ"};
    for(int i=0; i<11; i++) s.replace(v_e[i], "e");
    
    String v_i[] = {"í","ì","ỉ","ĩ","ị"};
    for(int i=0; i<5; i++) s.replace(v_i[i], "i");
    
    String v_o[] = {"ó","ò","ỏ","õ","ọ","ô","ố","ồ","ổ","ỗ","ộ","ơ","ớ","ờ","ở","ỡ","ợ"};
    for(int i=0; i<17; i++) s.replace(v_o[i], "o");
    
    String v_u[] = {"ú","ù","ủ","ũ","ụ","ư","ứ","ừ","ử","ữ","ự"};
    for(int i=0; i<11; i++) s.replace(v_u[i], "u");
    
    String v_y[] = {"ý","ỳ","ỷ","ỹ","ỵ"};
    for(int i=0; i<5; i++) s.replace(v_y[i], "y");
    
    s.replace("đ", "d");

    // Tẩy dấu chữ IN HOA
    String v_A[] = {"Á","À","Ả","Ã","Ạ","Ă","Ắ","Ằ","Ẳ","Ẵ","Ặ","Â","Ấ","Ầ","Ẩ","Ẫ","Ậ"};
    for(int i=0; i<17; i++) s.replace(v_A[i], "A");
    
    String v_E[] = {"É","È","Ẻ","Ẽ","Ẹ","Ê","Ế","Ề","Ể","Ễ","Ệ"};
    for(int i=0; i<11; i++) s.replace(v_E[i], "E");
    
    String v_I[] = {"Í","Ì","Ỉ","Ĩ","Ị"};
    for(int i=0; i<5; i++) s.replace(v_I[i], "I");
    
    String v_O[] = {"Ó","Ò","Ỏ","Õ","Ọ","Ô","Ố","Ồ","Ổ","Ỗ","Ộ","Ơ","Ớ","Ờ","Ở","Ỡ","Ợ"};
    for(int i=0; i<17; i++) s.replace(v_O[i], "O");
    
    String v_U[] = {"Ú","Ù","Ủ","Ũ","Ụ","Ư","Ứ","Ừ","Ử","Ữ","Ự"};
    for(int i=0; i<11; i++) s.replace(v_U[i], "U");
    
    String v_Y[] = {"Ý","Ỳ","Ỷ","Ỹ","Ỵ"};
    for(int i=0; i<5; i++) s.replace(v_Y[i], "Y");
    
    s.replace("Đ", "D");
    
    return s;
}

// =========================================================================
// HÀM RÚT GỌN TÊN BỆNH NHÂN (Hiển thị màn hình OLED)
// =========================================================================
String abbreviateName(String name) {
    name = removeVietnameseAccents(name);
    name.trim(); 
    int lastSpace = name.lastIndexOf(' '); 
    if (lastSpace == -1) return name; 

    String result = "";
    int start = 0;
    while (start < lastSpace) {
        if (name.charAt(start) != ' ') {
            result += name.charAt(start); 
            result += ".";                
            int nextSpace = name.indexOf(' ', start);
            if (nextSpace == -1 || nextSpace >= lastSpace) break;
            start = nextSpace + 1;
        } else {
            start++;
        }
    }
    result += name.substring(lastSpace + 1);
    return result;
}

// =========================================================================
// CẬP NHẬT MÀN HÌNH OLED
// =========================================================================
void updateOLEDDisplay() {
    if (!hasOLED) return;

    display.clear();

    float temp = 0.0;
    float hum = 0.0;
    if (USE_SIMULATOR) {
        temp = currentTemperature;
        hum = currentHumidity;
    } else {
        temp = hasHDC ? currentTemperature : 0.0;
        hum = hasHDC ? currentHumidity : 0.0;
    }

    if (sosActive) {
        display.drawHeader("! CANH BAO SOS !");
        if (isSosVitalsPhase()) {
            display.drawBiometrics(temp, hum, currentHeartRate, currentSpO2);
        } else {
            display.drawSosAlert(isSosBlinkVisible());
        }
        display.drawStatus("Giu nut 5s de tat", true);
        display.show();
        return;
    }

    display.drawHeader("IoT HEALTH MONITOR");
    
    bool heartBeatFlash = (millis() % 1000 < 200) && (currentHeartRate > 0);

    switch (currentDisplayMode) {
        case 0:
            display.drawBiometrics(temp, hum, currentHeartRate, currentSpO2);
            break;
        case 1:
            display.drawCardiacScreen(currentHeartRate, currentSpO2, heartBeatFlash);
            break;
        case 2:
            display.drawEnvScreen(temp, hum);
            break;
        case 3: {
            // Tạo một biến chứa tên đã được viết tắt
            String shortPatientName = abbreviateName(patientName);
            
            // Truyền tên đã viết tắt vào màn hình OLED
            display.drawSystemScreen(shortPatientName, deviceID);
            break;
        }
    }

    String statusMsg = "Offline";
    if (WiFi.status() == WL_CONNECTED) {
        String t = getFormattedTime();
        statusMsg = (t != "--:--:--") ? ("Online " + t) : "Online";
    } else if (apPortalActive) {
        statusMsg = "AP: 192.168.4.1";
    }
    if (savedWifiCount > 0) {
        statusMsg += " " + String(savedWifiCount) + "/" + String(MAX_SAVED_WIFI_NETWORKS);
    }
    if (USE_SIMULATOR) {
        // Nếu ở chế độ giả lập, chúng ta vẫn cố gắng lấy giờ nếu WiFi được kết nối
        String t = getFormattedTime();
        if (t != "--:--:--") {
            statusMsg = "SIM " + t;
        } else {
            statusMsg = "SIM: On dinh";
        }
    }

    display.drawStatus(statusMsg, false);
    
    display.show();
}

void generateDeviceID() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char idStr[20];
    snprintf(idStr, sizeof(idStr), "ESP32_%02X%02X%02X", mac[3], mac[4], mac[5]);
    deviceID = String(idStr);
    Serial.print("[SYSTEM] MAC: ");
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print(macStr);
    Serial.print(" | Gen DeviceID: ");
    Serial.println(deviceID);
}

void loadConfig() {
    preferences.begin("health-cfg", true);
    patientName = preferences.getString("patient_name", DEFAULT_PATIENT_NAME);
    preferences.end();

    if (patientName.length() == 0 || patientName == "Chua gan benh nhan" || patientName == "Nguyen Van An") {
        patientName = DEFAULT_PATIENT_NAME;
    }

    loadWifiNetworksFromFlash();

    Serial.println("[FLASH] Da tai cau hinh:");
    Serial.print("  So mang WiFi da luu: "); Serial.println(savedWifiCount);
    for (int i = 0; i < savedWifiCount; i++) {
        Serial.print("    ["); Serial.print(i); Serial.print("] ");
        Serial.println(savedWifiNetworks[i].ssid);
    }
    Serial.print("  API Server: "); Serial.println(API_SERVER_LABEL);
    Serial.print("  Patient Name: "); Serial.println(patientName);
}

void savePatientName(const String& name) {
    preferences.begin("health-cfg", false);
    preferences.putString("patient_name", name);
    preferences.end();
}

String buildWifiSetupStyles() {
    String css = "<style>";
    css += "body { font-family: 'Inter', -apple-system, sans-serif; background: #0b0f19; color: #f1f5f9; padding: 20px; display: flex; justify-content: center; align-items: flex-start; min-height: 90vh; margin: 0; }";
    css += ".card { background: #1e293b; padding: 30px; border-radius: 16px; max-width: 480px; width: 100%; box-shadow: 0 10px 25px -5px rgba(0,0,0,0.3); border: 1px solid #334155; margin-bottom: 20px; }";
    css += "h2 { margin-top: 0; color: #38bdf8; font-size: 24px; text-align: center; font-weight: 700; margin-bottom: 5px; }";
    css += ".subtitle { text-align: center; color: #94a3b8; font-size: 13px; margin-bottom: 25px; }";
    css += "label { display: block; margin: 15px 0 6px; font-size: 13px; color: #94a3b8; font-weight: 500; letter-spacing: 0.05em; }";
    css += "input, select { width: 100%; padding: 12px; background: #0f172a; border: 1px solid #475569; border-radius: 8px; color: #fff; box-sizing: border-box; font-size: 14px; transition: border 0.2s; }";
    css += "input:focus, select:focus { border-color: #38bdf8; outline: none; }";
    css += "select option { background: #1e293b; color: #fff; }";
    css += "button, .btn { display: inline-block; width: 100%; padding: 14px; background: #38bdf8; border: none; border-radius: 8px; color: #0f172a; font-weight: 700; margin-top: 15px; cursor: pointer; transition: all 0.2s; font-size: 15px; text-align: center; text-decoration: none; box-sizing: border-box; }";
    css += "button:hover, .btn:hover { background: #0ea5e9; }";
    css += ".btn-secondary { background: #22c55e; color: #0f172a; }";
    css += ".btn-secondary:hover { background: #16a34a; }";
    css += ".note { font-size: 12px; color: #64748b; text-align: center; margin-top: 20px; line-height: 1.5; }";
    css += ".device-info { background: #0f172a; padding: 8px 12px; border-radius: 6px; text-align: center; font-size: 12px; color: #38bdf8; font-family: monospace; margin-bottom: 15px; }";
    css += ".status-box { background: #0f172a; padding: 12px; border-radius: 8px; margin-bottom: 15px; font-size: 13px; line-height: 1.6; }";
    css += ".status-online { color: #22c55e; }";
    css += ".status-offline { color: #f59e0b; }";
    css += ".saved-list { list-style: none; padding: 0; margin: 0; }";
    css += ".saved-item { display: flex; justify-content: space-between; align-items: center; background: #0f172a; padding: 10px 12px; border-radius: 8px; margin-bottom: 8px; font-size: 13px; }";
    css += ".saved-item.active { border: 1px solid #22c55e; }";
    css += ".btn-delete { width: auto; padding: 6px 12px; margin: 0; background: #ef4444; color: #fff; font-size: 12px; }";
    css += ".btn-delete:hover { background: #dc2626; }";
    css += ".badge { font-size: 11px; color: #22c55e; margin-left: 6px; }";
    css += "</style>";
    return css;
}

int cachedWifiCount = -1;
unsigned long lastWifiScanTime = 0;

void handleRoot() {
    if (server.hasArg("scan")) {
        Serial.println("[WIFI SETUP] Nguoi dung yeu cau quet WiFi...");
        WiFi.scanDelete();
        WiFi.scanNetworks(true);
        unsigned long scanStart = millis();
        while (WiFi.scanComplete() < 0 && millis() - scanStart < 15000) {
            serviceWifiPortal();
            delay(50);
        }
        cachedWifiCount = WiFi.scanComplete();
        if (cachedWifiCount < 0) cachedWifiCount = 0;
        lastWifiScanTime = millis();
        Serial.print("[WIFI SETUP] Tim thay: ");
        Serial.println(cachedWifiCount);
    }
    int n = cachedWifiCount;

    bool isConnected = (WiFi.status() == WL_CONNECTED);
    String currentSsid = isConnected ? WiFi.SSID() : "";

    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<meta http-equiv=\"refresh\" content=\"30\">";
    html += "<title>Cau hinh WiFi - IoT Health</title>";
    html += buildWifiSetupStyles();
    html += "<script>";
    html += "function selectWifi(element) {";
    html += "  var ssidInput = document.getElementById('ssid');";
    html += "  if(element.value !== '') { ssidInput.value = element.value; }";
    html += "}";
    html += "</script>";
    html += "</head><body>";
    html += "<div class=\"card\">";
    html += "<h2>CAU HINH WIFI</h2>";
    html += "<div class=\"subtitle\">Giam Sat Suc Khoe IoT - HUST</div>";
    html += "<div class=\"device-info\">Thiet bi: " + deviceID + " | AP: 192.168.4.1</div>";

    html += "<div class=\"status-box\">";
    if (isConnected) {
        html += "<span class=\"status-online\">Dang ket noi: <b>" + currentSsid + "</b></span><br>";
        html += "IP: " + WiFi.localIP().toString();
    } else {
        html += "<span class=\"status-offline\">Chua ket noi WiFi nao</span><br>";
        html += "Ket noi vao WiFi <b>" + deviceID + "</b> de cau hinh.";
    }
    html += "</div>";

    html += "<form action=\"/save_patient\" method=\"POST\">";
    html += "<label>TEN BENH NHAN</label>";
    html += "<input type=\"text\" name=\"patient_name\" value=\"" + patientName + "\" required placeholder=\"Nhap ten benh nhan\">";
    html += "<button type=\"submit\" class=\"btn-secondary\">LUU TEN BENH NHAN</button>";
    html += "</form>";

    if (savedWifiCount > 0) {
        html += "<form action=\"/reconnect\" method=\"POST\">";
        html += "<button type=\"submit\" class=\"btn-secondary\">KET NOI LAI CAC MANG DA LUU</button>";
        html += "</form>";
    }

    if (savedWifiCount > 0) {
        html += "<label>WIFI DA LUU (" + String(savedWifiCount) + "/" + String(MAX_SAVED_WIFI_NETWORKS) + ")</label>";
        html += "<ul class=\"saved-list\">";
        for (int i = 0; i < savedWifiCount; i++) {
            bool isActive = isConnected && savedWifiNetworks[i].ssid == currentSsid;
            html += "<li class=\"saved-item" + String(isActive ? " active" : "") + "\">";
            html += "<span>" + savedWifiNetworks[i].ssid;
            if (isActive) html += "<span class=\"badge\">Dang dung</span>";
            html += "</span>";
            html += "<form action=\"/delete_wifi\" method=\"POST\" style=\"margin:0;\">";
            html += "<input type=\"hidden\" name=\"ssid\" value=\"" + savedWifiNetworks[i].ssid + "\">";
            html += "<button type=\"submit\" class=\"btn-delete\">Xoa</button>";
            html += "</form>";
            html += "</li>";
        }
        html += "</ul>";
    }

    html += "<form action=\"/save\" method=\"POST\">";
    html += "<label>CHON WIFI XUNG QUANH</label>";
    html += "<a href=\"/?scan=1\" class=\"btn-secondary\" style=\"margin-top:0;margin-bottom:10px;\">QUET WIFI XUNG QUANH</a>";
    if (n < 0) {
        html += "<select onchange=\"selectWifi(this)\"><option value=\"\">Chua quet. Bam nut quet WiFi phia tren.</option></select>";
    } else if (n == 0) {
        html += "<select onchange=\"selectWifi(this)\"><option value=\"\">Khong tim thay mang nao. Nhap tay SSID ben duoi.</option></select>";
    } else {
        html += "<select onchange=\"selectWifi(this)\">";
        html += "<option value=\"\">-- Chon mang WiFi --</option>";
        for (int i = 0; i < n; ++i) {
            String ssid = WiFi.SSID(i);
            if (ssid.length() == 0) continue;
            int rssi = WiFi.RSSI(i);
            String authType = "Secured";
            switch (WiFi.encryptionType(i)) {
                case WIFI_AUTH_OPEN: authType = "Open"; break;
                case WIFI_AUTH_WEP: authType = "WEP"; break;
                case WIFI_AUTH_WPA_PSK: authType = "WPA"; break;
                case WIFI_AUTH_WPA2_PSK: authType = "WPA2"; break;
                case WIFI_AUTH_WPA_WPA2_PSK: authType = "WPA/WPA2"; break;
                default: break;
            }
            html += "<option value=\"" + ssid + "\">" + ssid + " (" + String(rssi) + "dBm, " + authType + ")</option>";
        }
        html += "</select>";
    }

    html += "<label>TEN WIFI (SSID)</label>";
    html += "<input type=\"text\" name=\"ssid\" id=\"ssid\" required placeholder=\"Nhap ten WiFi\">";
    html += "<label>MAT KHAU WIFI</label>";
    html += "<input type=\"password\" name=\"pass\" placeholder=\"Nhap mat khau WiFi\">";
    html += "<button type=\"submit\">KET NOI & LUU WIFI</button>";
    html += "</form>";
    html += "<p class=\"note\">Huong dan:<br>1. Ket noi dien thoai vao WiFi <b>" + deviceID + "</b> (khong mat khau)<br>2. TAT du lieu di dong (4G/5G) tren dien thoai<br>3. Mo trinh duyet vao <b>http://192.168.4.1</b><br>Thiet bi chay dong thoi AP + STA.</p>";
    html += "</div></body></html>";

    server.send(200, "text/html; charset=utf-8", html);
}

void handleSavePatient() {
    if (!server.hasArg("patient_name")) {
        server.send(400, "text/plain", "Thieu ten benh nhan");
        return;
    }

    String reqName = server.arg("patient_name");
    reqName.trim();
    if (reqName.length() == 0) {
        server.send(400, "text/plain", "Ten benh nhan khong duoc de trong");
        return;
    }

    patientName = reqName;
    savePatientName(reqName);

    Serial.print("[WIFI SETUP] Da luu ten benh nhan (khong doi WiFi): ");
    Serial.println(reqName);

    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += buildWifiSetupStyles();
    html += "</head><body><div class=\"card\"><div class=\"status-box\">";
    html += "<h2 style=\"color:#22c55e;text-align:center;\">Da Luu Ten Benh Nhan!</h2>";
    html += "<p style=\"text-align:center;\">Ten hien tai: <b>" + reqName + "</b></p>";
    html += "<p style=\"text-align:center;color:#94a3b8;\">WiFi khong thay doi.</p>";
    html += "<a href=\"/\" class=\"btn\">QUAY LAI TRANG CAU HINH</a>";
    html += "</div></div></body></html>";
    server.send(200, "text/html; charset=utf-8", html);

    if (hasOLED) {
        updateOLEDDisplay();
    }
}

void handleSave() {
    if (!server.hasArg("ssid")) {
        server.send(400, "text/plain", "Yeu cau khong hop le");
        return;
    }

    String reqSsid = server.arg("ssid");
    String reqPass = server.arg("pass");
    reqSsid.trim();
    if (reqSsid.length() == 0) {
        server.send(400, "text/plain", "SSID khong duoc de trong");
        return;
    }

    Serial.println("\n[WIFI SETUP] Bat dau test ket noi...");
    Serial.print("  SSID: "); Serial.println(reqSsid);

    if (hasOLED) {
        display.clear();
        display.drawHeader("IoT HEALTH MONITOR");
        display.drawText(4, 20, "Kiem tra WiFi...", 1);
        display.drawText(4, 32, reqSsid, 1);
        display.drawStatus("Dang ket noi...", false);
        display.show();
    }

    bool connected = connectToNetwork(reqSsid, reqPass, WIFI_CONNECT_TIMEOUT_MS);

    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, LOW);

    if (connected) {
        Serial.println("[WIFI SETUP] Ket noi thanh cong! Dang luu vao Flash.");

        addOrUpdateWifiNetwork(reqSsid, reqPass);
        cachedWifiCount = -1;

        String successHtml = "<!DOCTYPE html><html><head>";
        successHtml += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
        successHtml += buildWifiSetupStyles();
        successHtml += "</head><body><div class=\"card\"><div class=\"status-box\">";
        successHtml += "<h2 style=\"color:#22c55e;text-align:center;\">Ket Noi Thanh Cong!</h2>";
        successHtml += "<p style=\"text-align:center;\">Da luu WiFi <b>" + reqSsid + "</b> vao bo nho.</p>";
        successHtml += "<p style=\"text-align:center;\">IP: " + WiFi.localIP().toString() + "</p>";
        successHtml += "<p style=\"text-align:center;color:#94a3b8;\">Thiet bi tiep tuc hoat dong binh thuong (AP + STA).</p>";
        successHtml += "<a href=\"/\" class=\"btn\">QUAY LAI TRANG CAU HINH</a>";
        successHtml += "</div></div></body></html>";
        server.send(200, "text/html; charset=utf-8", successHtml);

        if (hasOLED) {
            display.clear();
            display.drawHeader("IoT HEALTH MONITOR");
            display.drawText(4, 24, "WIFI OK!", 1);
            display.drawText(4, 36, reqSsid.substring(0, 16), 1);
            display.drawStatus(WiFi.localIP().toString(), false);
            display.show();
        }

        digitalWrite(BUZZER_PIN, HIGH);
        delay(150);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
        digitalWrite(BUZZER_PIN, HIGH);
        delay(150);
        digitalWrite(BUZZER_PIN, LOW);
    } else {
        Serial.println("[WIFI SETUP] Ket noi that bai! Khong luu.");

        // Restore previous connection if possible
        tryConnectSavedNetworks();

        if (hasOLED) {
            display.clear();
            display.drawHeader("IoT HEALTH MONITOR");
            display.drawText(4, 20, "WIFI THAT BAI!", 1);
            display.drawText(4, 32, "Kiem tra mat khau", 1);
            display.drawStatus("192.168.4.1", false);
            display.show();
        }

        digitalWrite(BUZZER_PIN, HIGH);
        delay(800);
        digitalWrite(BUZZER_PIN, LOW);

        String errorHtml = "<!DOCTYPE html><html><head>";
        errorHtml += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
        errorHtml += buildWifiSetupStyles();
        errorHtml += "</head><body><div class=\"card\"><div class=\"status-box\">";
        errorHtml += "<h2 style=\"color:#ef4444;text-align:center;\">Ket Noi That Bai!</h2>";
        errorHtml += "<p style=\"text-align:center;\">Khong ket noi duoc toi <b>" + reqSsid + "</b></p>";
        errorHtml += "<p style=\"text-align:center;\">Vui long kiem tra SSID va mat khau.</p>";
        errorHtml += "<a href=\"/\" class=\"btn\">QUAY LAI TRANG CAU HINH</a>";
        errorHtml += "</div></div></body></html>";
        server.send(200, "text/html; charset=utf-8", errorHtml);
    }
}

void handleReconnect() {
    Serial.println("[WIFI SETUP] Nguoi dung yeu cau ket noi lai...");

    if (savedWifiCount == 0) {
        String html = "<!DOCTYPE html><html><head>";
        html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
        html += buildWifiSetupStyles();
        html += "</head><body><div class=\"card\"><div class=\"status-box\">";
        html += "<h2 style=\"color:#f59e0b;text-align:center;\">Chua Co WiFi Luu</h2>";
        html += "<p style=\"text-align:center;\">Hay them mang WiFi truoc khi ket noi lai.</p>";
        html += "<a href=\"/\" class=\"btn\">QUAY LAI</a>";
        html += "</div></div></body></html>";
        server.send(200, "text/html; charset=utf-8", html);
        return;
    }

    if (hasOLED) {
        display.clear();
        display.drawHeader("IoT HEALTH MONITOR");
        display.drawText(4, 20, "Ket noi lai...", 1);
        display.drawText(4, 32, String(savedWifiCount) + " mang da luu", 1);
        display.drawStatus("Dang thu...", false);
        display.show();
    }

    WiFi.disconnect();
    connectedSSID = "";
    delay(200);

    bool connected = tryConnectSavedNetworks();

    String resultHtml = "<!DOCTYPE html><html><head>";
    resultHtml += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    resultHtml += buildWifiSetupStyles();
    resultHtml += "</head><body><div class=\"card\"><div class=\"status-box\">";

    if (connected) {
        resultHtml += "<h2 style=\"color:#22c55e;text-align:center;\">Ket Noi Lai Thanh Cong!</h2>";
        resultHtml += "<p style=\"text-align:center;\">Dang dung: <b>" + connectedSSID + "</b></p>";
        resultHtml += "<p style=\"text-align:center;\">IP: " + WiFi.localIP().toString() + "</p>";

        if (hasOLED) {
            display.clear();
            display.drawHeader("IoT HEALTH MONITOR");
            display.drawText(4, 24, "WIFI OK!", 1);
            display.drawText(4, 36, connectedSSID.substring(0, 16), 1);
            display.drawStatus(WiFi.localIP().toString(), false);
            display.show();
        }

        digitalWrite(BUZZER_PIN, HIGH);
        delay(150);
        digitalWrite(BUZZER_PIN, LOW);
    } else {
        resultHtml += "<h2 style=\"color:#ef4444;text-align:center;\">Ket Noi Lai That Bai</h2>";
        resultHtml += "<p style=\"text-align:center;\">Khong ket noi duoc mang nao trong " + String(savedWifiCount) + " mang da luu.</p>";
        resultHtml += "<p style=\"text-align:center;color:#94a3b8;\">Kiem tra mang WiFi hoac them mang moi.</p>";

        if (hasOLED) {
            display.clear();
            display.drawHeader("IoT HEALTH MONITOR");
            display.drawText(4, 24, "WIFI FAIL", 1);
            display.drawText(4, 36, "Thu lai sau...", 1);
            display.drawStatus("192.168.4.1", false);
            display.show();
        }
    }

    resultHtml += "<a href=\"/\" class=\"btn\">QUAY LAI TRANG CAU HINH</a>";
    resultHtml += "</div></div></body></html>";
    server.send(200, "text/html; charset=utf-8", resultHtml);
}

void handleDeleteWifi() {
    if (!server.hasArg("ssid")) {
        server.send(400, "text/plain", "Thieu SSID");
        return;
    }

    String reqSsid = server.arg("ssid");
    bool wasConnected = (WiFi.status() == WL_CONNECTED && WiFi.SSID() == reqSsid);

    if (removeWifiNetwork(reqSsid)) {
        Serial.print("[WIFI SETUP] Da xoa mang: ");
        Serial.println(reqSsid);

        if (wasConnected) {
            WiFi.disconnect();
            connectedSSID = "";
            delay(200);
            tryConnectSavedNetworks();
        }

        cachedWifiCount = -1;

        String html = "<!DOCTYPE html><html><head>";
        html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
        html += buildWifiSetupStyles();
        html += "</head><body><div class=\"card\"><div class=\"status-box\">";
        html += "<h2 style=\"color:#22c55e;text-align:center;\">Da Xoa WiFi</h2>";
        html += "<p style=\"text-align:center;\">Mang <b>" + reqSsid + "</b> da duoc xoa khoi bo nho.</p>";
        html += "<a href=\"/\" class=\"btn\">QUAY LAI TRANG CAU HINH</a>";
        html += "</div></div></body></html>";
        server.send(200, "text/html; charset=utf-8", html);
    } else {
        server.send(404, "text/plain", "Khong tim thay mang WiFi");
    }
}

void handleNotFound() {
    // Captive portal: chuyen moi request ve trang cau hinh
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
}

void initWifiPortal() {
    apPortalActive = true;
    apPortalStartMs = millis();

    WiFi.persistent(false);
    WiFi.mode(WIFI_AP_STA);
    delay(50);

    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    String apName = deviceID;
    bool apStarted = WiFi.softAP(apName.c_str(), NULL, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);
    if (!apStarted) {
        Serial.println("[WIFI] AP khoi tao that bai lan 1, thu lai channel 6...");
        delay(100);
        apStarted = WiFi.softAP(apName.c_str(), NULL, 6, 0, WIFI_AP_MAX_CONN);
    }

    dnsServer.start(53, "*", apIP);

    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/save_patient", HTTP_POST, handleSavePatient);
    server.on("/reconnect", HTTP_POST, handleReconnect);
    server.on("/delete_wifi", HTTP_POST, handleDeleteWifi);
    server.onNotFound(handleNotFound);
    server.begin();

    Serial.println("\n-----------------------------------------");
    Serial.println("[WIFI] Che do AP + STA (Dual Mode)");
    Serial.print("  AP Setup: "); Serial.println(apName);
    Serial.print("  AP IP: "); Serial.println(WiFi.softAPIP());
    Serial.print("  AP OK: "); Serial.println(apStarted ? "YES" : "NO");
    Serial.println("  IP cau hinh: http://192.168.4.1");
    Serial.println("-----------------------------------------\n");

    if (!apStarted && hasOLED) {
        display.clear();
        display.drawHeader("IoT HEALTH MONITOR");
        display.drawText(4, 24, "LOI KHOI TAO AP!", 1);
        display.drawStatus("Kiem tra lai", true);
        display.show();
    }
}

void startAPMode() {
    initWifiPortal();

    if (hasOLED) {
        display.clear();
        display.drawHeader("IoT HEALTH MONITOR");
        display.drawText(4, 20, "WiFi AP+STA", 1);
        display.drawText(4, 32, "Ket noi AP:", 1);
        display.drawText(4, 44, deviceID, 1);
        display.drawStatus("192.168.4.1", false);
        display.show();
    }
}

void checkModeButton() {
    int btnState = digitalRead(MODE_BUTTON_PIN);
    
    if (btnState == LOW) {
        if (!modeBtnPressedLast) {
            modeBtnPressStart = millis();
            modeBtnPressedLast = true;
            Serial.println("[MODE_BTN] Nut dang duoc nhan...");
        } else {
            unsigned long pressDuration = millis() - modeBtnPressStart;
            if (pressDuration > 1500) {
                digitalWrite(LED_RED_PIN, (millis() / 250) % 2);
                digitalWrite(LED_GREEN_PIN, LOW);
            }
            if (pressDuration >= RESET_HOLD_TIME_MS) {
                Serial.println("\n[MODE_BTN] NHAN GIU > 10s! DANG XOA CAU HINH VE MAC DINH...");
                
                if (hasOLED) {
                    display.clear();
                    display.drawHeader("IoT HEALTH MONITOR");
                    display.drawText(4, 24, "FACTORY RESET...", 1);
                    display.drawStatus("Vui long cho...", true);
                    display.show();
                }

                digitalWrite(BUZZER_PIN, HIGH);
                digitalWrite(LED_RED_PIN, HIGH);
                
                preferences.begin("health-cfg", false);
                preferences.clear();
                preferences.end();
                
                delay(3000);
                digitalWrite(BUZZER_PIN, LOW);
                digitalWrite(LED_RED_PIN, LOW);
                
                ESP.restart();
            }
        }
    } else {
        if (modeBtnPressedLast) {
            unsigned long pressDuration = millis() - modeBtnPressStart;
            modeBtnPressedLast = false;
            Serial.println("[MODE_BTN] Da nha nut.");
            digitalWrite(LED_RED_PIN, LOW);
            
            // Xử lý nhấn nhanh chuyển màn hình (50ms - 1500ms)
            if (pressDuration >= 50 && pressDuration < 1500) {
                currentDisplayMode = (currentDisplayMode + 1) % MAX_DISPLAY_MODES;
                Serial.print("[MODE_BTN] Chuyen sang Man hinh Mode: ");
                Serial.println(currentDisplayMode);

                digitalWrite(BUZZER_PIN, HIGH);
                modeBtnBeepOffAt = millis() + 50;

                if (hasOLED) {
                    updateOLEDDisplay();
                }
            }
        }
    }
}

// =========================================================================
// SETUP & LOOP CHÍNH
// =========================================================================
void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n--- HE THONG GIAM SAT SUC KHOE IoT KHOI DONG ---");

    // Khởi tạo các chân cảnh báo và nút nhấn
    initAlertPins();
    pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(SOS_BUTTON_PIN, INPUT_PULLUP);

    // Bat AP portal som nhat co the (truoc khi init cam bien)
    generateDeviceID();
    loadConfig();
    initWifiPortal();

    // 1. Khởi tạo OLED
    Serial.println("[OLED] Khoi tao...");
    hasOLED = display.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setTimeOut(100);
    
    if (hasOLED) {
        Serial.println("[OLED] Khoi tao SSD1306 thanh cong!");
        display.clear();
        display.drawHeader("IoT HEALTH MONITOR");
        display.drawText(4, 20, "Khoi dong thiet bi...", 1);
        display.drawStatus("Setup...", false);
        display.show();
    } else {
        Serial.println("[OLED] Loi! Khong tim thay man hinh OLED SSD1306.");
    }

    // Khởi chạy phần cứng cảm biến
    scanI2CBus();

    if (USE_SIMULATOR) {
        Serial.println("[SYSTEM] Dang chay o che do GIAP LAP (Simulator Mode).");
        if (hasOLED) {
            display.clear();
            display.drawHeader("IoT HEALTH MONITOR");
            display.drawText(4, 20, "Mode: GIAP LAP (SIM)", 1);
            display.drawText(4, 32, "Khong can cam hardware", 1);
            display.drawStatus("Khoi chay...", false);
            display.show();
            delay(1000);
        }
    } else {
        Serial.println("[SYSTEM] Dang chay o che do DOC PHAN CUNG THAT.");
        
        // 2. Khởi tạo cảm biến HDC2022
        Serial.println("[HDC2022] Khoi tao...");
        hdc.Init(0x40);
        float testTemp = hdc.get_Temperature();
        if (testTemp > -40.0 && testTemp < 120.0) {
            hasHDC = true;
            Serial.println("[HDC2022] Khoi tao thanh cong!");
        } else {
            Serial.println("[HDC2022] Loi! Khong tim thay cam bien HDC2022.");
        }

        // 3. Khởi tạo cảm biến MAX30102
        Serial.println("[MAX30102] Khoi tao...");
        hasMAX = particleSensor.begin();
        if (hasMAX) {
            Serial.println("[MAX30102] Khoi tao thanh cong!");
        } else {
            Serial.println("[MAX30102] Loi! Khong tim thay cam bien MAX30102.");
        }
    }

    // 4. Kết nối WiFi
    setupWiFi();

    xTaskCreate(
        httpPostTask,
        "httpPost",
        20480,
        NULL,
        1,
        &httpTaskHandle);
}

void loop() {
    unsigned long now = millis();

    checkModeButton();
    checkSosButton();
    serviceModeBtnBeep();

    // Uu tien doc MAX30102 truoc (FIFO day khi WiFi chiem CPU)
    if (!USE_SIMULATOR) {
        processMax30102Fifo();
    }

    // Phuc vu AP portal (DNS + WebServer)
    if (apPortalActive) {
        dnsServer.processNextRequest();
        server.handleClient();
    }

    processStaConnectNonBlocking();

    // In log debug chi tiết định kỳ mỗi 1 giây
    static unsigned long lastDebugTime = 0;
    if (now - lastDebugTime >= 1000) {
        lastDebugTime = now;
        if (!USE_SIMULATOR) {
            Serial.println("=================================================================");
            Serial.print("[DEBUG CORE] WiFi STA: "); 
            Serial.print(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED");
            Serial.print(" | AP IP: "); Serial.print(WiFi.softAPIP());
            Serial.print(" | AP clients: "); Serial.print(WiFi.softAPgetStationNum());
            Serial.print(" | HDC2022: ");
            Serial.print(hasHDC ? "ONLINE" : "OFFLINE");
            if (hasHDC) {
                Serial.print(" (T: "); Serial.print(currentTemperature, 1); 
                Serial.print("C, H: "); Serial.print(currentHumidity, 1); Serial.print("%)");
            }
            Serial.print(" | MAX30102: "); 
            Serial.println(hasMAX ? "ONLINE" : "OFFLINE");

            if (hasMAX) {
                uint8_t readPtr = particleSensor.readRegister(0x06);
                uint8_t writePtr = particleSensor.readRegister(0x04);
                uint8_t partId = particleSensor.readRegister(0xFF);
                
                Serial.print("  -> PartID: 0x"); Serial.print(partId, HEX);
                Serial.print(" | FIFO R_Ptr: "); Serial.print(readPtr);
                Serial.print(" | W_Ptr: "); Serial.print(writePtr);
                Serial.print(" | SampleReady: "); Serial.println(lastSampleReady ? "YES" : "NO");
                
                Serial.print("  -> Raw IR: "); Serial.print(lastRawIR);
                Serial.print(" | Raw RED: "); Serial.print(lastRawRed);
                Serial.print(" | HR: "); Serial.print(currentHeartRate, 1);
                Serial.print(" bpm | SpO2: "); Serial.print(currentSpO2, 1); Serial.println(" %");

                if (lastRawIR == 0 && lastRawRed == 0) {
                    Serial.println("  [!] CHUAN DOAN: Du lieu tho = 0. Chip co the chua duoc cap nguon thich hop hoac thanh ghi cau hinh bi reset.");
                } else if (last_beat_time == 0 || lastRawIR < 5000) {
                    Serial.println("  [!] CHUAN DOAN: Chua phat hien da tiep xuc voi cam bien. Vui long dat thiet bi sat da.");
                } else if (lastRawIR >= 5000 && lastRawIR <= 15000) {
                    Serial.println("  [!] CHUAN DOAN: Tiep xuc qua nhe. Vui long an nhe hoac deo chat day day them mot chut.");
                } else if (currentHeartRate == 0) {
                    Serial.println("  [*] Tin hieu tot — dang tinh nhip tim, giu tay on dinh 5-10 giay...");
                } else {
                    Serial.println("  [*] CHUAN DOAN: Do sinh trac hoc on dinh.");
                }
            } else {
                Serial.println("  [!] CHUAN DOAN: Khong tim thay MAX30102! Kiem tra ngay ket noi I2C SDA (IO7) va SCL (IO8).");
            }
            Serial.println("=================================================================");
        }
    }

    // Kiểm tra và tự động kết nối lại WiFi
    if (now - lastWifiCheckTime >= 10000) {
        lastWifiCheckTime = now;
        checkWiFiConnection();
    }

    // 2. Đọc cảm biến liên tục
    if (USE_SIMULATOR) {
        static unsigned long lastSimRead = 0;
        if (now - lastSimRead >= 1000) {
            lastSimRead = now;
            readBiometrics();
        }
    } else {
        readBiometrics();
    }

    // 3. Cập nhật cảnh báo Buzzer & LED liên tục
    bool wifiConnecting = (savedWifiCount > 0 && WiFi.status() != WL_CONNECTED);
    handleAlerts(currentHeartRate, currentSpO2, currentTemperature, wifiConnecting);

    // 4. Cập nhật OLED (100ms khi phase SOS nháy, 500ms còn lại)
    unsigned long displayIntervalMs = 500UL;
    if (sosActive && !isSosVitalsPhase()) {
        displayIntervalMs = 100UL;
    }
    if (now - lastDisplayTime >= displayIntervalMs) {
        lastDisplayTime = now;
        updateOLEDDisplay();
    }

    // 5. Gửi dữ liệu lên API Server định kỳ (kèm sos=1 khi đang SOS)
    if (now - lastMeasureTime >= MEASURE_INTERVAL_MS) {
        Serial.println("\n--- CHU KY CAP NHAT THONG TIN ---");
        Serial.print("Nhiet do: "); Serial.print(currentTemperature, 1); Serial.println(" C");
        Serial.print("Do am: "); Serial.print(currentHumidity, 1); Serial.println(" %");
        Serial.print("Nhip tim: "); Serial.print(currentHeartRate, 1); Serial.println(" bpm");
        Serial.print("SpO2: "); Serial.print(currentSpO2, 1); Serial.println(" %");

        if (requestHttpPost()) {
            lastMeasureTime = now;
        }
    }
}
