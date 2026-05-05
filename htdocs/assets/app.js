const API_BASE = 'api.php?action=';

const state = {
    devices: [],
    selectedDeviceId: null,
    deviceQuery: '',
    charts: {},
    pollingHandle: null,
    role: document.body.dataset.role || '',
};

const els = {
    deviceList: document.getElementById('deviceList'),
    deviceCountBadge: document.getElementById('deviceCountBadge'),
    deviceForm: document.getElementById('deviceForm'),
    deviceIdInput: document.getElementById('deviceIdInput'),
    patientNameInput: document.getElementById('patientNameInput'),
    deleteBtn: document.getElementById('deleteBtn'),
    refreshBtn: document.getElementById('refreshBtn'),
    seedBtn: document.getElementById('seedBtn'),
    deviceSearchInput: document.getElementById('deviceSearchInput'),
    detailTitle: document.getElementById('detailTitle'),
    detailSubtitle: document.getElementById('detailSubtitle'),
    selectedHealthBadge: document.getElementById('selectedHealthBadge'),
    infoDeviceIdHero: document.getElementById('infoDeviceIdHero'),
    globalMessage: document.getElementById('globalMessage'),
    alertsContainer: document.getElementById('alertsContainer'),
    heartRateValue: document.getElementById('heartRateValue'),
    spo2Value: document.getElementById('spo2Value'),
    temperatureValue: document.getElementById('temperatureValue'),
    updatedAtValue: document.getElementById('updatedAtValue'),
    infoDeviceId: document.getElementById('infoDeviceId'),
    infoPatientName: document.getElementById('infoPatientName'),
    infoHistoryCount: document.getElementById('infoHistoryCount'),
    infoCreatedAt: document.getElementById('infoCreatedAt'),
    currentStatusPanel: document.getElementById('currentStatusPanel'),
    historyTableBody: document.getElementById('historyTableBody'),
    summaryTotalDevices: document.getElementById('summaryTotalDevices'),
    summaryAlertDevices: document.getElementById('summaryAlertDevices'),
    summaryAvgHeartRate: document.getElementById('summaryAvgHeartRate'),
    summaryAvgSpo2: document.getElementById('summaryAvgSpo2'),
    heartRateCard: document.getElementById('heartRateCard'),
    spo2Card: document.getElementById('spo2Card'),
    temperatureCard: document.getElementById('temperatureCard'),
};

function formatDateTime(value) {
    if (!value) return '--';
    const date = new Date(value);
    if (Number.isNaN(date.getTime())) return value;
    return date.toLocaleString('vi-VN');
}

function escapeHtml(text) {
    return String(text)
        .replaceAll('&', '&amp;')
        .replaceAll('<', '&lt;')
        .replaceAll('>', '&gt;')
        .replaceAll('"', '&quot;')
        .replaceAll("'", '&#39;');
}

function safeMetric(value, unit = '') {
    if (value === undefined || value === null || value === '') return `--${unit ? ` ${unit}` : ''}`;
    return `${value}${unit ? ` ${unit}` : ''}`;
}

function showMessage(text, type = 'info') {
    els.globalMessage.innerHTML = `<div class="flash-message ${type}">${escapeHtml(text)}</div>`;
    if (type !== 'error') {
        setTimeout(() => {
            if (els.globalMessage.textContent.includes(text)) {
                els.globalMessage.innerHTML = '';
            }
        }, 3200);
    }
}

async function apiGet(action, query = '') {
    const response = await fetch(`${API_BASE}${action}${query}`);
    const data = await response.json();
    if (response.status === 401) {
        window.location.href = `login.php?next=${encodeURIComponent(window.location.pathname + window.location.search)}`;
        throw new Error(data.message || 'Phien dang nhap da het han');
    }
    if (!response.ok || !data.success) {
        throw new Error(data.message || 'API lỗi');
    }
    return data;
}

async function apiPost(action, payload) {
    const formData = new URLSearchParams();
    Object.entries(payload || {}).forEach(([key, value]) => {
        if (value === undefined || value === null) return;
        formData.append(key, String(value));
    });

    const response = await fetch(`${API_BASE}${action}`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded; charset=UTF-8' },
        body: formData.toString(),
    });
    const data = await response.json();
    if (response.status === 401) {
        window.location.href = `login.php?next=${encodeURIComponent(window.location.pathname + window.location.search)}`;
        throw new Error(data.message || 'Phien dang nhap da het han');
    }
    if (!response.ok || !data.success) {
        throw new Error(data.message || 'API lỗi');
    }
    return data;
}

function createChart(canvasId, label, color, yTitle) {
    const ctx = document.getElementById(canvasId).getContext('2d');
    return new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label,
                data: [],
                borderColor: color,
                backgroundColor: color,
                pointRadius: 2.5,
                pointHoverRadius: 4,
                borderWidth: 2,
                tension: 0.3,
                fill: false,
            }],
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    labels: {
                        color: '#c9d5f2',
                        boxWidth: 10,
                    },
                },
            },
            scales: {
                x: {
                    ticks: {
                        color: '#8d9bb8',
                        maxTicksLimit: 7,
                    },
                    grid: {
                        color: 'rgba(255,255,255,0.06)',
                    },
                },
                y: {
                    ticks: {
                        color: '#8d9bb8',
                    },
                    title: {
                        display: true,
                        text: yTitle,
                        color: '#8d9bb8',
                    },
                    grid: {
                        color: 'rgba(255,255,255,0.06)',
                    },
                },
            },
        },
    });
}

function initCharts() {
    state.charts.heartRate = createChart('heartRateChart', 'Nhịp tim', '#2b7fff', 'bpm');
    state.charts.spo2 = createChart('spo2Chart', 'Nồng độ SpO₂', '#22c55e', '%');
    state.charts.temperature = createChart('temperatureChart', 'Nhiệt độ cơ thể', '#fb923c', '°C');
}

function updateChart(chart, labels, values) {
    chart.data.labels = labels;
    chart.data.datasets[0].data = values;
    chart.update();
}

function getLatestReading(device) {
    return device && device.latest ? device.latest : {};
}

function average(values) {
    if (!values.length) return null;
    const total = values.reduce((sum, value) => sum + value, 0);
    return total / values.length;
}

function updateSystemSummary() {
    const devices = state.devices || [];
    const latestReadings = devices.map(getLatestReading);
    const heartRates = latestReadings.map((item) => Number(item.heartRate)).filter((value) => Number.isFinite(value));
    const spo2Values = latestReadings.map((item) => Number(item.spo2)).filter((value) => Number.isFinite(value));
    const alertCount = devices.filter((device) => Array.isArray(device.alerts) && device.alerts.length > 0).length;

    els.summaryTotalDevices.textContent = String(devices.length);
    els.summaryAlertDevices.textContent = String(alertCount);
    const avgHeart = average(heartRates);
    const avgSpo2 = average(spo2Values);
    els.summaryAvgHeartRate.textContent = avgHeart === null ? '--' : `${avgHeart.toFixed(1)} bpm`;
    els.summaryAvgSpo2.textContent = avgSpo2 === null ? '--' : `${avgSpo2.toFixed(1)} %`;
}

function renderDeviceList() {
    updateSystemSummary();

    const query = state.deviceQuery.trim().toLowerCase();
    const devices = state.devices.filter((device) => {
        if (!query) return true;
        return `${device.deviceId} ${device.patientName || ''}`.toLowerCase().includes(query);
    });

    els.deviceCountBadge.textContent = String(devices.length);

    if (!devices.length) {
        els.deviceList.className = 'device-list empty-state';
        els.deviceList.innerHTML = query ? 'Không tìm thấy thiết bị phù hợp.' : 'Chưa có thiết bị nào.';
        return;
    }

    els.deviceList.className = 'device-list';
    els.deviceList.innerHTML = devices.map((device) => {
        const latest = getLatestReading(device);
        const alerts = Array.isArray(device.alerts) ? device.alerts : [];
        const isActive = device.deviceId === state.selectedDeviceId;
        return `
            <button class="device-item ${isActive ? 'active' : ''} ${alerts.length ? 'danger' : ''}" data-device-id="${escapeHtml(device.deviceId)}" type="button">
                <div class="device-item-top">
                    <div>
                        <div class="device-id">${escapeHtml(device.deviceId)}</div>
                        <div class="patient-name">${escapeHtml(device.patientName || 'Chưa gán bệnh nhân')}</div>
                    </div>
                    <span class="device-badge ${alerts.length ? 'danger' : 'normal'}">${alerts.length ? 'Cảnh báo' : 'Ổn định'}</span>
                </div>
                <div class="device-metrics">
                    <div><span>Nhịp tim</span><strong>${safeMetric(latest.heartRate, 'bpm')}</strong></div>
                    <div><span>Nồng độ SpO₂</span><strong>${safeMetric(latest.spo2, '%')}</strong></div>
                    <div><span>Nhiệt độ cơ thể</span><strong>${safeMetric(latest.temperature, '°C')}</strong></div>
                </div>
            </button>
        `;
    }).join('');

    els.deviceList.querySelectorAll('.device-item').forEach((item) => {
        item.addEventListener('click', () => selectDevice(item.dataset.deviceId));
    });
}

function clearMetricStates() {
    [els.heartRateCard, els.spo2Card, els.temperatureCard].forEach((card) => card.classList.remove('metric-alert'));
}

function renderAlerts(alerts) {
    if (!alerts || !alerts.length) {
        els.alertsContainer.innerHTML = '';
        return;
    }

    els.alertsContainer.innerHTML = `
        <div class="alerts-stack">
            ${alerts.map((alert) => `<div class="alert-banner">⚠ ${escapeHtml(alert)}</div>`).join('')}
        </div>
    `;
}

function fillForm(device) {
    els.deviceIdInput.value = device.deviceId || '';
    els.patientNameInput.value = device.patientName || '';
}

function isEditingDeviceForm() {
    const activeElement = document.activeElement;
    return activeElement === els.deviceIdInput || activeElement === els.patientNameInput;
}

function renderStatusPanel(device, alerts) {
    if (!device) {
        els.currentStatusPanel.className = 'current-status empty-block';
        els.currentStatusPanel.textContent = 'Chọn 1 thiết bị để xem mức độ ổn định và nhận xét nhanh về dữ liệu hiện tại.';
        els.selectedHealthBadge.className = 'health-badge neutral';
        els.selectedHealthBadge.textContent = 'Chưa có dữ liệu';
        return;
    }

    const isDanger = alerts.length > 0;
    els.currentStatusPanel.className = `current-status ${isDanger ? 'danger' : 'normal'}`;
    els.selectedHealthBadge.className = `health-badge ${isDanger ? 'danger' : 'normal'}`;
    els.selectedHealthBadge.textContent = isDanger ? 'Đang cảnh báo' : 'Ổn định';

    const message = isDanger
        ? `Thiết bị đang có ${alerts.length} cảnh báo cần theo dõi: ${alerts.join(', ')}.`
        : 'Thiết bị đang ở trạng thái ổn định. Các chỉ số hiện nằm trong ngưỡng an toàn.';

    els.currentStatusPanel.innerHTML = `
        <div class="status-title-row">
            <strong>${isDanger ? 'Cần chú ý ngay' : 'Theo dõi bình thường'}</strong>
            <span>${escapeHtml(device.deviceId)}</span>
        </div>
        <p>${escapeHtml(message)}</p>
    `;
}

function renderHistoryTable(history = [], alerts = []) {
    if (!history.length) {
        els.historyTableBody.innerHTML = '<tr><td colspan="5" class="empty-row">Chưa có dữ liệu để hiển thị.</td></tr>';
        return;
    }

    const rows = [...history].slice(-10).reverse().map((item) => {
        const rowAlerts = [];
        if (item.heartRate > 120 || item.heartRate < 50) rowAlerts.push('Nhịp tim');
        if (item.spo2 < 90) rowAlerts.push('Nồng độ SpO₂');
        if (item.temperature > 38.5 || item.temperature < 35) rowAlerts.push('Nhiệt độ cơ thể');
        const status = rowAlerts.length ? 'Cảnh báo' : 'Ổn định';
        return `
            <tr>
                <td>${escapeHtml(formatDateTime(item.timestamp))}</td>
                <td>${escapeHtml(String(item.heartRate))} bpm</td>
                <td>${escapeHtml(String(item.spo2))} %</td>
                <td>${escapeHtml(String(item.temperature))} °C</td>
                <td><span class="table-badge ${rowAlerts.length ? 'danger' : 'normal'}">${status}</span></td>
            </tr>
        `;
    }).join('');

    els.historyTableBody.innerHTML = rows;
}

function clearDetail() {
    els.detailTitle.textContent = 'Chọn thiết bị để xem chi tiết';
    els.detailSubtitle.textContent = 'Chọn 1 thiết bị trong danh sách bên trái để xem chỉ số sức khỏe, cảnh báo và biểu đồ lịch sử.';
    els.infoDeviceIdHero.textContent = '--';
    els.heartRateValue.textContent = '--';
    els.spo2Value.textContent = '--';
    els.temperatureValue.textContent = '--';
    els.updatedAtValue.textContent = '--';
    els.infoDeviceId.textContent = '--';
    els.infoPatientName.textContent = '--';
    els.infoHistoryCount.textContent = '0';
    els.infoCreatedAt.textContent = '--';
    if (!isEditingDeviceForm()) {
        els.deviceForm.reset();
    }
    clearMetricStates();
    renderAlerts([]);
    renderStatusPanel(null, []);
    renderHistoryTable([]);
    updateChart(state.charts.heartRate, [], []);
    updateChart(state.charts.spo2, [], []);
    updateChart(state.charts.temperature, [], []);
}

function applyMetricAlerts(latest) {
    clearMetricStates();
    if (!latest) return;

    if (latest.heartRate > 120 || latest.heartRate < 50) {
        els.heartRateCard.classList.add('metric-alert');
    }
    if (latest.spo2 < 90) {
        els.spo2Card.classList.add('metric-alert');
    }
    if (latest.temperature > 38) {
        els.temperatureCard.classList.add('metric-alert');
    }
}

async function loadDevices(showInfo = false) {
    const data = await apiGet('devices');
    state.devices = Array.isArray(data.devices) ? data.devices : [];

    if (!state.selectedDeviceId && state.devices.length) {
        state.selectedDeviceId = state.devices[0].deviceId;
    }

    if (state.selectedDeviceId && !state.devices.some((item) => item.deviceId === state.selectedDeviceId)) {
        state.selectedDeviceId = state.devices.length ? state.devices[0].deviceId : null;
    }

    renderDeviceList();

    if (showInfo) {
        showMessage('Đã làm mới danh sách thiết bị', 'success');
    }

    if (state.selectedDeviceId) {
        await loadDeviceDetail(state.selectedDeviceId, false);
    } else {
        clearDetail();
    }
}

async function loadDeviceDetail(deviceId, scrollIntoView = false) {
    if (!deviceId) {
        clearDetail();
        return;
    }

    const data = await apiGet('device', `&deviceId=${encodeURIComponent(deviceId)}`);
    const device = data.device;
    const latest = device.latest || {};
    const history = Array.isArray(device.history) ? device.history : [];
    const alerts = Array.isArray(device.alerts) ? device.alerts : [];

    state.selectedDeviceId = device.deviceId;
    renderDeviceList();
    if (!isEditingDeviceForm()) {
        fillForm(device);
    }

    els.detailTitle.textContent = `${device.patientName || 'Chưa gán bệnh nhân'} · ${device.deviceId}`;
    els.detailSubtitle.textContent = alerts.length
        ? 'Thiết bị đang phát sinh cảnh báo. Hãy kiểm tra các chỉ số bên dưới để đánh giá nhanh.'
        : 'Thiết bị đang được theo dõi ổn định. Biểu đồ dưới đây thể hiện xu hướng dữ liệu theo thời gian.';
    els.infoDeviceIdHero.textContent = device.deviceId || '--';
    els.heartRateValue.textContent = latest.heartRate ?? '--';
    els.spo2Value.textContent = latest.spo2 ?? '--';
    els.temperatureValue.textContent = latest.temperature ?? '--';
    els.updatedAtValue.textContent = formatDateTime(latest.timestamp || device.updatedAt);
    els.infoDeviceId.textContent = device.deviceId || '--';
    els.infoPatientName.textContent = device.patientName || 'Chưa gán bệnh nhân';
    els.infoHistoryCount.textContent = String(history.length);
    els.infoCreatedAt.textContent = formatDateTime(device.createdAt);

    renderAlerts(alerts);
    renderStatusPanel(device, alerts);
    renderHistoryTable(history, alerts);
    applyMetricAlerts(latest);

    const labels = history.map((item) => formatDateTime(item.timestamp));
    updateChart(state.charts.heartRate, labels, history.map((item) => item.heartRate));
    updateChart(state.charts.spo2, labels, history.map((item) => item.spo2));
    updateChart(state.charts.temperature, labels, history.map((item) => item.temperature));

    if (scrollIntoView) {
        window.scrollTo({ top: 0, behavior: 'smooth' });
    }
}

async function selectDevice(deviceId) {
    try {
        await loadDeviceDetail(deviceId, false);
    } catch (error) {
        showMessage(error.message, 'error');
    }
}

async function handleFormSubmit(event) {
    event.preventDefault();
    const deviceId = els.deviceIdInput.value.trim();
    const patientName = els.patientNameInput.value.trim();

    if (!deviceId) {
        showMessage('Vui lòng nhập device ID', 'error');
        return;
    }

    try {
        await apiPost('upsert_patient', { deviceId, patientName });
        state.selectedDeviceId = deviceId;
        await loadDevices();
        showMessage('Đã lưu thông tin bệnh nhân', 'success');
    } catch (error) {
        showMessage(error.message, 'error');
    }
}

async function handleDelete() {
    const deviceId = els.deviceIdInput.value.trim() || state.selectedDeviceId;
    if (!deviceId) {
        showMessage('Hãy chọn hoặc nhập device ID cần xóa', 'error');
        return;
    }

    const confirmed = window.confirm(`Xóa thiết bị ${deviceId}? Toàn bộ lịch sử sẽ bị xóa.`);
    if (!confirmed) return;

    try {
        await apiPost('delete_device', { deviceId });
        if (state.selectedDeviceId === deviceId) {
            state.selectedDeviceId = null;
        }
        els.deviceForm.reset();
        await loadDevices();
        showMessage(`Đã xóa thiết bị ${deviceId}`, 'success');
    } catch (error) {
        showMessage(error.message, 'error');
    }
}

async function handleSeedDemo() {
    try {
        await apiPost('seed_demo', {});
        await loadDevices();
        showMessage('Đã tạo dữ liệu mẫu để bạn xem thử dashboard', 'success');
    } catch (error) {
        showMessage(error.message, 'error');
    }
}

function startPolling() {
    if (state.pollingHandle) {
        clearInterval(state.pollingHandle);
    }

    state.pollingHandle = setInterval(async () => {
        if (isEditingDeviceForm()) {
            return;
        }

        try {
            await loadDevices();
        } catch (error) {
            console.error(error);
        }
    }, 3000);
}

function bindEvents() {
    els.deviceForm.addEventListener('submit', handleFormSubmit);
    els.deleteBtn.addEventListener('click', handleDelete);
    els.refreshBtn.addEventListener('click', () => {
        loadDevices(true).catch((error) => showMessage(error.message, 'error'));
    });
    els.seedBtn.addEventListener('click', handleSeedDemo);
    els.deviceSearchInput.addEventListener('input', (event) => {
        state.deviceQuery = event.target.value || '';
        renderDeviceList();
    });
}

function disableDeviceFormAutocomplete() {
    const attrs = {
        autocomplete: 'new-password',
        autocapitalize: 'off',
        autocorrect: 'off',
        spellcheck: 'false',
    };

    [els.deviceIdInput, els.patientNameInput].forEach((input) => {
        if (!input) return;
        Object.entries(attrs).forEach(([name, value]) => {
            input.setAttribute(name, value);
        });
    });

    if (els.deviceForm) {
        els.deviceForm.setAttribute('autocomplete', 'off');
    }
}

function applyRolePermissions() {
    if (state.role === 'doctor') {
        return;
    }

    els.seedBtn.hidden = true;
    els.deleteBtn.hidden = true;
}

async function bootstrap() {
    initCharts();
    disableDeviceFormAutocomplete();
    applyRolePermissions();
    bindEvents();
    try {
        await loadDevices();
        startPolling();
    } catch (error) {
        showMessage(error.message, 'error');
    }
}

document.addEventListener('DOMContentLoaded', bootstrap);
