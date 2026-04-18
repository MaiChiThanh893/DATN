# IoT Health Monitoring Dashboard

Hệ thống **theo dõi sức khỏe bệnh nhân theo thời gian thực** qua web, nhận dữ liệu từ thiết bị IoT (ví dụ **ESP32**) đo nhịp tim, nồng độ SpO₂ và nhiệt độ cơ thể. Giao diện dashboard hiển thị tổng quan, cảnh báo, biểu đồ xu hướng và bảng lịch sử đo.

## Mục lục

- [GitHub About](#github-about) — chi tiết: [.github/ABOUT.md](.github/ABOUT.md)
- [Tính năng](#tính-năng)
- [Kiến trúc & công nghệ](#kiến-trúc--công-nghệ)
- [Cấu trúc thư mục](#cấu-trúc-thư-mục)
- [Yêu cầu môi trường](#yêu-cầu-môi-trường)
- [Cài đặt & chạy local](#cài-đặt--chạy-local)
- [Triển khai hosting](#triển-khai-hosting)
- [API cho thiết bị / tích hợp](#api-cho-thiết-bị--tích-hợp)
- [Lưu trữ dữ liệu](#lưu-trữ-dữ-liệu)
- [Ngưỡng cảnh báo](#ngưỡng-cảnh-báo)
- [Giao diện người dùng](#giao-diện-người-dùng)
- [Lưu ý bảo mật & sản xuất](#lưu-ý-bảo-mật--sản-xuất)
- [Khắc phục sự cố](#khắc-phục-sự-cố)

---

## GitHub About

GitHub **không** lấy mô tả, website hay topics từ file trong repo; lệnh **`git` thuần không thể** sửa ô About trên web — cần **GitHub CLI** (`gh`) hoặc chỉnh tay trên github.com.

- **Tự động (khuyến nghị):** sau `gh auth login`, chạy [`scripts/update-github-about.ps1`](scripts/update-github-about.ps1) (mô tả UTF-8 nằm trong `scripts/github-about-description.txt`).
- **Dán tay / lệnh dài:** xem [.github/ABOUT.md](.github/ABOUT.md).

## Tính năng

- **Danh sách thiết bị** theo thời gian cập nhật, tìm kiếm theo `deviceId` hoặc tên bệnh nhân.
- **Đăng ký / cập nhật bệnh nhân** gắn với `deviceId` (có thể tạo thiết bị trước khi ESP32 gửi dữ liệu).
- **Chỉ số theo thiết bị**: nhịp tim (bpm), SpO₂ (%), nhiệt độ cơ thể (°C), thời điểm mẫu mới nhất.
- **Cảnh báo** dựa trên mẫu mới nhất (hiển thị trên dashboard và trong phản hồi API).
- **Biểu đồ** xu hướng theo thời gian (Chart.js).
- **Bảng lịch sử** (10 bản ghi gần nhất trên UI).
- **Làm mới dữ liệu**, **tạo dữ liệu mẫu** (demo), **xóa thiết bị** (kèm toàn bộ lịch sử).
- **Polling** tự động mỗi **3 giây** để gần với theo dõi thời gian thực.

---

## Kiến trúc & công nghệ

| Thành phần | Vai trò |
|------------|---------|
| `htdocs/index.php` | Trang dashboard (HTML + liên kết CSS/JS). |
| `htdocs/api.php` | REST-style JSON API (`action` qua query string). |
| `htdocs/lib/storage.php` | Đọc/ghi store JSON, logic thiết bị, lịch sử, cảnh báo. |
| `htdocs/assets/app.js` | Fetch API, Chart.js, polling, render UI. |
| `htdocs/assets/style.css` | Giao diện. |
| `htdocs/data/iot_store.json` | File dữ liệu (tự tạo nếu chưa có). |

**Phía client:** JavaScript thuần (không build step), **Chart.js** tải từ CDN.

**Phía server:** PHP ≥ 7.x khuyến nghị (dùng `json_encode`/`json_decode`, `Throwable`).

---

## Cấu trúc thư mục

```
ĐATN/
├── .htaccess              # Cấu hình máy chủ (ví dụ hosting; có thể tùy chỉnh theo môi trường)
└── htdocs/
    ├── index.php          # Dashboard
    ├── api.php            # API JSON
    ├── assets/
    │   ├── app.js
    │   └── style.css
    ├── lib/
    │   └── storage.php    # Lưu trữ & nghiệp vụ
    └── data/
        └── iot_store.json # Dữ liệu runtime (git có thể ignore nếu cần)
```

Document root khi triển khai thường trỏ vào thư mục **`htdocs/`** (hoặc copy nội dung `htdocs` lên `public_html` tùy hosting).

---

## Yêu cầu môi trường

- **PHP** có bật extension JSON (mặc định có).
- Quyền **ghi file** cho thư mục `htdocs/data/` (ứng dụng tạo `iot_store.json` và cập nhật khi có dữ liệu).
- Trình duyệt hiện đại (fetch, ES2020+ như `replaceAll`).

---

## Cài đặt & chạy local

1. Clone hoặc copy toàn bộ project.
2. Chạy PHP built-in server với document root là `htdocs`:

```bash
cd htdocs
php -S localhost:8080
```

3. Mở trình duyệt: `http://localhost:8080/`

**Gợi ý:** Dùng nút **“Tạo dữ liệu mẫu”** trên sidebar để xem dashboard đầy đủ mà chưa cần ESP32.

---

## Triển khai hosting

- Upload nội dung **`htdocs/`** lên thư mục web của hosting (ví dụ `public_html`).
- Đảm bảo **`htdocs/data/`** tồn tại và **có quyền ghi** (chmod tùy môi trường).
- File `.htaccess` ở root repo có thể là mẫu từ nhà cung cấp; trên server riêng bạn có thể đặt rule rewrite/CORS trong `htdocs/.htaccess` nếu cần.

---

## API cho thiết bị / tích hợp

Base URL (ví dụ local): `http://localhost:8080/api.php`

Tất cả phản hồi JSON có dạng `{ "success": true|false, ... }`. Lỗi validation thường trả **422**, không tìm thấy **404**, lỗi server **500**.

### CORS & phương thức

- Cho phép `GET`, `POST`, `OPTIONS`.
- `Access-Control-Allow-Origin: *` (phù hợp demo; xem mục bảo mật).

### GET

| `action` | Mô tả | Tham số |
|----------|--------|---------|
| `devices` | Danh sách thiết bị (đã sort theo `updatedAt` giảm dần). | — |
| `device` | Chi tiết một thiết bị (latest, history, alerts). | `deviceId` (bắt buộc) |

**Ví dụ:**

```http
GET /api.php?action=devices
GET /api.php?action=device&deviceId=ESP32_001
```

### POST

Body JSON (khuyến nghị) hoặc `application/x-www-form-urlencoded` / form fields (API đọc `php://input` JSON trước, fallback `$_POST`).

| `action` | Mô tả | Trường chính |
|----------|--------|----------------|
| `post_data` | ESP32 / gateway đẩy một mẫu đo. | `deviceId`, `heartRate`, `spo2`, `temperature` (bắt buộc); `patientName`, `timestamp` (tùy chọn, ISO 8601) |
| `upsert_patient` | Tạo/cập nhật tên bệnh nhân cho thiết bị. | `deviceId`, `patientName` |
| `delete_device` | Xóa thiết bị và toàn bộ lịch sử. | `deviceId` |
| `seed_demo` | Tạo dữ liệu demo (3 thiết bị + lịch sử). | body rỗng `{}` được |

**Ví dụ gửi dữ liệu từ ESP32 (POST JSON):**

```http
POST /api.php?action=post_data
Content-Type: application/json

{
  "deviceId": "ESP32_001",
  "patientName": "Nguyễn Văn A",
  "heartRate": 78,
  "spo2": 98,
  "temperature": 36.6,
  "timestamp": "2026-04-18T10:00:00+00:00"
}
```

Phản hồi thành công gồm `entry` (bản ghi vừa lưu) và `device` (chi tiết sau khi cập nhật).

---

## Lưu trữ dữ liệu

- File **`htdocs/data/iot_store.json`** chứa object gốc với:
  - `devices`: map theo `deviceId` — mỗi thiết bị có `patientName`, `latest`, `history`, `createdAt`, `updatedAt`.
  - `meta`: metadata (thời điểm tạo/cập nhật store).
- Mỗi thiết bị giữ tối đa **120** mẫu trong `history` (cũ bị cắt bớt, cấu hình `IOT_HISTORY_LIMIT` trong `lib/storage.php`).

---

## Ngưỡng cảnh báo

Logic cảnh báo trên **mẫu mới nhất** (`latest`) trong `lib/storage.php`:

| Chỉ số | Điều kiện cảnh báo |
|--------|---------------------|
| Nhịp tim | &lt; **50** bpm hoặc &gt; **120** bpm |
| SpO₂ | &lt; **90** % |
| Nhiệt độ cơ thể | &gt; **38.5** °C |

Giao diện có thể hiển thị gợi ý ngưỡng khác một phần (ví dụ ghi chú trên thẻ metric); khi tích hợp lâu dài nên **đồng bộ** UI với rule server nếu cần nhất quán.

---

## Giao diện người dùng

- **Sidebar:** điều hướng mục (Tổng quan, Thiết bị, Biểu đồ, Lịch sử), form quản lý thiết bị, danh sách thiết bị.
- **Main:** thẻ tổng hợp, metric cards, hồ sơ thiết bị, trạng thái, ba biểu đồ đường, bảng lịch sử.
- **Polling 3s:** gọi lại danh sách và chi tiết thiết bị đang chọn.

Font: **Inter** (Google Fonts). Biểu đồ: **Chart.js** (CDN jsDelivr).

---

## Lưu ý bảo mật & sản xuất

- **Không có xác thực** trên API: bất kỳ ai biết URL đều có thể ghi/xóa dữ liệu. Trước khi đưa lên môi trường thật, nên thêm **API key**, **HTTPS**, giới hạn IP, hoặc **reverse proxy** có auth.
- `Access-Control-Allow-Origin: *` tiện cho demo nhưng **mở rộng** khả năng lạm dụng từ trang web khác; thu hẹp origin khi cần.
- Endpoint `seed_demo` chỉ nên **bật trên môi trường dev** hoặc bảo vệ bằng mật khẩu / tắt trên production.
- Dữ liệu y tế nhạy cảm: cân nhắc mã hóa, phân quyền, và tuân thủ quy định địa phương về bảo vệ dữ liệu cá nhân.

---

## Khắc phục sự cố

| Hiện tượng | Hướng xử lý |
|------------|-------------|
| API trả lỗi 500 / không lưu được | Kiểm tra quyền ghi `htdocs/data/`, dung lượng đĩa, log PHP. |
| Trang trắng / JSON lỗi | Bật hiển thị lỗi PHP trên dev; kiểm tra phiên bản PHP. |
| Chart không vẽ | Kiểm tra kết nối internet (Chart.js từ CDN) và console trình duyệt. |
| CORS từ domain khác | Server đã bật CORS rộng; nếu vẫn lỗi, kiểm tra preflight `OPTIONS` và proxy. |

---

## Giấy phép

Dự án đồ án / nội bộ — bổ sung file `LICENSE` nếu bạn cần phân phối công khai.

---

*Tài liệu này mô tả mã nguồn trong repo tại thời điểm tạo README. Khi chỉnh logic trong `storage.php` hoặc `api.php`, nên cập nhật lại các bảng API và ngưỡng cảnh báo cho khớp.*
