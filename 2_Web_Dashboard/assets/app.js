const API_BASE = 'api.php?action=';

const ICON = {
    warning: '<svg class="icn" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M1 21h22L12 2 1 21zm12-3h-2v-2h2v2zm0-4h-2v-4h2v4z"/></svg>',
    check: '<svg class="icn" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/></svg>',
};

const state = {
    devices: [],
    selectedDeviceId: null,
    deviceQuery: '',
    charts: {},
    pollingHandle: null,
    role: document.body.dataset.role || '',
    alertedIds: null,
    chartView: 'all',
    chartRange: '200',
    currentHistory: [],
    readAlerts: new Set(),
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
    summaryAvgTemp: document.getElementById('summaryAvgTemp'),
    summaryAvgHum: document.getElementById('summaryAvgHum'),
    alertBell: document.getElementById('alertBell'),
    alertBellCount: document.getElementById('alertBellCount'),
    alertPanel: document.getElementById('alertPanel'),
    toastHost: document.getElementById('toastHost'),
    app: document.querySelector('.app'),
    sidebarToggle: document.getElementById('sidebarToggle'),
    devPickerBtn: document.getElementById('devPickerBtn'),
    devPickerLabel: document.getElementById('devPickerLabel'),
    devDrawer: document.getElementById('devDrawer'),
    devDrawerOverlay: document.getElementById('devDrawerOverlay'),
    devDrawerClose: document.getElementById('devDrawerClose'),
    alertsView: document.getElementById('alertsView'),
    alertsViewList: document.getElementById('alertsViewList'),
    markAllReadBtn: document.getElementById('markAllReadBtn'),
    closeAlertsViewBtn: document.getElementById('closeAlertsViewBtn'),
    navAlerts: document.getElementById('navAlerts'),
    heartRateCard: document.getElementById('heartRateCard'),
    spo2Card: document.getElementById('spo2Card'),
    temperatureCard: document.getElementById('temperatureCard'),
    humidityValue: document.getElementById('humidityValue'),
    humidityCard: document.getElementById('humidityCard'),
    sosBanner: document.getElementById('sosBanner'),
    themeToggle: document.querySelector('[data-theme-toggle]'),
    themeLabel: document.querySelector('[data-theme-label]'),
    zoomInBtn: document.getElementById('zoomInBtn'),
    zoomOutBtn: document.getElementById('zoomOutBtn'),
    zoomResetBtn: document.getElementById('zoomResetBtn'),
    zoomLabel: document.getElementById('zoomLabel'),
};

const ZOOM_MIN = 10;
const ZOOM_DEFAULT = '200';
const ZOOM_FACTOR = 1.6;

function formatDateTime(value) {
    if (!value) return '--';
    const date = new Date(value);
    if (Number.isNaN(date.getTime())) return value;
    return date.toLocaleString('vi-VN');
}

function formatChartTime(value) {
    const date = new Date(value);
    if (Number.isNaN(date.getTime())) return value || '';
    return date.toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit' });
}

// Nhịp tim: làm tròn về số nguyên (.5 làm tròn lên)
function fmtHR(value) {
    const n = Number(value);
    return Number.isFinite(n) ? String(Math.round(n)) : '--';
}

// Nhiệt độ / Độ ẩm: luôn 1 chữ số thập phân, kể cả 10.0
function fmt1(value) {
    const n = Number(value);
    return Number.isFinite(n) ? n.toFixed(1) : '--';
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

function chartPalette() {
    const isLight = document.documentElement.getAttribute('data-theme') === 'light';
    return {
        legend: isLight ? '#3a4a6b' : '#c9d5f2',
        ticks: isLight ? '#5a6b8c' : '#8d9bb8',
        grid: isLight ? 'rgba(20,40,80,0.08)' : 'rgba(255,255,255,0.06)',
    };
}

function createChart(canvasId, label, color, yTitle) {
    const ctx = document.getElementById(canvasId).getContext('2d');
    const palette = chartPalette();
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
                        color: palette.legend,
                        boxWidth: 10,
                    },
                },
            },
            scales: {
                x: {
                    ticks: {
                        color: palette.ticks,
                        maxTicksLimit: 7,
                    },
                    grid: {
                        color: palette.grid,
                    },
                },
                y: {
                    ticks: {
                        color: palette.ticks,
                    },
                    title: {
                        display: true,
                        text: yTitle,
                        color: palette.ticks,
                    },
                    grid: {
                        color: palette.grid,
                    },
                },
            },
        },
    });
}

// Key lưu range riêng cho mobile vs desktop để mobile luôn mặc định ít mẫu
function chartRangeKey() {
    return window.innerWidth <= 560 ? 'iot_chart_range_m' : 'iot_chart_range';
}

function initCharts() {
    state.charts.main = createMainChart('mainChart');
    setChartView(state.chartView || 'all');
    const defaultRange = window.innerWidth <= 560 ? '50' : '200'; // mobile: ít mẫu nhất cho dễ xem
    let saved = defaultRange;
    try { saved = localStorage.getItem(chartRangeKey()) || defaultRange; } catch (e) {}
    setChartRange(saved);
}

function createMainChart(canvasId) {
    const canvas = document.getElementById(canvasId);
    if (!canvas) return null;
    const p = chartPalette();
    const mk = (label, color, axis) => ({
        label,
        data: [],
        borderColor: color,
        backgroundColor: color + '22',
        yAxisID: axis,
        borderWidth: 2.4,
        pointRadius: 0,
        pointHoverRadius: 4,
        tension: 0.32,
        fill: false,
    });
    return new Chart(canvas.getContext('2d'), {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                mk('Nhịp tim (bpm)', '#2563eb', 'yHr'),
                mk('SpO₂ (%)', '#0f9d6b', 'ySpo2'),
                mk('Nhiệt độ (°C)', '#dc2626', 'yTemp'),
                mk('Độ ẩm (%)', '#0891b2', 'yHum'),
            ],
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            interaction: { mode: 'index', intersect: false },
            plugins: {
                legend: { labels: { color: p.legend, boxWidth: 12, usePointStyle: true, padding: 16 } },
                tooltip: { enabled: true, callbacks: {
                    title: (items) => {
                        const it = items && items[0];
                        if (!it) return '';
                        const full = it.chart._fullLabels;
                        return (full && full[it.dataIndex]) || it.label;
                    },
                    label: (item) => {
                        const lbl = item.dataset.label || '';
                        const v = item.parsed.y;
                        if (v === null || v === undefined || Number.isNaN(v)) return `${lbl}: --`;
                        let val;
                        if (item.datasetIndex === 0) val = String(Math.round(v));        // Nhịp tim -> số nguyên
                        else if (item.datasetIndex === 2 || item.datasetIndex === 3) val = Number(v).toFixed(1); // Nhiệt độ, Độ ẩm -> 1 thập phân
                        else val = String(v);                                            // SpO2 -> nguyên gốc
                        return `${lbl}: ${val}`;
                    },
                } },
            },
            scales: {
                x: { ticks: { color: p.ticks, maxTicksLimit: 8 }, grid: { color: p.grid } },
                yHr: { position: 'left', min: 20, max: 200, ticks: { color: p.ticks }, grid: { color: p.grid }, title: { display: true, text: 'bpm', color: p.ticks } },
                ySpo2: { position: 'left', min: 10, max: 100, ticks: { color: p.ticks }, grid: { color: p.grid }, title: { display: true, text: '%', color: p.ticks } },
                yTemp: { position: 'left', display: false, ticks: { color: p.ticks }, grid: { color: p.grid }, title: { display: true, text: '°C', color: p.ticks } },
                yHum: { position: 'left', display: false, ticks: { color: p.ticks }, grid: { color: p.grid }, title: { display: true, text: '%', color: p.ticks } },
            },
        },
    });
}

function setChartView(view) {
    state.chartView = view;
    document.querySelectorAll('.ctab').forEach((b) => b.classList.toggle('active', b.dataset.view === view));

    const chart = state.charts.main;
    if (!chart) return;
    const ds = chart.data.datasets;
    const sc = chart.options.scales;
    const homeAxis = ['yHr', 'ySpo2', 'yTemp', 'yHum'];

    if (view === 'all') {
        // Tổng hợp: tất cả line dùng chung 1 trục trái, max động (tính trong applyChartRange)
        ds.forEach((d) => { d.hidden = false; d.yAxisID = 'yHr'; });
        sc.yHr.display = true;
        sc.yHr.title.display = false; // bỏ chữ 'bpm' ở trục chung
        sc.ySpo2.display = false;
        sc.yTemp.display = false;
        sc.yHum.display = false;
    } else {
        // View đơn: mỗi line về trục riêng của nó (đều nằm bên trái), chỉ hiện trục tương ứng
        const idx = { hr: 0, spo2: 1, temp: 2, hum: 3 }[view] ?? 0;
        ds.forEach((d, i) => { d.hidden = i !== idx; d.yAxisID = homeAxis[i]; });
        sc.yHr.display = idx === 0;
        sc.yHr.title.display = true;
        sc.yHr.min = 20; sc.yHr.max = 200; // chart Nhịp tim riêng: cố định 20–200
        sc.ySpo2.display = idx === 1;
        sc.yTemp.display = idx === 2;
        sc.yHum.display = idx === 3;
    }
    applyChartRange(); // áp dữ liệu + tính lại max động cho view tổng hợp
}

function refreshChartsTheme() {
    const palette = chartPalette();
    Object.values(state.charts).forEach((chart) => {
        if (!chart) return;
        if (chart.options.plugins && chart.options.plugins.legend) {
            chart.options.plugins.legend.labels.color = palette.legend;
        }
        Object.values(chart.options.scales || {}).forEach((scale) => {
            if (scale.ticks) scale.ticks.color = palette.ticks;
            if (scale.grid) scale.grid.color = palette.grid;
            if (scale.title) scale.title.color = palette.ticks;
        });
        chart.update();
    });
}

function updateChart(chart, labels, values) {
    chart.data.labels = labels;
    chart.data.datasets[0].data = values;
    chart.update();
}

function setChartRange(range) {
    state.chartRange = range;
    document.querySelectorAll('.rtab').forEach((b) => b.classList.toggle('active', b.dataset.range === String(range)));
    try { localStorage.setItem(chartRangeKey(), String(range)); } catch (e) {}
    applyChartRange();
}

function zoomChart(direction) {
    const total = (state.currentHistory || []).length;
    const current = state.chartRange === 'all' ? (total || Number(ZOOM_DEFAULT)) : Number(state.chartRange);
    // direction < 1 = zoom in (ít mẫu hơn, chi tiết hơn); > 1 = zoom out (nhiều mẫu hơn, tổng quát hơn)
    let next = Math.round(current * direction);

    if (direction > 1) {
        if (total && next >= total) { setChartRange('all'); return; }
        next = Math.min(next, total || 2000);
    } else {
        next = Math.max(ZOOM_MIN, next);
    }
    setChartRange(String(next));
}

function updateZoomUI() {
    const total = (state.currentHistory || []).length;
    const isAll = state.chartRange === 'all';
    const shown = isAll ? total : Math.min(Number(state.chartRange), total || Number(state.chartRange));

    if (els.zoomLabel) {
        els.zoomLabel.textContent = isAll ? 'Tất cả' : String(shown);
    }
    if (els.zoomInBtn) els.zoomInBtn.disabled = !isAll && Number(state.chartRange) <= ZOOM_MIN;
    if (els.zoomOutBtn) els.zoomOutBtn.disabled = isAll || (total > 0 && shown >= total);
}

function applyChartRange() {
    const chart = state.charts.main;
    if (!chart) return;
    const history = state.currentHistory || [];
    const sliced = state.chartRange === 'all' ? history : history.slice(-Number(state.chartRange));
    chart.data.labels = sliced.map((item) => formatChartTime(item.timestamp));
    chart._fullLabels = sliced.map((item) => formatDateTime(item.timestamp));
    chart.data.datasets[0].data = sliced.map((item) => item.heartRate);
    chart.data.datasets[1].data = sliced.map((item) => item.spo2);
    chart.data.datasets[2].data = sliced.map((item) => item.temperature);
    chart.data.datasets[3].data = sliced.map((item) => item.humidity);

    // View Tổng hợp: max trục trái bám theo max hiện tại của các line (trần 200), min 20
    if (state.chartView === 'all') {
        let mx = 0;
        sliced.forEach((item) => {
            [item.heartRate, item.spo2, item.temperature, item.humidity].forEach((v) => {
                const n = Number(v);
                if (Number.isFinite(n) && n > mx) mx = n;
            });
        });
        chart.options.scales.yHr.min = 20;
        chart.options.scales.yHr.max = mx > 0 ? Math.min(200, Math.ceil((mx + 5) / 10) * 10) : 200;
    }

    chart.update();
    updateZoomUI();
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
    const tempValues = latestReadings.map((item) => Number(item.temperature)).filter((value) => Number.isFinite(value));
    const humValues = latestReadings.map((item) => Number(item.humidity)).filter((value) => Number.isFinite(value));
    const alertCount = devices.filter((device) => Array.isArray(device.alerts) && device.alerts.length > 0).length;

    els.summaryTotalDevices.textContent = String(devices.length);
    els.summaryAlertDevices.textContent = String(alertCount);
    const avgHeart = average(heartRates);
    const avgSpo2 = average(spo2Values);
    const avgTemp = average(tempValues);
    const avgHum = average(humValues);
    els.summaryAvgHeartRate.textContent = avgHeart === null ? '--' : `${Math.round(avgHeart)} bpm`;
    els.summaryAvgSpo2.textContent = avgSpo2 === null ? '--' : `${avgSpo2.toFixed(1)} %`;
    if (els.summaryAvgTemp) els.summaryAvgTemp.textContent = avgTemp === null ? '--' : `${avgTemp.toFixed(1)} °C`;
    if (els.summaryAvgHum) els.summaryAvgHum.textContent = avgHum === null ? '--' : `${avgHum.toFixed(1)} %`;

    processAlerts();
}

function loadReadAlerts() {
    try { return new Set(JSON.parse(localStorage.getItem('iot_read_alerts') || '[]')); } catch (e) { return new Set(); }
}
function saveReadAlerts() {
    try { localStorage.setItem('iot_read_alerts', JSON.stringify([...state.readAlerts])); } catch (e) {}
}
function alertSig(device) {
    const alerts = Array.isArray(device.alerts) ? [...device.alerts].sort() : [];
    return `${device.deviceId}::${alerts.join('|')}`;
}
function isUnread(device) {
    return Array.isArray(device.alerts) && device.alerts.length > 0 && !state.readAlerts.has(alertSig(device));
}

function processAlerts() {
    const flagged = state.devices.filter((device) => Array.isArray(device.alerts) && device.alerts.length > 0);
    const unread = flagged.filter(isUnread);

    renderAlertPanel(flagged, unread);

    if (state.alertedIds === null) {
        state.alertedIds = new Set(flagged.map(alertSig));
        return;
    }
    flagged.forEach((device) => {
        const sig = alertSig(device);
        if (!state.alertedIds.has(sig) && isUnread(device)) {
            showToast(device);
        }
    });
    state.alertedIds = new Set(flagged.map(alertSig));
}

function alertItemHTML(device) {
    const read = !isUnread(device);
    const sig = escapeHtml(alertSig(device));
    return `
        <div class="feed-i ${read ? 'read' : ''}">
            <div class="bar"></div>
            <button class="b" data-device-id="${escapeHtml(device.deviceId)}" type="button" style="border:0;background:none;text-align:left;flex:1;min-width:0;cursor:pointer">
                <b>${escapeHtml(device.patientName || 'Chưa gán bệnh nhân')} · ${escapeHtml(device.deviceId)}</b>
                <small>${escapeHtml(device.alerts.join(' · '))}</small>
            </button>
            ${read ? '' : `<button class="mark" data-mark="${sig}" type="button" title="Đánh dấu đã đọc">${ICON.check}</button>`}
        </div>
    `;
}

function bindAlertItems(container) {
    container.querySelectorAll('[data-device-id]').forEach((item) => {
        item.addEventListener('click', (event) => {
            event.stopPropagation();
            selectDevice(item.dataset.deviceId);
            closeAlertPanel();
        });
    });
    container.querySelectorAll('[data-mark]').forEach((btn) => {
        btn.addEventListener('click', (event) => {
            event.stopPropagation();
            markRead(btn.dataset.mark);
        });
    });
}

function renderAlertPanel(flagged, unread) {
    if (els.alertBellCount) {
        els.alertBellCount.textContent = String(unread.length);
        els.alertBellCount.hidden = unread.length === 0;
    }
    if (els.alertBell) {
        els.alertBell.classList.toggle('has-alerts', unread.length > 0);
    }
    if (!els.alertPanel) return;

    const body = flagged.length
        ? flagged.map(alertItemHTML).join('')
        : '<div class="feed-empty">Không có cảnh báo. Tất cả thiết bị đang ổn định.</div>';

    els.alertPanel.innerHTML =
        `<div class="alert-panel-h">Chưa đọc (${unread.length}) · Tổng ${flagged.length}</div>` +
        `<div class="feed">${body}</div>` +
        `<div class="alert-panel-foot"><button class="btn ghost" data-act="mark-all" type="button">Đã đọc tất cả</button><button class="btn" data-act="view-all" type="button">Xem tất cả</button></div>`;

    bindAlertItems(els.alertPanel);
    const mAll = els.alertPanel.querySelector('[data-act="mark-all"]');
    const vAll = els.alertPanel.querySelector('[data-act="view-all"]');
    if (mAll) mAll.addEventListener('click', (e) => { e.stopPropagation(); markAllRead(); });
    if (vAll) vAll.addEventListener('click', (e) => { e.stopPropagation(); openAlertsView(); });
}

function markRead(sig) {
    state.readAlerts.add(sig);
    saveReadAlerts();
    processAlerts();
}
function markAllRead() {
    state.devices.filter((d) => Array.isArray(d.alerts) && d.alerts.length > 0).forEach((d) => state.readAlerts.add(alertSig(d)));
    saveReadAlerts();
    processAlerts();
}
function openAlertsView() {
    window.location.href = 'alerts.php';
}

function toastsEnabled() {
    try { return localStorage.getItem('iot_settings_toasts') !== '0'; } catch (e) { return true; }
}

function showToast(device) {
    if (!els.toastHost || !device || !toastsEnabled()) return;
    const el = document.createElement('div');
    el.className = 'toast danger';
    el.innerHTML = `
        <div class="ic">${ICON.warning}</div>
        <div class="b">
            <b>${escapeHtml(device.patientName || 'Chưa gán bệnh nhân')} · ${escapeHtml(device.deviceId)}</b>
            <small>${escapeHtml((device.alerts || []).join(' · '))}</small>
        </div>
    `;
    el.addEventListener('click', () => { selectDevice(device.deviceId); el.remove(); });
    while (els.toastHost.children.length >= 4) {
        els.toastHost.removeChild(els.toastHost.firstChild);
    }
    els.toastHost.appendChild(el);
    setTimeout(() => {
        el.classList.add('out');
        setTimeout(() => el.remove(), 300);
    }, 6000);
}

function closeAlertPanel() {
    if (els.alertPanel) els.alertPanel.hidden = true;
}

function toggleAlertPanel() {
    if (!els.alertPanel) return;
    els.alertPanel.hidden = !els.alertPanel.hidden;
}

function deviceSeverity(device) {
    const alerts = Array.isArray(device.alerts) ? device.alerts : [];
    if (alerts.length) return 'danger';
    const l = getLatestReading(device);
    const hr = Number(l.heartRate), sp = Number(l.spo2), t = Number(l.temperature);
    if ((Number.isFinite(hr) && (hr >= 110 || hr <= 55))
        || (Number.isFinite(sp) && sp <= 92)
        || (Number.isFinite(t) && t >= 37.8)) {
        return 'warn';
    }
    return 'ok';
}

const SEVERITY_LABEL = { ok: 'Ổn định', warn: 'Chú ý', danger: 'Nguy hiểm' };

function cellClass(value, bad, warn) {
    const n = Number(value);
    if (!Number.isFinite(n)) return '';
    if (bad(n)) return ' bad';
    if (warn(n)) return ' warnv';
    return '';
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
        els.deviceList.innerHTML = `<div class="feed-empty">${query ? 'Không tìm thấy thiết bị phù hợp.' : 'Chưa có thiết bị nào.'}</div>`;
        return;
    }

    els.deviceList.innerHTML = devices.map((device) => {
        const latest = getLatestReading(device);
        const isActive = device.deviceId === state.selectedDeviceId;
        const sev = deviceSeverity(device);
        const hrC = cellClass(latest.heartRate, (n) => n > 120 || n < 50, (n) => n >= 110 || n <= 55);
        return `
            <button class="devrow device-item ${isActive ? 'active' : ''}" data-device-id="${escapeHtml(device.deviceId)}" type="button" title="${SEVERITY_LABEL[sev]}">
                <span class="sdot ${sev}"></span>
                <span class="di"><b>${escapeHtml(device.patientName || 'Chưa gán bệnh nhân')}</b><small>${escapeHtml(device.deviceId)}</small></span>
                <span class="dhr${hrC}">${Number.isFinite(Number(latest.heartRate)) ? fmtHR(latest.heartRate) : safeMetric(latest.heartRate)}</span>
            </button>
        `;
    }).join('');

    els.deviceList.querySelectorAll('.device-item').forEach((item) => {
        item.addEventListener('click', () => {
            selectDevice(item.dataset.deviceId);
            closeDrawer();
        });
    });
}

function clearMetricStates() {
    [els.heartRateCard, els.spo2Card, els.temperatureCard, els.humidityCard].forEach((card) => card && card.classList.remove('metric-alert'));
}

function renderAlerts(alerts) {
    if (!els.alertsContainer) return;
    if (!alerts || !alerts.length) {
        els.alertsContainer.innerHTML = '';
        return;
    }

    els.alertsContainer.innerHTML = `
        <div class="alerts-stack">
            ${alerts.map((alert) => `<div class="alert-banner${alert.indexOf('SOS') !== -1 ? ' sos' : ''}">${ICON.warning} ${escapeHtml(alert)}</div>`).join('')}
        </div>
    `;
}

function fillForm(device) {
    if (!els.deviceIdInput || !els.patientNameInput) return;
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
        const hasHum = item.humidity !== null && item.humidity !== undefined && item.humidity !== '';
        const d = new Date(item.timestamp);
        const okD = !Number.isNaN(d.getTime());
        const tTime = okD ? d.toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit', second: '2-digit' }) : escapeHtml(String(item.timestamp));
        const tDate = okD ? d.toLocaleDateString('vi-VN') : '';
        return `
            <tr>
                <td>${tTime}${tDate ? ` <span class="cell-date">${tDate}</span>` : ''}</td>
                <td>${fmtHR(item.heartRate)} <span class="unit">bpm</span></td>
                <td>${escapeHtml(String(item.spo2))} <span class="unit">%</span></td>
                <td>${fmt1(item.temperature)} <span class="unit">°C</span></td>
                <td>${hasHum ? fmt1(item.humidity) + ' <span class="unit">%</span>' : '--'}</td>
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
    if (els.humidityValue) els.humidityValue.textContent = '--';
    if (els.sosBanner) els.sosBanner.hidden = true;
    els.updatedAtValue.textContent = '--';
    els.infoDeviceId.textContent = '--';
    els.infoPatientName.textContent = '--';
    els.infoHistoryCount.textContent = '0';
    els.infoCreatedAt.textContent = '--';
    if (els.deviceForm && !isEditingDeviceForm()) {
        els.deviceForm.reset();
    }
    clearMetricStates();
    renderAlerts([]);
    renderStatusPanel(null, []);
    renderHistoryTable([]);
    state.currentHistory = [];
    applyChartRange();
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
    if (els.humidityCard && latest.humidity !== null && latest.humidity !== undefined && (latest.humidity < 30 || latest.humidity > 70)) {
        els.humidityCard.classList.add('metric-alert');
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
    if (els.devPickerLabel) els.devPickerLabel.textContent = device.patientName || device.deviceId;
    els.detailSubtitle.textContent = alerts.length
        ? 'Thiết bị đang phát sinh cảnh báo. Hãy kiểm tra các chỉ số bên dưới để đánh giá nhanh.'
        : 'Thiết bị đang được theo dõi ổn định. Biểu đồ dưới đây thể hiện xu hướng dữ liệu theo thời gian.';
    els.infoDeviceIdHero.textContent = device.deviceId || '--';
    els.heartRateValue.textContent = fmtHR(latest.heartRate);
    els.spo2Value.textContent = latest.spo2 ?? '--';
    els.temperatureValue.textContent = fmt1(latest.temperature);
    if (els.humidityValue) els.humidityValue.textContent = fmt1(latest.humidity);
    if (els.sosBanner) {
        const sosOn = !!(latest && (Number(latest.sosStatus) === 1 || latest.sosStatus === true));
        els.sosBanner.hidden = !sosOn;
        if (sosOn) els.sosBanner.innerHTML = `${ICON.warning} <b>SOS</b> — Thiết bị đang gọi khẩn cấp! Cần hỗ trợ ngay.`;
    }
    els.updatedAtValue.textContent = formatDateTime(latest.timestamp || device.updatedAt);
    els.infoDeviceId.textContent = device.deviceId || '--';
    els.infoPatientName.textContent = device.patientName || 'Chưa gán bệnh nhân';
    els.infoHistoryCount.textContent = String(history.length);
    els.infoCreatedAt.textContent = formatDateTime(device.createdAt);

    renderAlerts(alerts);
    renderStatusPanel(device, alerts);
    renderHistoryTable(history, alerts);
    applyMetricAlerts(latest);

    state.currentHistory = history;
    applyChartRange();

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
    if (els.deviceForm) els.deviceForm.addEventListener('submit', handleFormSubmit);
    if (els.deleteBtn) els.deleteBtn.addEventListener('click', handleDelete);
    if (els.refreshBtn) els.refreshBtn.addEventListener('click', () => {
        loadDevices(true).catch((error) => showMessage(error.message, 'error'));
    });
    if (els.seedBtn) els.seedBtn.addEventListener('click', handleSeedDemo);
    if (els.deviceSearchInput) els.deviceSearchInput.addEventListener('input', (event) => {
        state.deviceQuery = event.target.value || '';
        renderDeviceList();
    });

    document.querySelectorAll('.ctab').forEach((b) => {
        b.addEventListener('click', () => setChartView(b.dataset.view));
    });
    document.querySelectorAll('.rtab').forEach((b) => {
        b.addEventListener('click', () => setChartRange(b.dataset.range));
    });
    if (els.zoomInBtn) els.zoomInBtn.addEventListener('click', () => zoomChart(1 / ZOOM_FACTOR));
    if (els.zoomOutBtn) els.zoomOutBtn.addEventListener('click', () => zoomChart(ZOOM_FACTOR));
    if (els.zoomResetBtn) els.zoomResetBtn.addEventListener('click', () => setChartRange(ZOOM_DEFAULT));
    if (els.devPickerBtn) els.devPickerBtn.addEventListener('click', openDrawer);
    if (els.devDrawerClose) els.devDrawerClose.addEventListener('click', closeDrawer);
    if (els.devDrawerOverlay) els.devDrawerOverlay.addEventListener('click', closeDrawer);

    if (els.alertBell) {
        els.alertBell.addEventListener('click', (event) => {
            event.stopPropagation();
            toggleAlertPanel();
        });
    }
    if (els.alertPanel) {
        els.alertPanel.addEventListener('click', (event) => event.stopPropagation());
    }
    document.addEventListener('click', closeAlertPanel);
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

    if (els.seedBtn) els.seedBtn.hidden = true;
    if (els.deleteBtn) els.deleteBtn.hidden = true;
}

function applyThemeLabel() {
    const isLight = document.documentElement.getAttribute('data-theme') === 'light';
    if (els.themeLabel) {
        els.themeLabel.textContent = isLight ? 'Dark mode' : 'Light mode';
    }
    if (els.themeToggle) {
        els.themeToggle.setAttribute('aria-pressed', isLight ? 'true' : 'false');
    }
}

function toggleTheme() {
    const isLight = document.documentElement.getAttribute('data-theme') === 'light';
    const next = isLight ? 'dark' : 'light';
    document.documentElement.setAttribute('data-theme', next);
    try {
        localStorage.setItem('iot_dashboard_theme', next);
    } catch (error) {
        /* localStorage không khả dụng, bỏ qua */
    }
    applyThemeLabel();
    refreshChartsTheme();
}

function initTheme() {
    window.addEventListener('themechange', refreshChartsTheme);
}

function openDrawer() {
    if (els.devDrawer) els.devDrawer.classList.add('open');
    if (els.devDrawerOverlay) els.devDrawerOverlay.classList.add('show');
}
function closeDrawer() {
    if (els.devDrawer) els.devDrawer.classList.remove('open');
    if (els.devDrawerOverlay) els.devDrawerOverlay.classList.remove('show');
}

function initSidebar() {
    if (!els.app || !els.sidebarToggle) return;
    let collapsed = false;
    try { collapsed = localStorage.getItem('iot_sidebar_collapsed') === '1'; } catch (e) {}
    els.app.classList.toggle('rail-collapsed', collapsed && window.innerWidth > 880);
    els.sidebarToggle.addEventListener('click', () => {
        if (window.innerWidth <= 880) return; // mobile: hamburger mở off-canvas nav (theme.js)
        const next = !els.app.classList.contains('rail-collapsed');
        els.app.classList.toggle('rail-collapsed', next);
        try { localStorage.setItem('iot_sidebar_collapsed', next ? '1' : '0'); } catch (e) {}
        setTimeout(() => {
            Object.values(state.charts).forEach((chart) => chart && chart.resize());
        }, 200);
    });
}

async function bootstrap() {
    state.readAlerts = loadReadAlerts();
    initCharts();
    initTheme();
    initSidebar();
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
