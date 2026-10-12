/**
 * Engine info: the evaluation as a text bar and a sentence, and the line
 * Boudica expects, in ordinary move notation.
 */
var Boudica = window.Boudica || {};

Boudica.EngineInfo = (function () {
    var BAR_WIDTH = 24;
    var PV_MOVES = 6;
    var lastPv = [];

    function update(info) {
        var whiteCp = null;
        var whiteMate = null;
        // Scores arrive from Boudica's side; turn them into White's.
        var sign = _engineColor() === 'white' ? 1 : -1;
        if (info.score_mate !== null && info.score_mate !== undefined) {
            whiteMate = sign * info.score_mate;
        } else if (info.score_cp !== null && info.score_cp !== undefined) {
            whiteCp = sign * info.score_cp;
        }

        _renderEval(whiteCp, whiteMate, info.depth);
        if (info.pv && info.pv.length > 0) {
            lastPv = info.pv;
            _renderLine(info.pv);
        }
    }

    // Boudica's line starts with the move it was choosing. Once that move is
    // played, show only what it expects to happen next.
    function afterEngineMove(uci) {
        var rest = lastPv[0] === uci ? lastPv.slice(1) : [];
        lastPv = rest;
        if (rest.length > 0) {
            _renderLine(rest);
        } else {
            document.getElementById('engine-line').textContent = '';
        }
    }

    function clear() {
        _setBar(BAR_WIDTH / 2, 'Evaluation: no evaluation yet');
        document.getElementById('eval-words').textContent = 'No evaluation yet.';
        document.getElementById('engine-line').textContent = '';
        lastPv = [];
    }

    function _engineColor() {
        var player = Boudica.Game ? Boudica.Game.getPlayerColor() : 'white';
        return player === 'white' ? 'black' : 'white';
    }

    function _renderEval(whiteCp, whiteMate, depth) {
        var share, words, label;

        if (whiteMate !== null) {
            var side = whiteMate > 0 ? 'White' : 'Black';
            var n = Math.abs(whiteMate);
            share = whiteMate > 0 ? BAR_WIDTH : 0;
            words = side + ' mates in ' + n + '.';
            label = 'Evaluation: ' + side + ' mates in ' + n;
        } else if (whiteCp !== null) {
            // Sigmoid mapping from centipawns to White's share of the bar
            var pct = 50 + 50 * (2 / (1 + Math.exp(-whiteCp / 200)) - 1);
            share = Math.round(pct / 100 * BAR_WIDTH);
            var pawns = whiteCp / 100;
            var shown = pawns.toFixed(1);
            if (shown === '0.0' || shown === '-0.0') shown = '0.0';
            else if (pawns > 0) shown = '+' + shown;
            words = shown + ', ' + _describe(pawns) + '.';
            label = 'Evaluation: White ' + Math.round(pct) + ' percent, Black ' + (100 - Math.round(pct)) + ' percent';
        } else {
            return;
        }

        _setBar(share, label);
        document.getElementById('eval-words').textContent =
            words + (depth ? ' Depth ' + depth + '.' : '');
    }

    function _describe(pawns) {
        var side = pawns > 0 ? 'White' : 'Black';
        var size = Math.abs(pawns);
        if (size < 0.3) return 'the position is level';
        if (size < 1) return side + ' is slightly better';
        if (size < 2.5) return side + ' is better';
        return side + ' is winning';
    }

    function _setBar(share, label) {
        var bar = document.getElementById('eval-bar');
        bar.textContent = 'White ' + '#'.repeat(share) + '.'.repeat(BAR_WIDTH - share) + ' Black';
        bar.setAttribute('aria-label', label);
    }

    // Replay Boudica's expected line (UCI) from the current position to get
    // numbered algebraic notation: "9.h3 Na5 10.Bc2".
    function _renderLine(pv) {
        var el = document.getElementById('engine-line');
        var fen = Boudica.Board.getFen();
        el.textContent = '';
        if (!pv || pv.length === 0 || !fen) return;

        var game = new Chess(fen);
        var moveNumber = parseInt(fen.split(' ')[5], 10) || 1;
        var parts = [];

        for (var i = 0; i < Math.min(pv.length, PV_MOVES); i++) {
            var uci = pv[i];
            var move = game.move({ from: uci.slice(0, 2), to: uci.slice(2, 4), promotion: uci[4] || 'q' });
            if (!move) break;
            if (move.color === 'w') {
                parts.push(moveNumber + '.' + move.san);
            } else {
                parts.push(i === 0 ? moveNumber + '...' + move.san : move.san);
                moveNumber++;
            }
        }

        if (parts.length > 0) {
            el.replaceChildren(
                document.createTextNode('Boudica expects '),
                _mono(parts.join(' ')),
                document.createTextNode('.')
            );
        }
    }

    function _mono(text) {
        var span = document.createElement('span');
        span.className = 'mono';
        span.textContent = text;
        return span;
    }

    return {
        update: update,
        afterEngineMove: afterEngineMove,
        clear: clear
    };
})();
