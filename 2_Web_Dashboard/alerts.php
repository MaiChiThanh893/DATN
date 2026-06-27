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
    <title>Cảnh báo | MCT IoT Monitoring</title>
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
        <a class="nav-i active" href="alerts.php"><span class="ic"><?php echo icon('bell'); ?></span> Cảnh báo</a>
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
                <h1>Trang cảnh báo</h1>
                <div class="sub">Toàn bộ cảnh báo hiện tại của hệ thống</div>
            </div>
            <div class="sp"></div>
            <button class="btn ghost" id="markAllReadBtn" type="button">Đánh dấu tất cả đã đọc</button>
            <button class="iconbtn" data-theme-toggle type="button" aria-pressed="false"><?php echo icon('theme'); ?> <span data-theme-label>Tối</span></button>
            <a class="iconbtn danger" href="logout.php"><?php echo icon('logout'); ?> <span class="lbl">Đăng xuất</span></a>
        </header>

        <div class="content">
            <section id="globalMessage"></section>

            <section class="stats">
                <div class="stat alarm"><div class="h"><span class="lab">Đang cảnh báo</span><span class="ic"><?php echo icon('warning'); ?></span></div><div class="num" id="statFlagged">0</div><div class="delta">thiết bị</div></div>
                <div class="stat"><div class="h"><span class="lab">Chưa đọc</span><span class="ic"><?php echo icon('bell'); ?></span></div><div class="num" id="statUnread">0</div><div class="delta">cảnh báo</div></div>
                <div class="stat"><div class="h"><span class="lab">Đã đọc</span><span class="ic"><?php echo icon('check'); ?></span></div><div class="num" id="statRead">0</div><div class="delta">cảnh báo</div></div>
            </section>

            <section class="card">
                <div class="card-h">
                    <h3>Cảnh báo hiện tại</h3>
                    <label class="search" style="min-width:200px"><?php echo icon('search'); ?> <input id="alertSearch" type="text" placeholder="Lọc theo bệnh nhân / thiết bị"></label>
                </div>
                <div class="feed" id="alertsList"><div class="feed-empty">Đang tải…</div></div>
            </section>
        </div>
    </div>
</div>

<script src="<?php echo asset('assets/theme.js'); ?>"></script>
<script src="<?php echo asset('assets/alerts.js'); ?>"></script>
</body>
</html>
