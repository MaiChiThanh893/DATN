<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>IoT Health Monitoring Dashboard</title>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&display=swap" rel="stylesheet">
    <link rel="stylesheet" href="assets/style.css">
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
    <div class="dashboard-shell">
        <aside class="sidebar">
            <div class="brand-panel">
                <div class="brand-icon">🩺</div>
                <div class="brand-header-copy">
                    <p class="section-kicker">dashboard</p>
                    <h1>IoT Health Monitoring</h1>
                </div>
                <p class="brand-text">Hệ thống theo dõi sức khỏe bệnh nhân thời gian thực dựa trên nền tảng IoT (ESP32).</p>
            </div>

            <nav class="side-nav">
                <p class="nav-title">Bảng điều khiển</p>
                <a href="#overview" class="nav-link active">Tổng quan</a>
                <a href="#devices" class="nav-link">Thiết bị</a>
                <a href="#charts" class="nav-link">Biểu đồ</a>
                <a href="#history" class="nav-link">Lịch sử</a>
            </nav>

            <section class="sidebar-card">
                <div class="card-head compact-head">
                    <div>
                        <p class="section-kicker">Hành động nhanh</p>
                        <h2>Quản lý thiết bị</h2>
                    </div>
                </div>
                <div class="button-stack">
                    <button id="refreshBtn" class="btn btn-primary" type="button">Làm mới dữ liệu</button>
                    <button id="seedBtn" class="btn btn-secondary" type="button">Tạo dữ liệu mẫu</button>
                </div>
                <form id="deviceForm" class="device-form">
                    <label>
                        Device ID
                        <input type="text" id="deviceIdInput" placeholder="VD: ESP32_001" required>
                    </label>
                    <label>
                        Tên bệnh nhân
                        <input type="text" id="patientNameInput" placeholder="Nhập tên bệnh nhân">
                    </label>
                    <div class="form-row">
                        <button type="submit" class="btn btn-primary">Lưu thông tin</button>
                        <button type="button" id="deleteBtn" class="btn btn-danger">Xóa thiết bị</button>
                    </div>
                </form>
                <p class="helper-text">Bạn có thể tạo device trước, sau đó để ESP32 đẩy dữ liệu lên hệ thống.</p>
            </section>

            <section class="sidebar-card device-panel" id="devices">
                <div class="card-head compact-head">
                    <div>
                        <p class="section-kicker">Theo dõi trực tiếp</p>
                        <h2>Danh sách thiết bị</h2>
                    </div>
                    <span id="deviceCountBadge" class="count-chip">0</span>
                </div>
                <div class="search-wrap">
                    <input type="text" id="deviceSearchInput" placeholder="Tìm theo device ID hoặc bệnh nhân">
                </div>
                <div id="deviceList" class="device-list empty-state">Chưa có thiết bị nào</div>
            </section>
        </aside>

        <main class="main-content">
            <header class="topbar">
                <div>
                    <p class="breadcrumb">TRANG CHỦ</p>
                    <h2 class="page-title">Dashboard giám sát sức khỏe</h2>
                </div>
                <div class="live-indicator">
                    <span class="pulse-dot"></span>
                    <span>Polling mỗi 3 giây</span>
                </div>
            </header>

            <section class="hero-panel panel" id="overview">
                <div class="hero-copy">
                    <span class="status-pill online">Hệ thống trực tuyến</span>
                    <h3 id="detailTitle">Chọn thiết bị để xem chi tiết</h3>
                    <p id="detailSubtitle">Chọn 1 thiết bị trong danh sách bên trái để xem chỉ số sức khỏe, cảnh báo và biểu đồ lịch sử</p>
                    <div class="hero-tags">
                        <span class="hero-tag">ESP32</span>
                        <span class="hero-tag">Nhịp tim</span>
                        <span class="hero-tag">Nồng độ SpO₂</span>
                        <span class="hero-tag">Nhiệt độ cơ thể</span>
                    </div>
                </div>
                <div class="hero-visual">
                    <div class="hero-ring"></div>
                    <div class="hero-card-mini">
                        <span>Thiết bị đang chọn</span>
                        <strong id="infoDeviceIdHero">--</strong>
                    </div>
                    <div class="hero-card-mini alt">
                        <span>Trạng thái</span>
                        <strong id="selectedHealthBadge" class="health-badge neutral">Chưa có dữ liệu</strong>
                    </div>
                </div>
            </section>

            <section class="summary-grid">
                <article class="summary-card panel">
                    <p class="summary-label">Thiết bị đang quản lý</p>
                    <strong id="summaryTotalDevices" class="summary-value">0</strong>
                    <span class="summary-note">Tổng số thiết bị có trong hệ thống</span>
                </article>
                <article class="summary-card panel alert-glow">
                    <p class="summary-label">Thiết bị có cảnh báo</p>
                    <strong id="summaryAlertDevices" class="summary-value">0</strong>
                    <span class="summary-note">Cần được chú ý ngay</span>
                </article>
                <article class="summary-card panel blue-glow">
                    <p class="summary-label">Nhịp tim trung bình</p>
                    <strong id="summaryAvgHeartRate" class="summary-value">--</strong>
                    <span class="summary-note">Tính theo mẫu mới nhất của từng thiết bị</span>
                </article>
                <article class="summary-card panel green-glow">
                    <p class="summary-label">Nồng độ SpO₂ trung bình</p>
                    <strong id="summaryAvgSpo2" class="summary-value">--</strong>
                    <span class="summary-note">Đánh giá nhanh độ bão hòa oxy trong máu</span>
                </article>
            </section>

            <section id="globalMessage"></section>
            <section id="alertsContainer"></section>

            <section class="metrics-grid">
                <article id="heartRateCard" class="metric-card panel metric-blue">
                    <div class="metric-top">
                        <span class="metric-title">Nhịp tim</span>
                        <span class="metric-unit">bpm</span>
                    </div>
                    <p id="heartRateValue" class="metric-value">--</p>
                    <p class="metric-note">Cảnh báo khi &lt; 55 hoặc &gt; 120</p>
                </article>
                <article id="spo2Card" class="metric-card panel metric-green">
                    <div class="metric-top">
                        <span class="metric-title">Nồng độ SpO₂</span>
                        <span class="metric-unit">%</span>
                    </div>
                    <p id="spo2Value" class="metric-value">--</p>
                    <p class="metric-note">Cảnh báo khi thấp hơn 90%</p>
                </article>
                <article id="temperatureCard" class="metric-card panel metric-orange">
                    <div class="metric-top">
                        <span class="metric-title">Nhiệt độ cơ thể</span>
                        <span class="metric-unit">°C</span>
                    </div>
                    <p id="temperatureValue" class="metric-value">--</p>
                    <p class="metric-note">Cảnh báo khi cao hơn 38.5°C</p>
                </article>
                <article class="metric-card panel metric-neutral">
                    <div class="metric-top">
                        <span class="metric-title">Cập nhật gần nhất</span>
                        <span class="metric-unit">time</span>
                    </div>
                    <p id="updatedAtValue" class="metric-time">--</p>
                    <p class="metric-note">Thời gian server nhận mẫu mới nhất</p>
                </article>
            </section>

            <section class="content-grid">
                <article class="panel info-panel">
                    <div class="card-head">
                        <div>
                            <p class="section-kicker">Chi tiết theo dõi</p>
                            <h3>Hồ sơ thiết bị</h3>
                        </div>
                    </div>
                    <div class="info-grid">
                        <div class="info-item">
                            <span>Device ID</span>
                            <strong id="infoDeviceId">--</strong>
                        </div>
                        <div class="info-item">
                            <span>Bệnh nhân</span>
                            <strong id="infoPatientName">--</strong>
                        </div>
                        <div class="info-item">
                            <span>Số mẫu đã lưu</span>
                            <strong id="infoHistoryCount">0</strong>
                        </div>
                        <div class="info-item">
                            <span>Thời gian tạo</span>
                            <strong id="infoCreatedAt">--</strong>
                        </div>
                    </div>
                </article>

                <article class="panel status-panel-wrap">
                    <div class="card-head">
                        <div>
                            <p class="section-kicker">Đánh giá hiện tại</p>
                            <h3>Tình trạng thiết bị</h3>
                        </div>
                    </div>
                    <div id="currentStatusPanel" class="current-status empty-block">
                        Chọn 1 thiết bị để xem mức độ ổn định và nhận xét nhanh về dữ liệu hiện tại.
                    </div>
                </article>
            </section>

            <section class="charts-section" id="charts">
                <article class="panel chart-card">
                    <div class="card-head">
                        <div>
                            <p class="section-kicker">Xu hướng dữ liệu</p>
                            <h3>Nhịp tim theo thời gian</h3>
                        </div>
                    </div>
                    <div class="chart-wrap"><canvas id="heartRateChart"></canvas></div>
                </article>
                <article class="panel chart-card">
                    <div class="card-head">
                        <div>
                            <p class="section-kicker">Xu hướng dữ liệu</p>
                            <h3>Nồng độ SpO₂ theo thời gian</h3>
                        </div>
                    </div>
                    <div class="chart-wrap"><canvas id="spo2Chart"></canvas></div>
                </article>
                <article class="panel chart-card chart-wide">
                    <div class="card-head">
                        <div>
                            <p class="section-kicker">Xu hướng dữ liệu</p>
                            <h3>Nhiệt độ cơ thể theo thời gian</h3>
                        </div>
                    </div>
                    <div class="chart-wrap"><canvas id="temperatureChart"></canvas></div>
                </article>
            </section>

            <section class="panel history-panel" id="history">
                <div class="card-head">
                    <div>
                        <p class="section-kicker">Lịch sử gần nhất</p>
                        <h3>Bảng dữ liệu đo</h3>
                    </div>
                </div>
                <div class="table-wrap">
                    <table>
                        <thead>
                            <tr>
                                <th>Thời gian</th>
                                <th>Nhịp tim</th>
                                <th>Nồng độ SpO₂</th>
                                <th>Nhiệt độ cơ thể</th>
                                <th>Trạng thái</th>
                            </tr>
                        </thead>
                        <tbody id="historyTableBody">
                            <tr>
                                <td colspan="5" class="empty-row">Chưa có dữ liệu để hiển thị.</td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </section>
        </main>
    </div>

    <script src="assets/app.js"></script>
</body>
</html>
