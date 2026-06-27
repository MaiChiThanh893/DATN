<?php

require_once __DIR__ . '/lib/auth.php';
require_once __DIR__ . '/lib/icons.php';

require_login();

$currentRole = htmlspecialchars(auth_current_role(), ENT_QUOTES, 'UTF-8');
$currentRoleLabel = htmlspecialchars(auth_current_role_label(), ENT_QUOTES, 'UTF-8');
?>
<!DOCTYPE html>
<html lang="vi" data-theme="light">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MCT IoT Monitoring — Dashboard</title>
    <script>
        (function () {
            try {
                var t = localStorage.getItem('iot_dashboard_theme');
                if (t !== 'light' && t !== 'dark') { t = 'light'; }
                document.documentElement.setAttribute('data-theme', t);
            } catch (e) {
                document.documentElement.setAttribute('data-theme', 'light');
            }
        })();
    </script>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&display=swap" rel="stylesheet">
    <link rel="stylesheet" href="<?php echo asset('assets/style.css'); ?>">
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body data-role="<?php echo $currentRole; ?>">
<div class="app">

    <nav class="rail">
        <div class="brand">
            <div class="logo"><?php echo icon('heart'); ?></div>
            <div><b>MCT IoT</b><small>Health Monitor</small></div>
        </div>
        <a class="nav-i active" href="#overview"><span class="ic"><?php echo icon('dashboard'); ?></span> Tổng quan</a>
        <a class="nav-i" href="devices.php"><span class="ic"><?php echo icon('monitor'); ?></span> Thiết bị</a>
        <a class="nav-i" href="alerts.php"><span class="ic"><?php echo icon('bell'); ?></span> Cảnh báo</a>
        <a class="nav-i" href="#history"><span class="ic"><?php echo icon('chart'); ?></span> Lịch sử</a>
        <div class="spacer"></div>
        <a class="nav-i" href="settings.php"><span class="ic"><?php echo icon('settings'); ?></span> Cài đặt</a>
        <div class="me">
            <div class="av"><?php echo strtoupper(substr($currentRoleLabel, 0, 2)); ?></div>
            <div><b><?php echo $currentRoleLabel; ?></b><small><?php echo $currentRole; ?></small></div>
        </div>
    </nav>

    <div class="main">
        <header class="topbar">
            <button class="iconbtn" id="sidebarToggle" data-nav-toggle type="button" aria-label="Menu"><?php echo icon('menu'); ?></button>
            <div>
                <h1>Tổng quan giám sát</h1>
                <div class="sub">Theo dõi sức khỏe bệnh nhân thời gian thực</div>
            </div>
            <div class="sp"></div>
            <div class="bell-wrap">
                <button class="iconbtn bell" id="alertBell" type="button" aria-label="Cảnh báo"><?php echo icon('bell'); ?><span class="bell-badge" id="alertBellCount" hidden>0</span></button>
                <div class="alert-panel" id="alertPanel" hidden></div>
            </div>
            <button class="iconbtn" data-theme-toggle type="button" aria-pressed="false"><?php echo icon('theme'); ?> <span data-theme-label>Tối</span></button>
            <a class="iconbtn danger" href="logout.php"><?php echo icon('logout'); ?> <span class="lbl">Đăng xuất</span></a>
        </header>

        <div class="content">
            <section id="globalMessage"></section>

            <section class="stats" id="overview">
                <div class="stat"><div class="h"><span class="lab">Thiết bị</span><span class="ic"><?php echo icon('monitor'); ?></span></div><div class="num" id="summaryTotalDevices">0</div><div class="delta">đang quản lý</div></div>
                <div class="stat alarm"><div class="h"><span class="lab">Đang cảnh báo</span><span class="ic"><?php echo icon('warning'); ?></span></div><div class="num" id="summaryAlertDevices">0</div><div class="delta">cần chú ý ngay</div></div>
                <div class="stat"><div class="h"><span class="lab">Nhịp tim TB</span><span class="ic"><?php echo icon('heart'); ?></span></div><div class="num" id="summaryAvgHeartRate">--</div><div class="delta">mẫu mới nhất</div></div>
                <div class="stat"><div class="h"><span class="lab">SpO₂ TB</span><span class="ic"><?php echo icon('drop'); ?></span></div><div class="num" id="summaryAvgSpo2">--</div><div class="delta">độ bão hòa oxy</div></div>
                <div class="stat"><div class="h"><span class="lab">Nhiệt độ TB</span><span class="ic"><?php echo icon('thermostat'); ?></span></div><div class="num" id="summaryAvgTemp">--</div><div class="delta">thân nhiệt</div></div>
                <div class="stat"><div class="h"><span class="lab">Độ ẩm TB</span><span class="ic"><?php echo icon('humidity'); ?></span></div><div class="num" id="summaryAvgHum">--</div><div class="delta">môi trường</div></div>
            </section>

            <section class="card" id="detail">
                <div class="detail-head">
                    <button class="devpicker-btn" id="devPickerBtn" type="button" title="Chọn thiết bị"><?php echo icon('monitor'); ?> <span id="devPickerLabel">Chọn thiết bị</span> <span class="caret"><?php echo icon('caret'); ?></span></button>
                    <div class="av"><?php echo icon('heart'); ?></div>
                    <div>
                        <h2 id="detailTitle">Chọn thiết bị để xem chi tiết</h2>
                        <div class="meta" id="detailSubtitle">Chọn một thiết bị trong danh sách để xem chỉ số, cảnh báo và biểu đồ.</div>
                    </div>
                    <span class="health-badge neutral" id="selectedHealthBadge">Chưa có dữ liệu</span>
                </div>

                <div id="sosBanner" class="sos-banner" hidden></div>

                <span id="infoDeviceIdHero" hidden></span>
                <div style="display:flex;gap:20px;flex-wrap:wrap;padding:0 18px 6px;font-size:12.5px;color:var(--muted)">
                    <span>Device: <b id="infoDeviceId" style="color:var(--text-2)">--</b></span>
                    <span>Bệnh nhân: <b id="infoPatientName" style="color:var(--text-2)">--</b></span>
                    <span>Số mẫu: <b id="infoHistoryCount" style="color:var(--text-2)">0</b></span>
                    <span>Tạo: <b id="infoCreatedAt" style="color:var(--text-2)">--</b></span>
                    <span>Cập nhật: <b id="updatedAtValue" style="color:var(--text-2)">--</b></span>
                </div>

                <div class="vitals">
                    <div class="vital" id="heartRateCard"><div class="lab"><span>Nhịp tim</span><span>bpm</span></div><div class="v" id="heartRateValue">--</div><div class="th">Ngưỡng an toàn 50–120</div></div>
                    <div class="vital" id="spo2Card"><div class="lab"><span>Nồng độ SpO₂</span><span>%</span></div><div class="v" id="spo2Value">--</div><div class="th">An toàn ≥ 90%</div></div>
                    <div class="vital" id="temperatureCard"><div class="lab"><span>Nhiệt độ</span><span>°C</span></div><div class="v" id="temperatureValue">--</div><div class="th">Cảnh báo &gt; 38.5°C</div></div>
                    <div class="vital" id="humidityCard"><div class="lab"><span>Độ ẩm</span><span>%</span></div><div class="v" id="humidityValue">--</div><div class="th">Thoải mái 30–70%</div></div>
                </div>

                <div id="currentStatusPanel" class="current-status empty-block">Chọn một thiết bị để xem mức độ ổn định và nhận xét nhanh.</div>

                <div class="chart-bar">
                    <div class="chart-tabs">
                        <button class="ctab active" data-view="all" type="button">Tổng hợp</button>
                        <button class="ctab" data-view="hr" type="button">Nhịp tim</button>
                        <button class="ctab" data-view="spo2" type="button">SpO₂</button>
                        <button class="ctab" data-view="temp" type="button">Nhiệt độ</button>
                        <button class="ctab" data-view="hum" type="button">Độ ẩm</button>
                    </div>
                    <div class="chart-range">
                        <span class="rlabel">Số mẫu</span>
                        <button class="rtab" data-range="50" type="button">50</button>
                        <button class="rtab" data-range="100" type="button">100</button>
                        <button class="rtab active" data-range="200" type="button">200</button>
                        <button class="rtab" data-range="all" type="button">Tất cả</button>
                        <div class="chart-zoom">
                            <button class="ztab" id="zoomOutBtn" type="button" title="Thu nhỏ — xem khoảng thời gian rộng hơn, tổng quát hơn"><?php echo icon('zoomout'); ?></button>
                            <span class="zlabel" id="zoomLabel">200</span>
                            <button class="ztab" id="zoomInBtn" type="button" title="Phóng to — xem ít mốc hơn, chi tiết hơn"><?php echo icon('zoomin'); ?></button>
                            <button class="ztab" id="zoomResetBtn" type="button" title="Đặt lại mức zoom"><?php echo icon('refresh'); ?></button>
                        </div>
                    </div>
                </div>
                <div class="chart-main"><div class="chart-wrap big"><canvas id="mainChart"></canvas></div></div>

                <div class="card-h" id="history" style="border-top:1px solid var(--border)"><h3>Lịch sử đo gần nhất</h3><span class="meta">10 mẫu</span></div>
                <div class="tablewrap">
                    <table>
                        <thead><tr><th>Thời gian</th><th>Nhịp tim</th><th>SpO₂</th><th>Nhiệt độ</th><th>Độ ẩm</th></tr></thead>
                        <tbody id="historyTableBody"><tr><td colspan="5" class="empty-row">Chưa có dữ liệu để hiển thị.</td></tr></tbody>
                    </table>
                </div>
            </section>
        </div>
    </div>
</div>

<div class="drawer-overlay" id="devDrawerOverlay"></div>
<aside class="drawer" id="devDrawer">
    <div class="drawer-h">
        <h3>Thiết bị <span class="muted" style="font-weight:600">(<span id="deviceCountBadge">0</span>)</span></h3>
        <button class="iconbtn" id="devDrawerClose" type="button" aria-label="Đóng"><?php echo icon('close'); ?></button>
    </div>
    <div class="devsearch-wrap"><input id="deviceSearchInput" class="devsearch" type="text" placeholder="Tìm thiết bị / bệnh nhân"></div>
    <div id="deviceList" class="devlist"><div class="feed-empty">Đang tải…</div></div>
    <a class="btn ghost block manage-link" href="devices.php"><?php echo icon('settings'); ?> Quản lý thiết bị</a>
</aside>

<div class="toast-host" id="toastHost"></div>

<script src="<?php echo asset('assets/theme.js'); ?>"></script>
<script src="<?php echo asset('assets/app.js'); ?>"></script>
</body>
</html>
