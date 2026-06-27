<?php

require_once __DIR__ . '/lib/auth.php';
require_once __DIR__ . '/lib/icons.php';

$next = 'index.php';
if (isset($_GET['next'])) {
    $next = $_GET['next'];
} elseif (isset($_POST['next'])) {
    $next = $_POST['next'];
}
$next = auth_sanitize_redirect($next);
$error = '';

if (auth_is_logged_in()) {
    header('Location: ' . $next);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $username = isset($_POST['username']) ? (string) $_POST['username'] : '';
    $password = isset($_POST['password']) ? (string) $_POST['password'] : '';

    $user = auth_user_for_credentials($username, $password);
    if ($user !== null) {
        auth_login($user);
        header('Location: ' . $next);
        exit;
    }

    $error = 'Sai tài khoản hoặc mật khẩu.';
}

function e($value)
{
    return htmlspecialchars((string) $value, ENT_QUOTES, 'UTF-8');
}
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
    <title>Đăng nhập | MCT IoT Monitoring</title>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&display=swap" rel="stylesheet">
    <link rel="stylesheet" href="<?php echo asset('assets/style.css'); ?>">
</head>
<body>
<div class="auth">
    <button class="iconbtn auth-top" data-theme-toggle type="button" aria-pressed="false"><?php echo icon('theme'); ?> <span data-theme-label>Tối</span></button>

    <section class="auth-side">
        <div class="brand">
            <div class="logo"><?php echo icon('heart'); ?></div>
            <div><b style="font-size:15px">MCT IoT Monitoring</b><br><small style="opacity:.85;font-size:11px;letter-spacing:1.5px;text-transform:uppercase">Health Monitor</small></div>
        </div>
        <div>
            <h1>Giám sát sức khỏe<br>thời gian thực</h1>
            <p class="lead">Theo dõi nhịp tim, SpO₂ và nhiệt độ từ thiết bị ESP32. Cảnh báo tức thì khi chỉ số vượt ngưỡng an toàn.</p>
        </div>
        <div style="display:flex;flex-direction:column;gap:14px">
            <div class="feat"><span class="fi"><?php echo icon('heart'); ?></span> Cập nhật mỗi 3 giây, biểu đồ xu hướng</div>
            <div class="feat"><span class="fi"><?php echo icon('warning'); ?></span> Cảnh báo theo mức độ: ổn định · chú ý · nguy hiểm</div>
            <div class="feat"><span class="fi"><?php echo icon('lock'); ?></span> Phân quyền bác sĩ / y tá</div>
        </div>
    </section>

    <section class="auth-main">
        <form class="login-card" method="post" action="login.php" autocomplete="on">
            <input type="hidden" name="next" value="<?php echo e($next); ?>">
            <h2>Đăng nhập</h2>
            <p class="sub">Truy cập hệ thống giám sát sức khỏe</p>

            <?php if ($error !== ''): ?>
                <div class="flash-message error"><?php echo e($error); ?></div>
            <?php endif; ?>

            <div class="field">
                <label>Tài khoản</label>
                <input type="text" name="username" value="<?php echo e(isset($_POST['username']) ? (string) $_POST['username'] : ''); ?>" placeholder="Nhập tên đăng nhập" required autofocus>
            </div>
            <div class="field">
                <label>Mật khẩu</label>
                <input type="password" name="password" placeholder="••••••••" required>
            </div>
            <button class="btn" type="submit">Đăng nhập</button>
            <div class="note">Demo: <b>bacsi</b> / <b>Bacsi@123</b> · <b>yta</b> / <b>Yta@123</b></div>
        </form>
    </section>
</div>
<script src="<?php echo asset('assets/theme.js'); ?>"></script>
</body>
</html>
