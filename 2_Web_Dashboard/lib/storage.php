<?php

define('IOT_DATA_DIR', __DIR__ . '/../data');
define('IOT_STORE_FILE', IOT_DATA_DIR . '/iot_store.json');
define('IOT_HISTORY_LIMIT', 500);

function ensure_data_store(): void
{
    if (!is_dir(IOT_DATA_DIR)) {
        mkdir(IOT_DATA_DIR, 0777, true);
    }

    if (!file_exists(IOT_STORE_FILE)) {
        $initial = [
            'devices' => [],
            'meta' => [
                'createdAt' => gmdate('c'),
                'updatedAt' => gmdate('c'),
            ],
        ];
        file_put_contents(IOT_STORE_FILE, json_encode($initial, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE), LOCK_EX);
    }
}

function default_store(): array
{
    return [
        'devices' => [],
        'meta' => [
            'createdAt' => gmdate('c'),
            'updatedAt' => gmdate('c'),
        ],
    ];
}

function load_store(): array
{
    ensure_data_store();
    $content = @file_get_contents(IOT_STORE_FILE);

    if ($content === false || trim($content) === '') {
        return default_store();
    }

    $data = json_decode($content, true);
    if (!is_array($data)) {
        return default_store();
    }

    $data['devices'] = isset($data['devices']) && is_array($data['devices']) ? $data['devices'] : [];
    $data['meta'] = isset($data['meta']) && is_array($data['meta']) ? $data['meta'] : [];

    return $data;
}

function save_store(array $store): bool
{
    ensure_data_store();
    $store['meta']['updatedAt'] = gmdate('c');
    $json = json_encode($store, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE);
    return file_put_contents(IOT_STORE_FILE, $json, LOCK_EX) !== false;
}

function normalize_device_id(string $deviceId): string
{
    return trim($deviceId);
}

function device_exists(array $store, string $deviceId): bool
{
    return isset($store['devices'][$deviceId]) && is_array($store['devices'][$deviceId]);
}

function build_alerts(?float $heartRate, ?float $spo2, ?float $temperature, ?float $humidity = null, $sosStatus = null): array
{
    $alerts = [];

    if ($sosStatus) {
        $alerts[] = "SOS — Thiết bị gọi khẩn cấp";
    }

    if ($heartRate !== null && ($heartRate > 120 || $heartRate < 50)) {
        $alerts[] = "Nhịp tim bất thường ({$heartRate}bpm)";
    }

    if ($spo2 !== null && $spo2 < 90) {
        $alerts[] = "Nồng độ SpO₂ thấp ({$spo2}%)";
    }

    if ($temperature !== null && $temperature > 38.5) {
        $alerts[] = "Nhiệt độ cơ thể cao bất thường ({$temperature}°C)";
    }

    if ($humidity !== null && ($humidity < 30 || $humidity > 70)) {
        $alerts[] = "Độ ẩm môi trường ngoài ngưỡng ({$humidity}%)";
    }
    return $alerts;
}

function create_or_get_device(array &$store, string $deviceId, string $patientName = ''): array
{
    if (!device_exists($store, $deviceId)) {
        $store['devices'][$deviceId] = [
            'deviceId' => $deviceId,
            'patientName' => $patientName,
            'latest' => null,
            'history' => [],
            'createdAt' => gmdate('c'),
            'updatedAt' => gmdate('c'),
        ];
    }

    return $store['devices'][$deviceId];
}

function list_devices(array $store): array
{
    $devices = array_values($store['devices']);

    usort($devices, function ($a, $b) {
        return strcmp($b['updatedAt'] ?? '', $a['updatedAt'] ?? '');
    });

    return array_map(function ($device) {
        $latest = $device['latest'] ?? null;
        $alerts = [];

        if ($latest) {
            $alerts = build_alerts(
                isset($latest['heartRate']) ? (float) $latest['heartRate'] : null,
                isset($latest['spo2']) ? (float) $latest['spo2'] : null,
                isset($latest['temperature']) ? (float) $latest['temperature'] : null,
                isset($latest['humidity']) ? (float) $latest['humidity'] : null,
                $latest['sosStatus'] ?? null
            );
        }

        return [
            'deviceId' => $device['deviceId'] ?? '',
            'patientName' => $device['patientName'] ?? '',
            'latest' => $latest,
            'alerts' => $alerts,
            'historyCount' => count($device['history'] ?? []),
            'updatedAt' => $device['updatedAt'] ?? null,
        ];
    }, $devices);
}

function get_device_detail(array $store, string $deviceId): ?array
{
    if (!device_exists($store, $deviceId)) {
        return null;
    }

    $device = $store['devices'][$deviceId];
    $latest = $device['latest'] ?? null;
    $alerts = [];

    if ($latest) {
        $alerts = build_alerts(
            isset($latest['heartRate']) ? (float) $latest['heartRate'] : null,
            isset($latest['spo2']) ? (float) $latest['spo2'] : null,
            isset($latest['temperature']) ? (float) $latest['temperature'] : null,
            isset($latest['humidity']) ? (float) $latest['humidity'] : null,
            $latest['sosStatus'] ?? null
        );
    }

    return [
        'deviceId' => $device['deviceId'] ?? '',
        'patientName' => $device['patientName'] ?? '',
        'latest' => $latest,
        'history' => array_values($device['history'] ?? []),
        'alerts' => $alerts,
        'createdAt' => $device['createdAt'] ?? null,
        'updatedAt' => $device['updatedAt'] ?? null,
    ];
}

function upsert_patient_for_device(array &$store, string $deviceId, string $patientName): void
{
    if (!device_exists($store, $deviceId)) {
        $store['devices'][$deviceId] = [
            'deviceId' => $deviceId,
            'patientName' => $patientName,
            'latest' => null,
            'history' => [],
            'createdAt' => gmdate('c'),
            'updatedAt' => gmdate('c'),
        ];
        return;
    }

    $store['devices'][$deviceId]['patientName'] = $patientName;
    $store['devices'][$deviceId]['updatedAt'] = gmdate('c');
}

function delete_device_by_id(array &$store, string $deviceId): bool
{
    if (!device_exists($store, $deviceId)) {
        return false;
    }

    unset($store['devices'][$deviceId]);
    return true;
}

function append_device_data(array &$store, string $deviceId, ?string $patientName, float $heartRate, float $spo2, float $temperature, ?float $humidity = null, ?string $timestamp = null, $sosStatus = null): array
{
    $deviceId = normalize_device_id($deviceId);
    if (!device_exists($store, $deviceId)) {
        $store['devices'][$deviceId] = [
            'deviceId' => $deviceId,
            'patientName' => $patientName ?? '',
            'latest' => null,
            'history' => [],
            'createdAt' => gmdate('c'),
            'updatedAt' => gmdate('c'),
        ];
    }

    if ($patientName !== null && trim($patientName) !== '') {
        $store['devices'][$deviceId]['patientName'] = trim($patientName);
    }

    $entry = [
        'timestamp' => $timestamp ?: gmdate('c'),
        'heartRate' => round($heartRate, 1),
        'spo2' => round($spo2, 1),
        'temperature' => round($temperature, 1),
        'humidity' => $humidity !== null ? round($humidity, 1) : null,
        'sosStatus' => $sosStatus ? 1 : 0,
    ];

    $store['devices'][$deviceId]['latest'] = $entry;
    $store['devices'][$deviceId]['history'][] = $entry;
    $store['devices'][$deviceId]['history'] = array_slice($store['devices'][$deviceId]['history'], -IOT_HISTORY_LIMIT);
    $store['devices'][$deviceId]['updatedAt'] = gmdate('c');

    return $entry;
}
