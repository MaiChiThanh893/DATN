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
    <title>Quản lý thiết bị | MCT IoT Monitoring</title>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&display=swap" rel="stylesheet">
    <link rel="stylesheet" href="<?php echo asset('assets/style.css'); ?>">
</head>
<body data-role="<?php echo $currentRole; ?>">
<div class="app">

    <nav class="rail">
        <div class="brand">
            <div class="logo"><?php echo icon('heart'); ?></div>
            <div><b>MCT IoT</b><small>Health Monitor</small></div>
        </div>
        <a class="nav-i" href="index.php"><span class="ic"><?php echo icon('dashboard'); ?></span> Tổng quan</a>
        <a class="nav-i active" href="devices.php"><span class="ic"><?php echo icon('monitor'); ?></span> Thiết bị</a>
        <a class="nav-i" href="alerts.php"><span class="ic"><?php echo icon('bell'); ?></span> Cảnh báo</a>
        <a class="nav-i" href="index.php#detail"><span class="ic"><?php echo icon('chart'); ?></span> Lịch sử</a>
        <div class="spacer"></div>
        <a class="nav-i" href="settings.php"><span class="ic"><?php echo icon('settings'); ?></span> Cài đặt</a>
        <div class="me">
            <div class="av"><?php echo strtoupper(substr($currentRoleLabel, 0, 2)); ?></div>
            <div><b><?php echo $currentRoleLabel; ?></b><small><?php echo $currentRole; ?></small></div>
        </div>
    </nav>

    <div class="main">
        <header class="topbar">
            <button class="iconbtn only-mobile" data-nav-toggle type="button" aria-label="Menu"><?php echo icon('menu'); ?></button>
            <a class="iconbtn hide-mobile" href="index.php"><?php echo icon('back'); ?> Dashboard</a>
            <div>
                <h1>Quản lý thiết bị</h1>
                <div class="sub">Thêm, sửa, xóa thiết bị / bệnh nhân</div>
            </div>
            <div class="sp"></div>
            <button class="iconbtn" data-theme-toggle type="button" aria-pressed="false"><?php echo icon('theme'); ?> <span data-theme-label>Tối</span></button>
            <a class="iconbtn danger" href="logout.php"><?php echo icon('logout'); ?> <span class="lbl">Đăng xuất</span></a>
        </header>

        <div class="content">
            <section id="globalMessage"></section>

            <section class="row">
                <div class="card">
                    <div class="card-h"><h3>Tất cả thiết bị</h3><span class="meta"><span id="deviceCount">0</span> thiết bị</span></div>
                    <div style="padding:12px 18px 0;display:flex;gap:10px;flex-wrap:wrap">
                        <label class="search" style="flex:1;min-width:180px"><?php echo icon('search'); ?> <input id="deviceSearch" type="text" placeholder="Lọc theo bệnh nhân / thiết bị"></label>
                        <button class="btn ghost" id="refreshBtn" type="button"><?php echo icon('refresh'); ?> Làm mới</button>
                        <button class="btn ghost" id="seedBtn" type="button"><?php echo icon('add'); ?> Dữ liệu mẫu</button>
                    </div>
                    <div class="tablewrap" style="margin-top:12px">
                        <table>
                            <thead><tr><th>Bệnh nhân / thiết bị</th><th>Nhịp tim</th><th>SpO₂</th><th>Nhiệt độ</th><th>Độ ẩm</th><th>Trạng thái</th><th></th></tr></thead>
                            <tbody id="deviceTable"><tr><td colspan="7" class="empty-row">Đang tải…</td></tr></tbody>
                        </table>
                    </div>
                </div>

                <div class="card" style="align-self:start">
                    <div class="card-h"><h3 id="formTitle">Thêm thiết bị</h3></div>
                    <div class="actions">
                        <form id="deviceForm" autocomplete="off">
                            <div class="field"><label>Device ID</label><input id="deviceIdInput" type="text" placeholder="VD: ESP32_001"></div>
                            <div class="field"><label>Tên bệnh nhân</label><input id="patientNameInput" type="text" placeholder="Nhập tên bệnh nhân"></div>
                            <div class="form-row">
                                <button class="btn" type="submit">Lưu</button>
                                <button class="btn ghost" id="resetBtn" type="button">Làm mới form</button>
                            </div>
                        </form>
                        <p class="helper">Tạo device trước, ESP32 sẽ đẩy dữ liệu lên sau. Bấm "Sửa" ở một dòng để nạp vào form.</p>
                    </div>
                </div>
            </section>
        </div>
    </div>
</div>

<script src="<?php echo asset('assets/theme.js'); ?>"></script>
<script src="<?php echo asset('assets/devices.js'); ?>"></script>
</body>
</html>
