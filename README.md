# MCT IoT Monitoring — Hệ thống giám sát sức khỏe thời gian thực

Hệ thống giám sát các chỉ số sức khỏe bệnh nhân (nhịp tim, SpO₂, nhiệt độ, độ ẩm) thu thập từ thiết bị nhúng ESP32-C3, truyền lên web dashboard qua WiFi, hiển thị theo thời gian thực kèm cảnh báo tự động khi chỉ số vượt ngưỡng an toàn.

**Đồ án tốt nghiệp — Mai Chí Thanh**

---

## Tổng quan hệ thống

```
┌─────────────────────────────┐        HTTP POST        ┌─────────────────────────┐
│        ESP32-C3             │ ──────────────────────► │     Web Dashboard       │
│                             │                         │                         │
│  ┌──────────┐ ┌──────────┐  │    (WiFi / Internet)    │  ┌─────────┐ ┌───────┐  │
│  │ MAX30102 │ │ HDC2022  │  │                         │  │  PHP    │ │ JSON  │  │
│  │ HR / SpO₂│ │ Temp /   │  │◄─────────────────────── │  │ Backend │ │  DB   │  │
│  └──────────┘ │ Humidity │  │       Polling           │  └─────────┘ └───────┘  │
│               └──────────┘  │                         │  ┌──────────────────┐   │
│  ┌─────────────────────┐    │                         │  │ Chart.js Frontend│   │
│  │   OLED SSD1306      │    │                         │  └──────────────────┘   │
│  └─────────────────────┘    │                         └─────────────────────────┘
└─────────────────────────────┘
```

---

## Tính năng chính

### Thiết bị (ESP32-C3)
- Đo nhịp tim và SpO₂ qua cảm biến MAX30102 (giao tiếp I2C)
- Đo nhiệt độ và độ ẩm qua cảm biến HDC2022 (giao tiếp I2C)
- Hiển thị chỉ số trực tiếp trên màn hình OLED 128×64 (xoay vòng 4 chế độ hiển thị)
- Cảnh báo tại chỗ: đèn LED đỏ và còi buzzer khi chỉ số bất thường
- Nút SOS khẩn cấp (giữ 5 giây để gửi tín hiệu, giữ lại 5 giây để hủy)
- Kết nối WiFi kép: AP (để cấu hình) + STA (để gửi dữ liệu)
- Lưu tối đa 10 mạng WiFi, tự động kết nối lại
- Giữ nút MODE 10 giây để reset về cài đặt gốc

### Web Dashboard
- **Giám sát thời gian thực:** Dashboard tự động làm mới chỉ số mới nhất từ các thiết bị
- **Biểu đồ xu hướng:** Lịch sử nhịp tim, SpO₂, nhiệt độ, độ ẩm bằng Chart.js, hỗ trợ chọn số mẫu (50/100/200/tất cả) và zoom/pan
- **Cảnh báo tự động:** Sinh cảnh báo khi vượt ngưỡng an toàn, hiển thị toast và chuông trên thanh điều hướng
- **Quản lý thiết bị / bệnh nhân:** Thêm, sửa, xóa thiết bị và gán tên bệnh nhân
- **Phân quyền người dùng:** Bác sĩ (toàn quyền) và Y tá (xem + cập nhật thông tin bệnh nhân)
- **Giao diện sáng/tối:** Chuyển đổi theme, lưu lựa chọn trên trình duyệt
- **Dữ liệu mẫu:** Nút tạo dữ liệu demo để thử nghiệm nhanh

---

## Ngưỡng cảnh báo

| Chỉ số   | Ngưỡng an toàn | Điều kiện cảnh báo   |
|----------|----------------|----------------------|
| Nhịp tim | 50–120 bpm     | < 50 hoặc > 120 bpm  |
| SpO₂     | ≥ 90%          | < 90%                |
| Nhiệt độ | ≤ 38.5°C       | > 38.5°C             |
| Độ ẩm    | 30–70%         | < 30% hoặc > 70%     |
| SOS      | —              | Thiết bị gửi tín hiệu SOS |

---

## Kiến trúc & công nghệ

| Thành phần      | Công nghệ                                          |
|-----------------|----------------------------------------------------|
| Vi điều khiển   | ESP32-C3 (Arduino IDE, C++)                        |
| Cảm biến        | MAX30102 (HR/SpO₂), HDC2022 (nhiệt độ/độ ẩm)      |
| Màn hình        | OLED SSD1306 128×64                                |
| Giao tiếp       | WiFi (HTTP POST), I2C                              |
| Backend         | PHP 7.3+ (không framework), lưu trữ JSON          |
| Frontend        | HTML5, CSS3, JavaScript thuần, Chart.js            |
| Xác thực        | PHP session, mật khẩu băm SHA-256                  |

---

## Cấu trúc thư mục

```
DATN/
├── 1_Hardware/
│   ├── DATN_Hardware.ino      # Firmware chính cho ESP32-C3
│   ├── config.h               # Cấu hình chân GPIO, ngưỡng, WiFi
│   ├── MAX30102.cpp/.h        # Driver cảm biến nhịp tim / SpO₂
│   ├── HDC2022.cpp/.h         # Driver cảm biến nhiệt độ / độ ẩm
│   └── SSD1306_128x64.cpp/.h  # Driver màn hình OLED
│
└── 2_Web_Dashboard/
    ├── index.php              # Dashboard tổng quan
    ├── devices.php            # Quản lý thiết bị / bệnh nhân
    ├── alerts.php             # Lịch sử cảnh báo
    ├── settings.php           # Cài đặt hiển thị
    ├── login.php              # Đăng nhập
    ├── logout.php             # Đăng xuất
    ├── api.php                # Các endpoint REST API (JSON)
    ├── lib/
    │   ├── auth.php           # Xác thực, phân quyền, quản lý phiên
    │   ├── storage.php        # Đọc/ghi dữ liệu JSON, sinh cảnh báo
    │   └── icons.php          # Hàm icon() và asset() (cache busting)
    ├── assets/
    │   ├── style.css          # Toàn bộ giao diện
    │   ├── theme.js           # Chuyển đổi sáng/tối
    │   ├── app.js             # Logic dashboard chính
    │   ├── devices.js         # Logic trang thiết bị
    │   ├── alerts.js          # Logic trang cảnh báo
    │   └── settings.js        # Logic trang cài đặt
    └── data/
        └── iot_store.json     # Kho dữ liệu (tự sinh khi chạy lần đầu)
```

---

## Cấu hình phần cứng (config.h)

| Chức năng    | Chân GPIO |
|--------------|-----------|
| I2C SDA      | GPIO 7    |
| I2C SCL      | GPIO 8    |
| Buzzer       | GPIO 3    |
| LED đỏ       | GPIO 4    |
| LED xanh     | GPIO 5    |
| Nút MODE     | GPIO 0    |
| Nút SOS      | GPIO 6    |

---

## Cài đặt

### 1. Phần cứng (ESP32-C3)

**Yêu cầu:**
- Arduino IDE 2.x với board package ESP32 (Espressif)
- Các thư viện: `ArduinoJson`, `Adafruit SSD1306`, `Adafruit GFX`

**Các bước:**
1. Mở `1_Hardware/DATN_Hardware.ino` trong Arduino IDE.
2. Chỉnh thông tin WiFi và địa chỉ server trong `config.h`:
   ```cpp
   #define WIFI_SSID     "TenMang"
   #define WIFI_PASSWORD "MatKhau"
   #define SERVER_URL    "http://<dia-chi-server>/api.php?action=post_data"
   ```
3. Chọn board **ESP32C3 Dev Module**, nạp firmware.
4. Thiết bị sẽ đo và gửi dữ liệu lên server mỗi 10 giây.

### 2. Web Dashboard

**Yêu cầu:**
- PHP 7.3 trở lên
- Web server hỗ trợ PHP (Apache, Nginx + PHP-FPM, hoặc PHP built-in server)
- Thư mục `data/` cần có quyền ghi

**Các bước:**
1. Sao chép toàn bộ thư mục `2_Web_Dashboard/` vào thư mục gốc của web server.
2. Đảm bảo thư mục `data/` tồn tại và có quyền ghi (tự tạo nếu chưa có):
   ```bash
   mkdir data && chmod 755 data
   ```
3. Chạy thử bằng PHP built-in server:
   ```bash
   cd 2_Web_Dashboard
   php -S localhost:8000
   ```
4. Mở trình duyệt tại `http://localhost:8000` — hệ thống tự chuyển sang trang đăng nhập.

---

## Tài khoản demo

| Vai trò | Tài khoản | Mật khẩu    |
|---------|-----------|-------------|
| Bác sĩ  | `bacsi`   | `Bacsi@123` |
| Y tá    | `yta`     | `Yta@123`   |

---

## API

Mọi yêu cầu gửi tới `api.php?action=<tên_hành_động>`, trả về JSON.

| Hành động        | Method | Quyền           | Mô tả                                       |
|------------------|--------|-----------------|---------------------------------------------|
| `health`         | GET    | Đăng nhập       | Kiểm tra trạng thái API và kho dữ liệu      |
| `devices`        | GET    | Bác sĩ, Y tá    | Danh sách thiết bị kèm chỉ số mới nhất      |
| `device`         | GET    | Bác sĩ, Y tá    | Chi tiết một thiết bị (cần param `deviceId`)|
| `post_data`      | POST   | Công khai        | Nhận dữ liệu cảm biến từ ESP32              |
| `upsert_patient` | POST   | Bác sĩ, Y tá    | Tạo/cập nhật tên bệnh nhân cho thiết bị     |
| `delete_device`  | POST   | Chỉ bác sĩ      | Xóa thiết bị                                |
| `seed_demo`      | POST   | Chỉ bác sĩ      | Tạo dữ liệu mẫu                             |

### Định dạng dữ liệu ESP32 gửi lên

`POST api.php?action=post_data` với body JSON:

```json
{
  "deviceId": "ESP32_001",
  "patientName": "Nguyen Van An",
  "heartRate": 78,
  "spo2": 97,
  "temperature": 36.8,
  "humidity": 55,
  "sos": false
}
```

Các trường bắt buộc: `deviceId`, `heartRate`, `spo2`, `temperature`. API cũng chấp nhận tên trường thay thế: `device_id`, `hr`, `oxygen`, `temp`.

---

## Lưu ý

- Dữ liệu lưu trong file JSON, giới hạn 500 mẫu lịch sử gần nhất mỗi thiết bị.
- Mật khẩu băm SHA-256 với salt cố định — phù hợp môi trường học tập/demo. Triển khai thực tế nên dùng `password_hash()` với salt ngẫu nhiên.
- Không có cơ sở dữ liệu — thiết kế đơn giản, phù hợp cho demo và học tập.

---

## Tác giả

**Mai Chí Thanh** — Đồ án tốt nghiệp (DATN)
