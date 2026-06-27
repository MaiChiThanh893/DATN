#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// CẤU HÌNH PHẦN CỨNG (PINOUT)
// ==========================================
// Chân giao tiếp I2C cho ESP32-C3
#define I2C_SDA_PIN     7
#define I2C_SCL_PIN     8

// Chân Buzzer & LED trạng thái
#define BUZZER_PIN      3                   // Chân kết nối còi Buzzer active/passive
#define LED_RED_PIN     4                   // Chân kết nối LED đỏ cảnh báo
#define LED_GREEN_PIN   5                   // Chân kết nối LED xanh ổn định

// Chân nút nhấn MODE (GPIO0) và thời gian nhấn giữ để Reset Factory (10 giây)
#define MODE_BUTTON_PIN 0
#define RESET_HOLD_TIME_MS 10000

// Nút SOS (GPIO6): nhấn bật SOS, giữ 5 giây để tắt
#define SOS_BUTTON_PIN       6
#define SOS_HOLD_OFF_MS      5000
#define SOS_PHASE_ALERT_MS   10000   // 10s: chỉ nháy chữ SOS
#define SOS_PHASE_VITALS_MS  10000   // 10s: hiển thị thông số sinh học
#define SOS_BLINK_ON_MS      800     // nhịp nháy SOS trong phase cảnh báo
#define SOS_BLINK_OFF_MS     400

// ==========================================
// CẤU HÌNH CHẾ ĐỘ GIẢ LẬP (SIMULATION)
// ==========================================
// Đặt true để tự động sinh dữ liệu đo sinh động (thích hợp cho demo/báo cáo)
// Đặt false để đọc từ cảm biến phần cứng thật (MAX30102 & HDC2022)
#define USE_SIMULATOR   false

// ==========================================
// CẤU HÌNH GỬI DỮ LIỆU
// ==========================================
#define MEASURE_INTERVAL_MS 10000            // Tần suất gửi dữ liệu (10 giây/lần)

// ==========================================
// CẤU HÌNH WIFI (AP + STA DUAL MODE)
// ==========================================
#define MAX_SAVED_WIFI_NETWORKS  10         // Số mạng WiFi tối đa lưu trong Flash
#define WIFI_CONNECT_TIMEOUT_MS  10000      // Thời gian chờ kết nối mỗi mạng (ms)
#define WIFI_RECONNECT_INTERVAL_MS 30000    // Thử kết nối lại khi mất mạng (ms)

#define WIFI_AP_CHANNEL  1
#define WIFI_AP_MAX_CONN 4
#define WIFI_STA_DEFER_MS 20000           // Hoan STA sau boot de AP on dinh truoc
#define WIFI_STA_PAUSE_WHEN_AP_CLIENT true // Khong thu STA khi co thiet bi ket noi AP

#endif

