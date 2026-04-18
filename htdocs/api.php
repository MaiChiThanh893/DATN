<?php

header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

require_once __DIR__ . '/lib/storage.php';

function json_response(array $payload, int $status = 200): void
{
    http_response_code($status);
    echo json_encode($payload, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    exit;
}

function request_data(): array
{
    $raw = file_get_contents('php://input');
    $json = json_decode($raw, true);
    if (is_array($json)) {
        return $json;
    }
    return $_POST;
}

$action = $_GET['action'] ?? 'devices';
$store = load_store();

try {
    if ($_SERVER['REQUEST_METHOD'] === 'GET') {
        switch ($action) {
            case 'devices':
                json_response([
                    'success' => true,
                    'devices' => list_devices($store),
                ]);
                break;

            case 'device':
                $deviceId = normalize_device_id($_GET['deviceId'] ?? '');
                if ($deviceId === '') {
                    json_response(['success' => false, 'message' => 'Thiếu deviceId'], 422);
                }

                $device = get_device_detail($store, $deviceId);
                if (!$device) {
                    json_response(['success' => false, 'message' => 'Không tìm thấy thiết bị'], 404);
                }

                json_response([
                    'success' => true,
                    'device' => $device,
                ]);
                break;

            default:
                json_response(['success' => false, 'message' => 'API không hợp lệ'], 404);
        }
    }

    if ($_SERVER['REQUEST_METHOD'] === 'POST') {
        $input = request_data();

        switch ($action) {
            case 'post_data':
                $deviceId = normalize_device_id((string) ($input['deviceId'] ?? ''));
                $patientName = isset($input['patientName']) ? trim((string) $input['patientName']) : null;
                $heartRate = isset($input['heartRate']) ? (float) $input['heartRate'] : null;
                $spo2 = isset($input['spo2']) ? (float) $input['spo2'] : null;
                $temperature = isset($input['temperature']) ? (float) $input['temperature'] : null;
                $timestamp = isset($input['timestamp']) ? trim((string) $input['timestamp']) : null;

                if ($deviceId === '' || $heartRate === null || $spo2 === null || $temperature === null) {
                    json_response([
                        'success' => false,
                        'message' => 'Thiếu deviceId hoặc dữ liệu sức khỏe',
                    ], 422);
                }

                $entry = append_device_data($store, $deviceId, $patientName, $heartRate, $spo2, $temperature, $timestamp);
                save_store($store);

                json_response([
                    'success' => true,
                    'message' => 'Đã nhận dữ liệu từ thiết bị',
                    'entry' => $entry,
                    'device' => get_device_detail($store, $deviceId),
                ]);
                break;

            case 'upsert_patient':
                $deviceId = normalize_device_id((string) ($input['deviceId'] ?? ''));
                $patientName = trim((string) ($input['patientName'] ?? ''));

                if ($deviceId === '') {
                    json_response(['success' => false, 'message' => 'Thiếu deviceId'], 422);
                }

                upsert_patient_for_device($store, $deviceId, $patientName);
                save_store($store);

                json_response([
                    'success' => true,
                    'message' => 'Đã lưu thông tin bệnh nhân',
                    'device' => get_device_detail($store, $deviceId),
                ]);
                break;

            case 'delete_device':
                $deviceId = normalize_device_id((string) ($input['deviceId'] ?? ''));
                if ($deviceId === '') {
                    json_response(['success' => false, 'message' => 'Thiếu deviceId'], 422);
                }

                if (!delete_device_by_id($store, $deviceId)) {
                    json_response(['success' => false, 'message' => 'Không tìm thấy thiết bị'], 404);
                }

                save_store($store);
                json_response([
                    'success' => true,
                    'message' => 'Đã xóa thiết bị',
                ]);
                break;

            case 'seed_demo':
                $now = time();
                $demoDevices = [
                    ['id' => 'ESP32_001', 'name' => 'Nguyễn Văn An'],
                    ['id' => 'ESP32_002', 'name' => 'Trần Thị Bình'],
                    ['id' => 'ESP32_003', 'name' => 'Lê Minh Châu'],
                ];

                foreach ($demoDevices as $index => $demo) {
                    upsert_patient_for_device($store, $demo['id'], $demo['name']);
                    for ($i = 0; $i < 18; $i++) {
                        append_device_data(
                            $store,
                            $demo['id'],
                            $demo['name'],
                            70 + ($index * 5) + (($i % 4) * 3),
                            97 - (($i + $index) % 3),
                            36.5 + (($i + $index) % 5) * 0.2,
                            gmdate('c', $now - ((18 - $i) * 60))
                        );
                    }
                }

                append_device_data($store, 'ESP32_002', 'Trần Thị Bình', 128, 88, 38.6, gmdate('c'));
                save_store($store);

                json_response([
                    'success' => true,
                    'message' => 'Đã tạo dữ liệu mẫu',
                    'devices' => list_devices($store),
                ]);
                break;

            default:
                json_response(['success' => false, 'message' => 'API không hợp lệ'], 404);
        }
    }

    json_response(['success' => false, 'message' => 'Method không được hỗ trợ'], 405);
} catch (Throwable $e) {
    json_response([
        'success' => false,
        'message' => 'Lỗi máy chủ',
        'error' => $e->getMessage(),
    ], 500);
}
