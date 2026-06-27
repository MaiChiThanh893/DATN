const READ_KEY = 'iot_read_alerts';
const CHECK_SVG = '<svg class="icn" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/></svg>';

const els = {
    list: document.getElementById('alertsList'),
    statFlagged: document.getElementById('statFlagged'),
    statUnread: document.getElementById('statUnread'),
    statRead: document.getElementById('statRead'),
    markAll: document.getElementById('markAllReadBtn'),
    search: document.getElementById('alertSearch'),
    msg: document.getElementById('globalMessage'),
};

let readSet = loadRead();
let devices = [];
let query = '';

function loadRead() {
    try { return new Set(JSON.parse(localStorage.getItem(READ_KEY) || '[]')); } catch (e) { return new Set(); }
}
function saveRead() {
    try { localStorage.setItem(READ_KEY, JSON.stringify([...readSet])); } catch (e) {}
}
function esc(t) {
    return String(t).replaceAll('&', '&amp;').replaceAll('<', '&lt;').replaceAll('>', '&gt;').replaceAll('"', '&quot;');
}
function sig(d) {
    const a = Array.isArray(d.alerts) ? [...d.alerts].sort() : [];
    return `${d.deviceId}::${a.join('|')}`;
}
function isUnread(d) {
    return Array.isArray(d.alerts) && d.alerts.length > 0 && !readSet.has(sig(d));
}

async function fetchDevices() {
    const res = await fetch('api.php?action=devices');
    const data = await res.json();
    if (res.status === 401) {
        window.location.href = 'login.php?next=alerts.php';
        return [];
    }
    if (!res.ok || !data.success) throw new Error(data.message || 'API lỗi');
    return Array.isArray(data.devices) ? data.devices : [];
}

function render() {
    const flagged = devices.filter((d) => Array.isArray(d.alerts) && d.alerts.length > 0);
    const unreadCount = flagged.filter(isUnread).length;

    if (els.statFlagged) els.statFlagged.textContent = String(flagged.length);
    if (els.statUnread) els.statUnread.textContent = String(unreadCount);
    if (els.statRead) els.statRead.textContent = String(flagged.length - unreadCount);

    const q = query.trim().toLowerCase();
    const shown = flagged.filter((d) => !q || `${d.deviceId} ${d.patientName || ''}`.toLowerCase().includes(q));

    if (!shown.length) {
        els.list.innerHTML = `<div class="feed-empty">${q ? 'Không có cảnh báo khớp bộ lọc.' : 'Không có cảnh báo nào. Tất cả thiết bị đang ổn định.'}</div>`;
        return;
    }

    els.list.innerHTML = shown.map((d) => {
        const read = !isUnread(d);
        const s = esc(sig(d));
        const updated = d.updatedAt ? new Date(d.updatedAt).toLocaleString('vi-VN') : '';
        return `
            <div class="feed-i ${read ? 'read' : ''}">
                <div class="bar"></div>
                <div class="b" style="flex:1;min-width:0">
                    <b>${esc(d.patientName || 'Chưa gán bệnh nhân')} · ${esc(d.deviceId)}</b>
                    <small>${esc(d.alerts.join(' · '))}${updated ? ' — ' + esc(updated) : ''}</small>
                </div>
                ${read ? '<span class="st ok" style="align-self:center">Đã đọc</span>' : `<button class="mark" data-mark="${s}" type="button" title="Đánh dấu đã đọc">${CHECK_SVG}</button>`}
            </div>
        `;
    }).join('');

    els.list.querySelectorAll('[data-mark]').forEach((b) => {
        b.addEventListener('click', () => {
            readSet.add(b.dataset.mark);
            saveRead();
            render();
        });
    });
}

async function refresh() {
    try {
        devices = await fetchDevices();
        render();
    } catch (e) {
        if (els.msg) els.msg.innerHTML = `<div class="flash-message error">${esc(e.message)}</div>`;
    }
}

if (els.markAll) {
    els.markAll.addEventListener('click', () => {
        devices.filter((d) => Array.isArray(d.alerts) && d.alerts.length > 0).forEach((d) => readSet.add(sig(d)));
        saveRead();
        render();
    });
}
if (els.search) {
    els.search.addEventListener('input', (e) => { query = e.target.value || ''; render(); });
}

refresh();
setInterval(refresh, 5000);
