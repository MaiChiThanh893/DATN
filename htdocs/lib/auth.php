<?php

define('IOT_AUTH_PASSWORD_SALT', 'iot-health-dashboard-v1');

function auth_role_labels(): array
{
    return [
        'doctor' => "B\u{00E1}c s\u{0129}",
        'nurse' => "Y t\u{00E1}",
    ];
}

function auth_users(): array
{
    $roleLabels = auth_role_labels();

    return [
        'bacsi' => [
            'username' => 'bacsi',
            'displayName' => $roleLabels['doctor'],
            'role' => 'doctor',
            'roleLabel' => $roleLabels['doctor'],
            'passwordHash' => '3c8f6be0b6af2dfa065a548a2504d42d7d5659c8517a75274a5dae23124d8472',
        ],
        'yta' => [
            'username' => 'yta',
            'displayName' => $roleLabels['nurse'],
            'role' => 'nurse',
            'roleLabel' => $roleLabels['nurse'],
            'passwordHash' => '2f8143bf39b5f3a8b1db3e9080260d969a1de98a793c6b74da26a8bd620bc6d9',
        ],
    ];
}

function auth_start_session(): void
{
    if (session_status() === PHP_SESSION_ACTIVE) {
        return;
    }

    $secure = !empty($_SERVER['HTTPS']) && $_SERVER['HTTPS'] !== 'off';
    session_name('iot_health_session');

    if (PHP_VERSION_ID >= 70300) {
        session_set_cookie_params([
            'lifetime' => 0,
            'path' => '/',
            'secure' => $secure,
            'httponly' => true,
            'samesite' => 'Lax',
        ]);
    } else {
        session_set_cookie_params(0, '/', '', $secure, true);
    }

    session_start();
}

function auth_password_hash(string $password): string
{
    return hash('sha256', IOT_AUTH_PASSWORD_SALT . $password);
}

function auth_user_for_credentials(string $username, string $password): ?array
{
    $username = strtolower(trim($username));
    $users = auth_users();

    if (!isset($users[$username])) {
        return null;
    }

    $user = $users[$username];
    if (!hash_equals($user['passwordHash'], auth_password_hash($password))) {
        return null;
    }

    return $user;
}

function auth_attempt(string $username, string $password): bool
{
    return auth_user_for_credentials($username, $password) !== null;
}

function auth_login(array $user): void
{
    auth_start_session();
    session_regenerate_id(true);
    $_SESSION['iot_auth'] = [
        'loggedIn' => true,
        'username' => $user['username'],
        'displayName' => $user['displayName'],
        'role' => $user['role'],
        'roleLabel' => $user['roleLabel'],
        'loggedInAt' => time(),
    ];
}

function auth_logout(): void
{
    auth_start_session();
    $_SESSION = [];

    if (ini_get('session.use_cookies')) {
        $params = session_get_cookie_params();
        setcookie(session_name(), '', time() - 42000, $params['path'], $params['domain'], $params['secure'], $params['httponly']);
    }

    session_destroy();
}

function auth_is_logged_in(): bool
{
    auth_start_session();
    return !empty($_SESSION['iot_auth']['loggedIn']);
}

function auth_current_user(): string
{
    auth_start_session();
    return (string) ($_SESSION['iot_auth']['username'] ?? '');
}

function auth_current_display_name(): string
{
    auth_start_session();
    return (string) ($_SESSION['iot_auth']['displayName'] ?? auth_current_user());
}

function auth_current_role(): string
{
    auth_start_session();
    return (string) ($_SESSION['iot_auth']['role'] ?? '');
}

function auth_current_role_label(): string
{
    auth_start_session();
    $role = auth_current_role();
    $roleLabels = auth_role_labels();

    if (isset($roleLabels[$role])) {
        return $roleLabels[$role];
    }

    return (string) ($_SESSION['iot_auth']['roleLabel'] ?? '');
}

function auth_can_access_api_action(string $action, string $method): bool
{
    if (auth_api_allows_public_access($action, $method)) {
        return true;
    }

    $role = auth_current_role();

    if ($role === 'doctor') {
        return true;
    }

    if ($role === 'nurse') {
        return in_array($action, ['devices', 'device', 'upsert_patient'], true);
    }

    return false;
}

function auth_sanitize_redirect(?string $target): string
{
    $target = trim((string) $target);

    if ($target === ''
        || preg_match('/[\r\n]/', $target)
        || preg_match('/^[a-z][a-z0-9+.-]*:/i', $target)
        || strpos($target, '//') === 0
    ) {
        return 'index.php';
    }

    return $target;
}

function auth_current_target(): string
{
    return auth_sanitize_redirect($_SERVER['REQUEST_URI'] ?? 'index.php');
}

function require_login(): void
{
    if (auth_is_logged_in()) {
        return;
    }

    header('Location: login.php?next=' . rawurlencode(auth_current_target()));
    exit;
}

function auth_api_allows_public_access(string $action, string $method): bool
{
    return strtoupper($method) === 'POST' && $action === 'post_data';
}
