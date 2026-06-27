const KEY_TOAST = 'iot_settings_toasts';
const KEY_RANGE = 'iot_chart_range';
const KEY_READ = 'iot_read_alerts';

const toggleToast = document.getElementById('toggleToast');
const defaultRange = document.getElementById('defaultRange');
const clearRead = document.getElementById('clearRead');
const msg = document.getElementById('globalMessage');

function flash(text) {
    if (!msg) return;
    msg.innerHTML = `<div class="flash-message success">${text}</div>`;
    setTimeout(() => { if (msg) msg.innerHTML = ''; }, 2500);
}
function get(key, fallback) {
    try { const v = localStorage.getItem(key); return v === null ? fallback : v; } catch (e) { return fallback; }
}
function set(key, value) {
    try { localStorage.setItem(key, value); } catch (e) {}
}

if (toggleToast) {
    toggleToast.checked = get(KEY_TOAST, '1') !== '0';
    toggleToast.addEventListener('change', () => {
        set(KEY_TOAST, toggleToast.checked ? '1' : '0');
        flash(toggleToast.checked ? 'Đã bật thông báo toast cảnh báo.' : 'Đã tắt thông báo toast cảnh báo.');
    });
}

if (defaultRange) {
    defaultRange.value = get(KEY_RANGE, '200');
    defaultRange.addEventListener('change', () => {
        set(KEY_RANGE, defaultRange.value);
        flash('Đã lưu số mẫu biểu đồ mặc định: ' + (defaultRange.value === 'all' ? 'Tất cả' : defaultRange.value));
    });
}

if (clearRead) {
    clearRead.addEventListener('click', () => {
        try { localStorage.removeItem(KEY_READ); } catch (e) {}
        flash('Đã xóa trạng thái "đã đọc" của tất cả cảnh báo.');
    });
}
