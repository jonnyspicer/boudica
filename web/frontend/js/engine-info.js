/**
 * Engine info display: eval bar, depth, PV.
 */
var Boudica = window.Boudica || {};

Boudica.EngineInfo = (function () {
    function update(info) {
        // Depth
        var depthEl = document.getElementById('info-depth');
        if (depthEl) depthEl.textContent = info.depth || '-';

        // PV display (UCI moves)
        var pvEl = document.getElementById('info-pv');
        if (pvEl && info.pv && info.pv.length > 0) {
            pvEl.textContent = info.pv.slice(0, 6).join(' ');
        }

        // Score display
        _updateEval(info.score_cp, info.score_mate);
    }

    function _updateEval(cp, mate) {
        var fillEl = document.getElementById('eval-fill');
        var textEl = document.getElementById('eval-text');
        if (!fillEl || !textEl) return;

        var pct, text;

        if (mate !== null && mate !== undefined) {
            text = (mate > 0 ? '+' : '') + 'M' + Math.abs(mate);
            pct = mate > 0 ? 95 : 5;
        } else if (cp !== null && cp !== undefined) {
            // Flip eval if playing as black
            var displayCp = cp;
            if (Boudica.Game && Boudica.Game.getPlayerColor() === 'black') {
                displayCp = -cp;
            }
            // Sigmoid mapping: map cp to 0-100%
            pct = 50 + 50 * (2 / (1 + Math.exp(-displayCp / 200)) - 1);
            pct = Math.max(2, Math.min(98, pct));
            text = (displayCp >= 0 ? '+' : '') + (displayCp / 100).toFixed(1);
        } else {
            pct = 50;
            text = '0.0';
        }

        fillEl.style.height = pct + '%';
        textEl.textContent = text;
    }

    function clear() {
        var depthEl = document.getElementById('info-depth');
        var pvEl = document.getElementById('info-pv');
        if (depthEl) depthEl.textContent = '-';
        if (pvEl) pvEl.textContent = '-';
        _updateEval(null, null);
    }

    return {
        update: update,
        clear: clear
    };
})();
