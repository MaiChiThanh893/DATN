(function () {
    function current() {
        return document.documentElement.getAttribute('data-theme') === 'light' ? 'light' : 'dark';
    }

    function updateLabels() {
        var isLight = current() === 'light';
        document.querySelectorAll('[data-theme-label]').forEach(function (el) {
            el.textContent = isLight ? 'Tối' : 'Sáng';
        });
        document.querySelectorAll('[data-theme-toggle]').forEach(function (b) {
            b.setAttribute('aria-pressed', isLight ? 'true' : 'false');
        });
    }

    function apply(theme) {
        var next = theme === 'light' ? 'light' : 'dark';
        document.documentElement.setAttribute('data-theme', next);
        try {
            localStorage.setItem('iot_dashboard_theme', next);
        } catch (e) {}
        updateLabels();
        window.dispatchEvent(new CustomEvent('themechange', { detail: next }));
    }

    function setupMobileNav() {
        var app = document.querySelector('.app');
        if (!app) return;

        var overlay = document.querySelector('.nav-overlay');
        if (!overlay) {
            overlay = document.createElement('div');
            overlay.className = 'nav-overlay';
            document.body.appendChild(overlay);
        }

        function isMobile() { return window.innerWidth <= 880; }
        function closeNav() { document.body.classList.remove('nav-open'); }

        document.querySelectorAll('[data-nav-toggle]').forEach(function (btn) {
            btn.addEventListener('click', function (e) {
                if (!isMobile()) return; // desktop: nhường cho hành vi thu gọn rail (app.js)
                e.preventDefault();
                e.stopPropagation();
                document.body.classList.toggle('nav-open');
            });
        });

        overlay.addEventListener('click', closeNav);

        // bấm 1 mục menu -> đóng drawer trên mobile
        document.querySelectorAll('.rail .nav-i').forEach(function (a) {
            a.addEventListener('click', function () { if (isMobile()) closeNav(); });
        });

        window.addEventListener('keydown', function (e) {
            if (e.key === 'Escape') closeNav();
        });

        function syncMode() {
            if (isMobile()) {
                app.classList.remove('rail-collapsed'); // tránh xung đột với chế độ desktop
            } else {
                closeNav();
            }
        }
        window.addEventListener('resize', syncMode);
        syncMode();
    }

    document.addEventListener('DOMContentLoaded', function () {
        updateLabels();
        document.querySelectorAll('[data-theme-toggle]').forEach(function (b) {
            b.addEventListener('click', function () {
                apply(current() === 'light' ? 'dark' : 'light');
            });
        });
        setupMobileNav();
    });
})();
