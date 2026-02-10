/**
 * Client-side chess clock display.
 */
var Boudica = window.Boudica || {};

Boudica.Clock = (function () {
    var remaining = { white: 300000, black: 300000 };
    var activeColor = null;
    var lastSync = 0;
    var intervalId = null;
    var LOW_TIME_MS = 30000;

    function start() {
        stop();
        lastSync = Date.now();
        intervalId = setInterval(_tick, 100);
    }

    function stop() {
        if (intervalId) {
            clearInterval(intervalId);
            intervalId = null;
        }
    }

    function syncTime(clocks) {
        remaining.white = clocks.white.remaining_ms;
        remaining.black = clocks.black.remaining_ms;
        lastSync = Date.now();
        _render();
    }

    function setActive(color) {
        activeColor = color;
        lastSync = Date.now();
        _render();
    }

    function _tick() {
        if (!activeColor) return;
        var elapsed = Date.now() - lastSync;
        var display = remaining[activeColor] - elapsed;
        if (display < 0) display = 0;
        _renderClock(activeColor, display);
    }

    function _render() {
        _renderClock('white', remaining.white);
        _renderClock('black', remaining.black);
    }

    function _renderClock(color, ms) {
        var el = _getClockEl(color);
        if (!el) return;

        el.textContent = _formatTime(ms);

        // Active state
        el.classList.toggle('active', activeColor === color);

        // Low time warning
        el.classList.toggle('low-time', ms < LOW_TIME_MS && activeColor === color);
    }

    function _getClockEl(color) {
        var playerColor = Boudica.Game ? Boudica.Game.getPlayerColor() : 'white';
        if (color === playerColor) {
            return document.getElementById('bottom-clock');
        }
        return document.getElementById('top-clock');
    }

    function _formatTime(ms) {
        if (ms <= 0) return '0:00';
        var totalSec = Math.ceil(ms / 1000);
        var min = Math.floor(totalSec / 60);
        var sec = totalSec % 60;
        return min + ':' + (sec < 10 ? '0' : '') + sec;
    }

    return {
        start: start,
        stop: stop,
        syncTime: syncTime,
        setActive: setActive
    };
})();
