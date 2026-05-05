# 🩺 Hệ Thống Giám Sát Sức Khỏe IoT (ESP32)

Ứng dụng web giám sát sức khỏe theo thời gian thực, được xây dựng cho đồ án tốt nghiệp.  
Hệ thống thu thập dữ liệu sinh hiệu từ thiết bị IoT (ESP32), xử lý trên máy chủ PHP, lưu trữ dưới dạng JSON và hiển thị qua dashboard trực quan cho nhân viên y tế.

---

## 📌 1) Tổng Quan Hệ Thống IoT

Hệ thống hướng đến bài toán theo dõi sức khỏe từ xa trong bệnh viện, phòng khám hoặc chăm sóc tại nhà.  
Dữ liệu đo từ thiết bị sẽ được gửi về API trung tâm, sau đó hiển thị trạng thái hiện tại, cảnh báo bất thường và lịch sử theo từng thiết bị/bệnh nhân.

### 🎯 Chỉ số theo dõi chính

- Nhịp tim (`Heart Rate`, bpm)
- Nồng độ oxy máu (`SpO2`, %)
- Nhiệt độ cơ thể (`Temperature`, °C)

### 👥 Đối tượng sử dụng

- **Bác sĩ**: toàn quyền theo dõi và thao tác dữ liệu
- **Y tá**: xem dữ liệu và cập nhật thông tin bệnh nhân theo quyền được cấp
- **Thiết bị IoT**: gửi dữ liệu đo qua endpoint công khai dành riêng cho ingest

---

## 🔄 2) Sơ Đồ Hoạt Động (Mô Tả Bằng Text)

```text
[ESP32 Device]
   -> Gửi dữ liệu đo (heartRate, spo2, temperature)
   -> HTTP POST /htdocs/api.php?action=post_data

[API PHP]
   -> Kiểm tra dữ liệu đầu vào
   -> Chuẩn hóa deviceId và timestamp
   -> Lưu vào htdocs/data/iot_store.json
   -> Tính cảnh báo theo ngưỡng sức khỏe

[Dashboard Web]
   -> app.js gọi GET /api.php?action=devices mỗi 3 giây
   -> Hiển thị danh sách thiết bị + trạng thái cảnh báo
   -> Khi chọn thiết bị: gọi GET /api.php?action=device&deviceId=...
   -> Vẽ biểu đồ lịch sử + bảng dữ liệu + cảnh báo chi tiết

[Người dùng y tế]
   -> Đăng nhập qua login.php
   -> Thao tác theo vai trò (doctor / nurse)
```

---

## 🧰 3) Công Nghệ Sử Dụng

### ⚙️ Backend

- **PHP**: xử lý API, nghiệp vụ, xác thực, phân quyền
- **JSON File Storage**: lưu dữ liệu tại `htdocs/data/iot_store.json`
- **Apache-compatible**: chạy tốt với XAMPP/Laragon/WAMP

### 🎨 Frontend

- **HTML5**
- **CSS3**
- **Vanilla JavaScript (ES6+)**
- **Chart.js (CDN)**: trực quan hóa dữ liệu theo thời gian

### 🔐 Bảo mật và phân quyền

- Đăng nhập theo **session**
- Phân quyền theo vai trò tại API (`doctor`, `nurse`)
- Endpoint ingest `post_data` cho thiết bị IoT được mở công khai theo thiết kế
- Cookie session cấu hình tăng cường an toàn (`HttpOnly`, `SameSite=Lax`, `Secure` khi có HTTPS)

---

## 🗂️ 4) Cấu Trúc Thư Mục

```text
DATN/
├── .htaccess
├── .override
├── README.md
└── htdocs/
    ├── index.php              # Trang dashboard (yêu cầu đăng nhập)
    ├── login.php              # Giao diện đăng nhập
    ├── logout.php             # Đăng xuất và hủy session
    ├── api.php                # API trung tâm cho dashboard và IoT
    ├── assets/
    │   ├── app.js             # Logic frontend: polling, render, chart
    │   └── style.css          # Giao diện dashboard và auth page
    ├── lib/
    │   ├── auth.php           # Xác thực + phân quyền + session helpers
    │   └── storage.php        # Lưu trữ JSON + cảnh báo + xử lý dữ liệu
    └── data/
        └── iot_store.json     # Kho dữ liệu runtime (tự tạo nếu chưa có)
```

---

## 🚀 5) Hướng Dẫn Chạy (XAMPP / localhost)

### 5.1 Yêu cầu môi trường

- PHP `7.4+` (khuyến nghị PHP `8.x`)
- Apache (XAMPP hoặc tương đương)

### 5.2 Cài đặt trên XAMPP

1. Sao chép project vào thư mục web root:
   - `C:/xampp/htdocs/DATN`
2. Mở **XAMPP Control Panel** và bật:
   - `Apache`
3. Đảm bảo thư mục dữ liệu có quyền ghi:
   - `htdocs/data/`
   - `htdocs/data/iot_store.json` (nếu đã tồn tại)
4. Truy cập ứng dụng:
   - `http://localhost/DATN/htdocs/login.php`

### 5.3 Tài khoản đăng nhập (theo mã nguồn hiện tại)

Được định nghĩa trong `htdocs/lib/auth.php`:

- Vai trò bác sĩ: username `bacsi`
- Vai trò y tá: username `yta`

> Mật khẩu được lưu dưới dạng hash SHA-256 có salt trong mã nguồn.

### 5.4 Kiểm tra nhanh endpoint ingest

```bash
curl -X POST "http://localhost/DATN/htdocs/api.php?action=post_data" ^
  -H "Content-Type: application/x-www-form-urlencoded" ^
  -d "deviceId=ESP32_001&heartRate=78&spo2=97&temperature=36.8"
```

---

## ✨ 6) Tính Năng Chính

- 🔐 **Đăng nhập/đăng xuất an toàn** bằng session
- 👩‍⚕️ **Phân quyền theo vai trò** (doctor/nurse) ở cả UI và API
- 📡 **Nhận dữ liệu IoT thời gian thực** qua `POST action=post_data`
- 📈 **Dashboard trực quan** cập nhật tự động mỗi 3 giây
- 🚨 **Cảnh báo chỉ số bất thường** theo ngưỡng lâm sàng
- 🧾 **Theo dõi chi tiết từng thiết bị** (latest + history + alert)
- 🔎 **Tìm kiếm thiết bị/bệnh nhân** nhanh trên giao diện
- 🧪 **Sinh dữ liệu mẫu** để demo và kiểm thử (`POST action=seed_demo`)
- 🗑️ **Xóa thiết bị và lịch sử** theo quyền (`POST action=delete_device`)

---

## 📊 7) Ngưỡng Cảnh Báo Hiện Tại

Được cài đặt tại `htdocs/lib/storage.php`:

- Nhịp tim bất thường: `< 50` hoặc `> 120` bpm
- SpO2 thấp: `< 90%`
- Nhiệt độ cao: `> 38.5°C`

---

## 🧪 8) Gợi Ý Kịch Bản Trình Bày Với Hội Đồng

1. Đăng nhập với vai trò bác sĩ  
2. Tạo dữ liệu mẫu (`seed_demo`)  
3. Chọn một thiết bị có cảnh báo để trình diễn phân tích  
4. Gửi thêm dữ liệu mới từ `curl` để chứng minh tính thời gian thực  
5. Đăng nhập vai trò y tá để chứng minh cơ chế phân quyền  

---

## 🔭 9) Hướng Phát Triển

- Chuyển từ JSON sang MySQL/PostgreSQL để tăng khả năng mở rộng
- Bổ sung xác thực thiết bị (token/JWT riêng cho firmware)
- Tích hợp kênh cảnh báo (email/SMS/Telegram)
- Bổ sung kiểm thử tự động (unit/integration) và pipeline CI/CD

---

## 📄 10) Giấy Phép

Hiện tại repository chưa kèm tệp giấy phép.  
Nếu phát hành công khai, nên bổ sung `LICENSE` (MIT hoặc Apache-2.0).
