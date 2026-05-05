<?php

require_once __DIR__ . '/lib/auth.php';

$next = auth_sanitize_redirect($_GET['next'] ?? $_POST['next'] ?? 'index.php');
$error = '';

if (auth_is_logged_in()) {
    header('Location: ' . $next);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $username = (string) ($_POST['username'] ?? '');
    $password = (string) ($_POST['password'] ?? '');

    $user = auth_user_for_credentials($username, $password);
    if ($user !== null) {
        auth_login($user);
        header('Location: ' . $next);
        exit;
    }

    $error = 'Sai tài khoản hoặc mật khẩu.';
}

function e(string $value): string
{
    return htmlspecialchars($value, ENT_QUOTES, 'UTF-8');
}
?>
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Đăng nhập | IoT Health Monitoring</title>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Manrope:wght@400;500;600;700;800&display=swap" rel="stylesheet">
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&display=swap" rel="stylesheet">
    <link rel="stylesheet" href="assets/style.css">
</head>
<body class="auth-page">
    <main class="auth-shell">
        <header class="auth-topbar">
            <div class="auth-logo">
                <span class="auth-logo-mark" aria-hidden="true">
                    <svg viewBox="0 0 20 20" focusable="false" aria-hidden="true">
                        <path d="M3 13.5V11l3.5-3.5 2.5 2.5L14 5l3 3" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/>
                        <path d="M3 16h14" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
                    </svg>
                </span>
                <span>IoT Health Monitoring</span>
            </div>

            <nav class="auth-nav" aria-label="Main">
                <a href="#">Tính năng</a>
                <a href="#">Hỗ trợ</a>
                <a href="#">Tài liệu</a>
            </nav>

            <a href="#" class="auth-signup-btn">Đăng ký</a>
        </header>

        <section class="auth-main">
            <section class="auth-left">
                <p class="auth-eyebrow">PRECISION MONITORING</p>
                <h1>Giám sát sức khỏe <span>Thời gian thực</span></h1>
                <p class="auth-description">Hệ thống IoT tiên tiến giúp kết nối và phân tích dữ liệu sinh trắc học của bạn mọi lúc, mọi nơi với độ chính xác chuẩn y khoa.</p>

                <div class="auth-feature-list">
                    <article class="auth-feature">
                        <span class="auth-feature-icon blue" aria-hidden="true">
                            <svg viewBox="0 0 20 20" focusable="false" aria-hidden="true">
                                <path d="M10 17s-5-3.2-5-7a3 3 0 0 1 5-2 3 3 0 0 1 5 2c0 3.8-5 7-5 7z" fill="currentColor"/>
                            </svg>
                        </span>
                        <div>
                            <h3>Theo dõi nhịp tim, SpO2, nhiệt độ liên tục</h3>
                            <p>Cập nhật dữ liệu từ thiết bị đeo thông minh mỗi giây.</p>
                        </div>
                    </article>

                    <article class="auth-feature">
                        <span class="auth-feature-icon orange" aria-hidden="true">
                            <svg viewBox="0 0 20 20" focusable="false" aria-hidden="true">
                                <path d="M10 3l7 13H3L10 3zm0 4.1a1 1 0 0 0-1 1V11a1 1 0 0 0 2 0V8.1a1 1 0 0 0-1-1zm0 7.4a1.1 1.1 0 1 0 0-2.2 1.1 1.1 0 0 0 0 2.2z" fill="currentColor"/>
                            </svg>
                        </span>
                        <div>
                            <h3>Cảnh báo chỉ số bất thường</h3>
                            <p>Hệ thống AI tự động phát hiện và gửi thông báo khẩn cấp.</p>
                        </div>
                    </article>

                    <article class="auth-feature">
                        <span class="auth-feature-icon cyan" aria-hidden="true">
                            <svg viewBox="0 0 20 20" focusable="false" aria-hidden="true">
                                <path d="M10 4a6 6 0 1 1-4.2 1.8" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
                                <path d="M2.5 5.3h3.8v3.8" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/>
                            </svg>
                        </span>
                        <div>
                            <h3>Lịch sử dữ liệu cho đánh giá nhanh</h3>
                            <p>Biểu đồ xu hướng giúp bác sĩ đưa ra chẩn đoán chính xác.</p>
                        </div>
                    </article>
                </div>
            </section>

            <section class="auth-right">
                <article class="auth-login-card">
                    <div class="auth-lock" aria-hidden="true">
                        <svg viewBox="0 0 20 20" focusable="false" aria-hidden="true">
                            <path d="M6.8 8V6.5a3.2 3.2 0 1 1 6.4 0V8" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
                            <rect x="5.2" y="8" width="9.6" height="8.4" rx="2.1" fill="currentColor"/>
                            <circle cx="10" cy="12.2" r="1.1" fill="#16295a"/>
                        </svg>
                    </div>
                    <h2>Đăng nhập</h2>
                    <p class="auth-login-subtitle">Truy cập vào hệ thống giám sát sức khỏe của bạn</p>

                    <form class="auth-login-form" method="post" action="login.php" autocomplete="on">
                        <input type="hidden" name="next" value="<?php echo e($next); ?>">

                        <?php if ($error !== ''): ?>
                            <div class="flash-message error"><?php echo e($error); ?></div>
                        <?php endif; ?>

                        <label>
                            <span>TÀI KHOẢN</span>
                            <div class="auth-input-wrap">
                                <span class="auth-input-icon" aria-hidden="true">
                                    <svg viewBox="0 0 20 20" focusable="false" aria-hidden="true">
                                        <circle cx="10" cy="7" r="3.2" fill="currentColor"/>
                                        <path d="M4 16a6 6 0 0 1 12 0" fill="currentColor"/>
                                    </svg>
                                </span>
                                <input type="text" name="username" value="<?php echo e((string) ($_POST['username'] ?? '')); ?>" placeholder="Nhập tên đăng nhập của bạn" required autofocus>
                            </div>
                        </label>

                        <label>
                            <span>MẬT KHẨU</span>
                            <div class="auth-input-wrap">
                                <span class="auth-input-icon" aria-hidden="true">
                                    <svg viewBox="0 0 20 20" focusable="false" aria-hidden="true">
                                        <path d="M8.5 12.2a2.8 2.8 0 1 1 0-4.4h2.5a2.8 2.8 0 1 1 0 4.4H8.5z" fill="none" stroke="currentColor" stroke-width="1.8"/>
                                        <path d="M13.8 10h2.2" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
                                    </svg>
                                </span>
                                <input type="password" name="password" placeholder="••••••••" required>
                            </div>
                        </label>

                        <div class="auth-row">
                            <label class="auth-remember">
                                <input type="checkbox" name="remember" value="1">
                                <span>Ghi nhớ tôi</span>
                            </label>
                            <a href="#" class="auth-link">Quên mật khẩu?</a>
                        </div>

                        <button class="btn btn-primary auth-submit" type="submit">Đăng nhập</button>

                        <p class="auth-register">Chưa có tài khoản? <a href="#">Tạo tài khoản mới</a></p>
                        <p class="auth-secure-note"><span class="auth-dot" aria-hidden="true"></span> KẾT NỐI ĐÁM MÂY BẢO MẬT ĐANG HOẠT ĐỘNG</p>
                    </form>
                </article>
            </section>
        </section>
    </main>
</body>
</html>
