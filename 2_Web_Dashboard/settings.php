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
    <title>Cài đặt | MCT IoT Monitoring</title>
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
        <a class="nav-i" href="devices.php"><span class="ic"><?php echo icon('monitor'); ?></span> Thiết bị</a>
        <a class="nav-i" href="alerts.php"><span class="ic"><?php echo icon('bell'); ?></span> Cảnh báo</a>
        <a class="nav-i" href="index.php#detail"><span class="ic"><?php echo icon('chart'); ?></span> Lịch sử</a>
        <div class="spacer"></div>
        <a class="nav-i active" href="settings.php"><span class="ic"><?php echo icon('settings'); ?></span> Cài đặt</a>
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
                <h1>Cài đặt</h1>
                <div class="sub">Tùy chỉnh trải nghiệm dashboard</div>
            </div>
            <div class="sp"></div>
            <button class="iconbtn" data-theme-toggle type="button" aria-pressed="false"><?php echo icon('theme'); ?> <span data-theme-label>Tối</span></button>
            <a class="iconbtn danger" href="logout.php"><?php echo icon('logout'); ?> <span class="lbl">Đăng xuất</span></a>
        </header>

        <div class="content">
            <section id="globalMessage"></section>

            <section class="card" style="max-width:680px">
                <div class="card-h"><h3>Thông báo & hiển thị</h3></div>

                <div class="setting-row">
                    <div class="si"><b>Thông báo toast cảnh báo</b><small>Hiện thông báo nổi ở góc khi có cảnh báo mới</small></div>
                    <label class="switch"><input type="checkbox" id="toggleToast"><span class="slider"></span></label>
                </div>

                <div class="setting-row">
                    <div class="si"><b>Số mẫu biểu đồ mặc định</b><small>Số điểm dữ liệu hiển thị trên biểu đồ khi mở dashboard</small></div>
                    <div class="setting-control">
                        <select id="defaultRange">
                            <option value="50">50 mẫu</option>
                            <option value="100">100 mẫu</option>
                            <option value="200">200 mẫu</option>
                            <option value="all">Tất cả</option>
                        </select>
                    </div>
                </div>

                <div class="setting-row">
                    <div class="si"><b>Trạng thái đã đọc cảnh báo</b><small>Xóa toàn bộ dấu "đã đọc" đã lưu trên trình duyệt này</small></div>
                    <button class="btn ghost" id="clearRead" type="button">Xóa trạng thái đã đọc</button>
                </div>
            </section>
        </div>
    </div>
</div>

<script src="<?php echo asset('assets/theme.js'); ?>"></script>
<script src="<?php echo asset('assets/settings.js'); ?>"></script>
</body>
</html>
