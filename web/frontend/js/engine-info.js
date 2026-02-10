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
            // Mate score is from engine's perspective — negate for player
            var playerMate = -mate;
            text = (playerMate > 0 ? '+' : '') + 'M' + Math.abs(playerMate);
            pct = playerMate > 0 ? 95 : 5;
        } else if (cp !== null && cp !== undefined) {
            // Score is from engine's perspective — negate so positive = good for player
            var displayCp = -cp;
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

        // Position text: bottom on white fill when winning, top on dark when losing
        if (pct >= 50) {
            textEl.style.bottom = '4px';
            textEl.style.top = 'auto';
            textEl.style.color = '#333';
        } else {
            textEl.style.top = '4px';
            textEl.style.bottom = 'auto';
            textEl.style.color = '#fff';
        }
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
