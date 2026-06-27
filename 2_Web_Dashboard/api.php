<?php

header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if (!defined('JSON_UNESCAPED_UNICODE')) {
    define('JSON_UNESCAPED_UNICODE', 0);
}
if (!defined('JSON_UNESCAPED_SLASHES')) {
    define('JSON_UNESCAPED_SLASHES', 0);
}

function api_json_encode($payload)
{
    $flags = JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES;
    return json_encode($payload, $flags);
}

function json_response($payload, $status)
{
    http_response_code((int) $status);
    echo api_json_encode($payload);
    exit;
}

function fail_response($message, $status, $extra)
{
    $payload = array(
        'success' => false,
        'message' => (string) $message,
    );

    if (is_array($extra)) {
        foreach ($extra as $key => $value) {
            $payload[$key] = $value;
        }
    }

    json_response($payload, $status);
}

function request_data()
{
    $raw = file_get_contents('php://input');
    $json = json_decode($raw, true);

    if (is_array($json)) {
        return $json;
    }

    return $_POST;
}

function request_value($input, $keys, $default)
{
    if (!is_array($keys)) {
        $keys = array($keys);
    }

    foreach ($keys as $key) {
        if (is_array($input) && array_key_exists($key, $input)) {
            return $input[$key];
        }
    }

    return $default;
}

function request_string_or_null($input, $keys)
{
    $value = request_value($input, $keys, null);
    if ($value === null) {
        return null;
    }

    $value = trim((string) $value);
    return $value === '' ? null : $value;
}

function request_float_or_null($input, $keys)
{
    $value = request_value($input, $keys, null);
    if ($value === null || $value === '') {
        return null;
    }

    if (is_string($value)) {
        $value = str_replace(',', '.', trim($value));
    }

    if (!is_numeric($value)) {
        return null;
    }

    return (float) $value;
}

function request_bool_or_null($input, $keys)
{
    $value = request_value($input, $keys, null);
    if ($value === null || $value === '') {
        return null;
    }
    if (is_bool($value)) {
        return $value;
    }
    $v = strtolower(trim((string) $value));
    if (in_array($v, array('1', 'true', 'yes', 'on'), true)) {
        return true;
    }
    if (in_array($v, array('0', 'false', 'no', 'off'), true)) {
        return false;
    }
    return null;
}

register_shutdown_function(function () {
    $error = error_get_last();
    if (!$error) {
        return;
    }

    $fatalTypes = array(E_ERROR, E_PARSE, E_CORE_ERROR, E_COMPILE_ERROR, E_USER_ERROR);
    if (!in_array($error['type'], $fatalTypes, true)) {
        return;
    }

    if (!headers_sent()) {
        header('Content-Type: application/json; charset=utf-8');
        http_response_code(500);
    }

    echo api_json_encode(array(
        'success' => false,
        'message' => 'Loi may chu',
        'error' => $error['message'],
        'file' => isset($error['file']) ? basename($error['file']) : null,
        'line' => isset($error['line']) ? (int) $error['line'] : null,
    ));
});

if (isset($_SERVER['REQUEST_METHOD']) && $_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

require_once dirname(__FILE__) . '/lib/storage.php';
require_once dirname(__FILE__) . '/lib/auth.php';

$method = isset($_SERVER['REQUEST_METHOD']) ? strtoupper($_SERVER['REQUEST_METHOD']) : 'GET';
$action = isset($_GET['action']) ? (string) $_GET['action'] : 'devices';

if (!auth_api_allows_public_access($action, $method) && !auth_is_logged_in()) {
    fail_response('Phien dang nhap da het han', 401, array('loginUrl' => 'login.php'));
}

if (!auth_can_access_api_action($action, $method)) {
    fail_response('Tai khoan nay khong co quyen thuc hien thao tac nay', 403, array());
}

$store = load_store();

try {
    if ($method === 'GET') {
        if ($action === 'health') {
            json_response(array(
                'success' => true,
                'message' => 'API san sang',
                'phpVersion' => PHP_VERSION,
                'storeFile' => basename(IOT_STORE_FILE),
                'storeExists' => file_exists(IOT_STORE_FILE),
                'storeWritable' => (file_exists(IOT_STORE_FILE) ? is_writable(IOT_STORE_FILE) : is_writable(IOT_DATA_DIR)),
            ), 200);
        }

        if ($action === 'devices') {
            json_response(array(
                'success' => true,
                'devices' => list_devices($store),
            ), 200);
        }

        if ($action === 'device') {
            $deviceId = normalize_device_id(isset($_GET['deviceId']) ? $_GET['deviceId'] : '');
            if ($deviceId === '') {
                fail_response('Thieu deviceId', 422, array());
            }

            $device = get_device_detail($store, $deviceId);
            if (!$device) {
                fail_response('Khong tim thay thiet bi', 404, array());
            }

            json_response(array(
                'success' => true,
                'device' => $device,
            ), 200);
        }

        fail_response('API khong hop le', 404, array());
    }

    if ($method === 'POST') {
        $input = request_data();

        if ($action === 'post_data') {
            $deviceId = normalize_device_id(request_value($input, array('deviceId', 'device_id', 'id'), ''));
            $patientName = request_string_or_null($input, array('patientName', 'patient_name', 'name'));
            $heartRate = request_float_or_null($input, array('heartRate', 'heart_rate', 'hr'));
            $spo2 = request_float_or_null($input, array('spo2', 'spO2', 'oxygen'));
            $temperature = request_float_or_null($input, array('temperature', 'temp', 'bodyTemp', 'temperatureC'));
            $humidity = request_float_or_null($input, array('humidity', 'humid', 'do_am', 'doAm'));
            $sosStatus = request_bool_or_null($input, array('sos', 'sosstatus', 'sosStatus', 'sos_status'));
            $timestamp = request_string_or_null($input, array('timestamp', 'time', 'createdAt'));

            if ($deviceId === '' || $heartRate === null || $spo2 === null || $temperature === null) {
                fail_response('Thieu deviceId hoac du lieu suc khoe', 422, array('receivedKeys' => array_keys(is_array($input) ? $input : array())));
            }

            $entry = append_device_data($store, $deviceId, $patientName, $heartRate, $spo2, $temperature, $humidity, $timestamp, $sosStatus);
            if (!save_store($store)) {
                fail_response('Khong the ghi du lieu vao data/iot_store.json. Kiem tra quyen ghi.', 500, array());
            }

            json_response(array(
                'success' => true,
                'message' => 'Da nhan du lieu tu thiet bi',
                'entry' => $entry,
                'device' => get_device_detail($store, $deviceId),
            ), 200);
        }

        if ($action === 'upsert_patient') {
            $deviceId = normalize_device_id(request_value($input, array('deviceId', 'device_id', 'id'), ''));
            $patientName = trim((string) request_value($input, array('patientName', 'patient_name', 'name'), ''));

            if ($deviceId === '') {
                fail_response('Thieu deviceId', 422, array());
            }

            upsert_patient_for_device($store, $deviceId, $patientName);
            if (!save_store($store)) {
                fail_response('Khong the luu thong tin benh nhan', 500, array());
            }

            json_response(array(
                'success' => true,
                'message' => 'Da luu thong tin benh nhan',
                'device' => get_device_detail($store, $deviceId),
            ), 200);
        }

        if ($action === 'delete_device') {
            $deviceId = normalize_device_id(request_value($input, array('deviceId', 'device_id', 'id'), ''));
            if ($deviceId === '') {
                fail_response('Thieu deviceId', 422, array());
            }

            if (!delete_device_by_id($store, $deviceId)) {
                fail_response('Khong tim thay thiet bi', 404, array());
            }

            if (!save_store($store)) {
                fail_response('Khong the xoa thiet bi khoi kho du lieu', 500, array());
            }

            json_response(array(
                'success' => true,
                'message' => 'Da xoa thiet bi',
            ), 200);
        }

        if ($action === 'seed_demo') {
            $now = time();
            $demoDevices = array(
                array('id' => 'ESP32_001', 'name' => 'Nguyen Van An'),
                array('id' => 'ESP32_002', 'name' => 'Tran Thi Binh'),
                array('id' => 'ESP32_003', 'name' => 'Le Minh Chau'),
            );

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
                        48 + (($i + $index) % 7) * 2,
                        gmdate('c', $now - ((18 - $i) * 60))
                    );
                }
            }

            append_device_data($store, 'ESP32_002', 'Tran Thi Binh', 128, 88, 38.6, 74, gmdate('c'));

            if (!save_store($store)) {
                fail_response('Khong the tao du lieu mau', 500, array());
            }

            json_response(array(
                'success' => true,
                'message' => 'Da tao du lieu mau',
                'devices' => list_devices($store),
            ), 200);
        }

        fail_response('API khong hop le', 404, array());
    }

    fail_response('Method khong duoc ho tro', 405, array());
} catch (Exception $e) {
    fail_response('Loi may chu', 500, array('error' => $e->getMessage()));
}
