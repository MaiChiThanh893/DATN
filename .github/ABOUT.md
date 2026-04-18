# Nội dung gợi ý cho mục **About** trên GitHub

Repo: [Maichithank8899/-ATN](https://github.com/Maichithank8899/-ATN)

## Cách 1: Chỉnh trên github.com (nhanh nhất)

1. Mở repo → bên phải phần **About** → biểu tượng bánh răng **⚙️** (Edit repository details).
2. Dán nội dung bên dưới vào **Description**, **Website**, **Topics** → **Save changes**.

### Description (dán nguyên dòng)

```
ĐATN — Dashboard giám sát sức khỏe IoT (ESP32): API PHP + JSON, giao diện web theo dõi nhịp tim, SpO₂, nhiệt độ cơ thể; biểu đồ Chart.js, cảnh báo ngưỡng, lịch sử đo, polling thời gian thực.
```

### Website (tùy chọn)

- Để trống nếu chưa có bản deploy công khai, **hoặc**
- Dán URL demo thật (InfinityFree, VPS, v.v.), **hoặc**
- Tạm thời trỏ về README: `https://github.com/Maichithank8899/-ATN#readme`

### Topics (gõ từng tag hoặc dán — GitHub tối đa 20 topic)

Gợi ý (copy từng từ vào ô Topics):

`iot` · `esp32` · `health-monitoring` · `php` · `dashboard` · `chart-js` · `javascript` · `rest-api` · `json-api` · `healthcare` · `vital-signs` · `spo2` · `heart-rate` · `temperature-monitoring` · `student-project` · `vietnam`

---

## Cách 2: GitHub CLI — script có sẵn trong repo

Trong PowerShell, tại thư mục gốc project:

```powershell
gh auth login
.\scripts\update-github-about.ps1
```

Script đọc mô tả tiếng Việt từ `scripts/github-about-description.txt`, cập nhật **description**, **website** và **topics** cho repo `Maichithank8899/-ATN`.

**Lưu ý:** Lệnh `git` thuần **không** thể sửa mục About trên GitHub; cần **`gh`** (GitHub CLI) hoặc chỉnh tay trên web.

---

## Cách 3: GitHub CLI — một lệnh dài (sau khi đăng nhập)

Sau `gh auth login`, có thể chạy trực tiếp (chỉnh `homepage` nếu bạn có URL demo):

```powershell
gh repo edit Maichithank8899/-ATN `
  --description "ĐATN — Dashboard giám sát sức khỏe IoT (ESP32): API PHP + JSON, giao diện web theo dõi nhịp tim, SpO₂, nhiệt độ cơ thể; biểu đồ Chart.js, cảnh báo ngưỡng, lịch sử đo, polling thời gian thực." `
  --homepage "https://github.com/Maichithank8899/-ATN#readme" `
  --add-topic iot --add-topic esp32 --add-topic health-monitoring --add-topic php `
  --add-topic dashboard --add-topic chart-js --add-topic javascript --add-topic rest-api `
  --add-topic json-api --add-topic healthcare --add-topic vital-signs --add-topic spo2 `
  --add-topic heart-rate --add-topic temperature-monitoring --add-topic student-project --add-topic vietnam
```

Nếu tài khoản GitHub đang bị **flagged** và không đăng nhập OAuth được, dùng **Cách 1** trên trình duyệt (đôi khi vẫn chỉnh được About trong khi app bên thứ ba bị chặn), hoặc liên hệ GitHub Support để gỡ hạn chế.
