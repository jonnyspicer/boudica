/**
 * Client-side chess clock display. The running clock is shown in <mark>.
 */
var Boudica = window.Boudica || {};

Boudica.Clock = (function () {
    var remaining = { white: 300000, black: 300000 };
    var unlimited = false;
    var activeColor = null;
    var lastSync = 0;
    var intervalId = null;

    function start() {
        stop();
        if (unlimited) return;
        lastSync = Date.now();
        intervalId = setInterval(_tick, 100);
    }

    function stop() {
        if (intervalId) {
            clearInterval(intervalId);
            intervalId = null;
        }
    }

    // When the game ends nothing is running, so nothing stays highlighted
    function clearActive() {
        activeColor = null;
        _render();
    }

    function syncTime(clocks) {
        unlimited = !!clocks.unlimited;
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
        if (!activeColor || unlimited) return;
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

        var text = unlimited ? 'no limit' : _formatTime(ms);
        if (activeColor === color && !unlimited) {
            var mark = el.firstElementChild;
            if (!mark || mark.tagName !== 'MARK') {
                mark = document.createElement('mark');
                el.replaceChildren(mark);
            }
            mark.textContent = text;
        } else {
            el.textContent = text;
        }
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
        clearActive: clearActive,
        syncTime: syncTime,
        setActive: setActive
    };
})();
