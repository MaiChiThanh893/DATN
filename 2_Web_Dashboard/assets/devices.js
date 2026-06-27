const API = 'api.php?action=';
const role = document.body.dataset.role || '';

const els = {
    table: document.getElementById('deviceTable'),
    count: document.getElementById('deviceCount'),
    search: document.getElementById('deviceSearch'),
    refresh: document.getElementById('refreshBtn'),
    seed: document.getElementById('seedBtn'),
    form: document.getElementById('deviceForm'),
    deviceId: document.getElementById('deviceIdInput'),
    patient: document.getElementById('patientNameInput'),
    reset: document.getElementById('resetBtn'),
    formTitle: document.getElementById('formTitle'),
    msg: document.getElementById('globalMessage'),
};

let devices = [];
let query = '';

function esc(t) {
    return String(t).replaceAll('&', '&amp;').replaceAll('<', '&lt;').replaceAll('>', '&gt;').replaceAll('"', '&quot;');
}
function metric(v) { return (v === undefined || v === null || v === '') ? '--' : v; }
function fmtHR(v) { const n = Number(v); return Number.isFinite(n) ? String(Math.round(n)) : '--'; }
function fmt1(v) { const n = Number(v); return Number.isFinite(n) ? n.toFixed(1) : '--'; }
function flash(text, type) {
    if (!els.msg) return;
    els.msg.innerHTML = `<div class="flash-message ${type || 'success'}">${esc(text)}</div>`;
    if (type !== 'error') setTimeout(() => { if (els.msg) els.msg.innerHTML = ''; }, 2600);
}

async function apiGet(action, q) {
    const res = await fetch(`${API}${action}${q || ''}`);
    const data = await res.json();
    if (res.status === 401) { window.location.href = 'login.php?next=devices.php'; throw new Error('Hết phiên'); }
    if (!res.ok || !data.success) throw new Error(data.message || 'API lỗi');
    return data;
}
async function apiPost(action, payload) {
    const body = new URLSearchParams();
    Object.entries(payload || {}).forEach(([k, v]) => { if (v !== undefined && v !== null) body.append(k, String(v)); });
    const res = await fetch(`${API}${action}`, { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded; charset=UTF-8' }, body: body.toString() });
    const data = await res.json();
    if (res.status === 401) { window.location.href = 'login.php?next=devices.php'; throw new Error('Hết phiên'); }
    if (!res.ok || !data.success) throw new Error(data.message || 'API lỗi');
    return data;
}

function severity(d) {
    const a = Array.isArray(d.alerts) ? d.alerts : [];
    if (a.length) return 'danger';
    const l = d.latest || {};
    const hr = Number(l.heartRate), sp = Number(l.spo2), t = Number(l.temperature);
    if ((Number.isFinite(hr) && (hr >= 110 || hr <= 55)) || (Number.isFinite(sp) && sp <= 92) || (Number.isFinite(t) && t >= 37.8)) return 'warn';
    return 'ok';
}
const LABEL = { ok: 'Ổn định', warn: 'Chú ý', danger: 'Nguy hiểm' };

function render() {
    const q = query.trim().toLowerCase();
    const shown = devices.filter((d) => !q || `${d.deviceId} ${d.patientName || ''}`.toLowerCase().includes(q));
    if (els.count) els.count.textContent = String(devices.length);

    if (!shown.length) {
        els.table.innerHTML = `<tr><td colspan="7" class="empty-row">${q ? 'Không tìm thấy thiết bị.' : 'Chưa có thiết bị nào.'}</td></tr>`;
        return;
    }

    const canDelete = role === 'doctor';
    els.table.innerHTML = shown.map((d) => {
        const l = d.latest || {};
        const sev = severity(d);
        return `
            <tr>
                <td class="who"><b>${esc(d.patientName || 'Chưa gán bệnh nhân')}</b><small>${esc(d.deviceId)}</small></td>
                <td class="val">${l.heartRate === undefined || l.heartRate === null ? '--' : fmtHR(l.heartRate)}</td>
                <td class="val">${metric(l.spo2)}</td>
                <td class="val">${l.temperature === undefined || l.temperature === null ? '--' : fmt1(l.temperature)}</td>
                <td class="val">${l.humidity === undefined || l.humidity === null ? '--' : fmt1(l.humidity)}</td>
                <td><span class="st ${sev}">${LABEL[sev]}</span></td>
                <td style="white-space:nowrap;text-align:right">
                    <button class="btn ghost" data-edit="${esc(d.deviceId)}" type="button">Sửa</button>
                    ${canDelete ? `<button class="btn danger" data-del="${esc(d.deviceId)}" type="button">Xóa</button>` : ''}
                </td>
            </tr>
        `;
    }).join('');

    els.table.querySelectorAll('[data-edit]').forEach((b) => b.addEventListener('click', () => fillForm(b.dataset.edit)));
    els.table.querySelectorAll('[data-del]').forEach((b) => b.addEventListener('click', () => removeDevice(b.dataset.del)));
}

function fillForm(deviceId) {
    const d = devices.find((x) => x.deviceId === deviceId);
    if (!d) return;
    els.deviceId.value = d.deviceId;
    els.patient.value = d.patientName || '';
    els.formTitle.textContent = 'Sửa thiết bị';
    els.patient.focus();
}
function resetForm() {
    els.form.reset();
    els.formTitle.textContent = 'Thêm thiết bị';
}

async function refresh() {
    try {
        const data = await apiGet('devices');
        devices = Array.isArray(data.devices) ? data.devices : [];
        render();
    } catch (e) { flash(e.message, 'error'); }
}

async function removeDevice(deviceId) {
    if (!window.confirm(`Xóa thiết bị ${deviceId}? Toàn bộ lịch sử sẽ mất.`)) return;
    try {
        await apiPost('delete_device', { deviceId });
        flash(`Đã xóa thiết bị ${deviceId}`);
        resetForm();
        await refresh();
    } catch (e) { flash(e.message, 'error'); }
}

if (els.form) {
    els.form.addEventListener('submit', async (event) => {
        event.preventDefault();
        const deviceId = els.deviceId.value.trim();
        const patientName = els.patient.value.trim();
        if (!deviceId) { flash('Vui lòng nhập Device ID', 'error'); return; }
        try {
            await apiPost('upsert_patient', { deviceId, patientName });
            flash('Đã lưu thông tin thiết bị');
            resetForm();
            await refresh();
        } catch (e) { flash(e.message, 'error'); }
    });
}
if (els.reset) els.reset.addEventListener('click', resetForm);
if (els.refresh) els.refresh.addEventListener('click', refresh);
if (els.search) els.search.addEventListener('input', (e) => { query = e.target.value || ''; render(); });
if (els.seed) {
    if (role !== 'doctor') { els.seed.hidden = true; }
    els.seed.addEventListener('click', async () => {
        try { await apiPost('seed_demo', {}); flash('Đã tạo dữ liệu mẫu'); await refresh(); }
        catch (e) { flash(e.message, 'error'); }
    });
}

refresh();
setInterval(refresh, 5000);
