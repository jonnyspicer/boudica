/**
 * Game state machine: new game, moves, history, game over.
 */
var Boudica = window.Boudica || {};

Boudica.Game = (function () {
    var gameId = null;
    var playerColorStr = 'white';
    var _isPlayerTurn = false;
    var _isGameOver = false;
    var moveHistory = []; // [{moveNum, white: san, black: san}]
    var moveCount = 0;

    function init() {
        // Wire WS handlers
        Boudica.WebSocketClient.on('game_started', _onGameStarted);
        Boudica.WebSocketClient.on('move_made', _onMoveMade);
        Boudica.WebSocketClient.on('engine_thinking', _onEngineThinking);
        Boudica.WebSocketClient.on('game_over', _onGameOver);
        Boudica.WebSocketClient.on('error', _onError);
        Boudica.WebSocketClient.on('draw_declined', _onDrawDeclined);
        Boudica.WebSocketClient.on('clock_update', _onClockUpdate);
    }

    function newGame(skillLevel, timeMs, incrementMs, color) {
        // Generate game ID
        gameId = _uuid();
        _isGameOver = false;
        moveHistory = [];
        moveCount = 0;
        playerColorStr = color;

        // Reset UI
        Boudica.Board.reset();
        Boudica.Board.setOrientation(color);
        Boudica.EngineInfo.clear();
        Boudica.Clock.stop();
        _clearMoveTable();
        _hideStatus();

        // Update player bars
        _updatePlayerBars(color);

        // Enable buttons
        document.getElementById('btn-resign').disabled = false;
        document.getElementById('btn-draw').disabled = false;

        // Connect and start
        Boudica.WebSocketClient.connect(gameId);

        // Wait for connection then send new_game
        Boudica.WebSocketClient.on('connected', function onConnected() {
            Boudica.WebSocketClient.off('connected', onConnected);
            Boudica.WebSocketClient.send({
                type: 'new_game',
                skill_level: skillLevel,
                time_ms: timeMs,
                increment_ms: incrementMs,
                player_color: color
            });
        });
    }

    function sendMove(uci) {
        Boudica.WebSocketClient.send({
            type: 'player_move',
            uci: uci
        });
    }

    function resign() {
        Boudica.WebSocketClient.send({ type: 'resign' });
    }

    function offerDraw() {
        Boudica.WebSocketClient.send({ type: 'offer_draw' });
    }

    function isPlayerTurn() { return _isPlayerTurn; }
    function isGameOver() { return _isGameOver; }
    function getPlayerColor() { return playerColorStr; }

    // ---- WS Handlers ----

    function _onGameStarted(msg) {
        playerColorStr = msg.player_color;
        _isPlayerTurn = (msg.player_color === 'white'); // white moves first

        Boudica.Board.setPosition(msg.fen);
        Boudica.Board.setOrientation(msg.player_color);
        Boudica.Clock.syncTime(msg.clocks);
        Boudica.Clock.setActive(_isPlayerTurn ? msg.player_color : (msg.player_color === 'white' ? 'black' : 'white'));
        Boudica.Clock.start();
    }

    function _onMoveMade(msg) {
        // Player's own move: piece already at target from drag/click, no animation.
        // Engine's move: animate so it's visually distinct.
        var isEngineMove = (msg.color === 'white') !== (playerColorStr === 'white');
        Boudica.Board.setPosition(msg.fen, isEngineMove);
        _isPlayerTurn = msg.is_player_turn;

        // Highlight last move
        if (msg.uci && msg.uci.length >= 4) {
            var from = msg.uci.substring(0, 2);
            var to = msg.uci.substring(2, 4);
            Boudica.Board.highlightLastMove(from, to);
        }

        // Check highlight
        Boudica.Board.highlightCheck();

        // Sync clocks
        Boudica.Clock.syncTime(msg.clocks);
        var activeColor = msg.is_player_turn ? playerColorStr :
            (playerColorStr === 'white' ? 'black' : 'white');
        Boudica.Clock.setActive(activeColor);

        // Add to move history
        _addMove(msg.color, msg.san);
    }

    function _onEngineThinking(msg) {
        Boudica.EngineInfo.update(msg);
    }

    function _onGameOver(msg) {
        _isGameOver = true;
        _isPlayerTurn = false;
        Boudica.Clock.stop();

        document.getElementById('btn-resign').disabled = true;
        document.getElementById('btn-draw').disabled = true;

        Boudica.Board.setPosition(msg.fen);

        // Show result
        var statusEl = document.getElementById('game-status');
        var text = '';
        var cls = 'draw';

        if (msg.result === 'draw') {
            text = 'Draw - ' + _formatTermination(msg.termination);
            cls = 'draw';
        } else {
            var winnerIsPlayer =
                (msg.result === 'white_wins' && playerColorStr === 'white') ||
                (msg.result === 'black_wins' && playerColorStr === 'black');
            if (winnerIsPlayer) {
                text = 'You win! - ' + _formatTermination(msg.termination);
                cls = 'win';
            } else {
                text = 'Boudica wins - ' + _formatTermination(msg.termination);
                cls = 'loss';
            }
        }

        statusEl.textContent = text;
        statusEl.className = 'game-status visible ' + cls;
    }

    function _onError(msg) {
        console.warn('Game error:', msg.code, msg.message);
        // Snap board back on illegal move
        if (msg.code === 'illegal_move') {
            Boudica.Board.cancelPending();
        }
    }

    function _onDrawDeclined() {
        // Could show a toast, for now just log
        console.log('Draw offer declined');
    }

    function _onClockUpdate(msg) {
        Boudica.Clock.syncTime(msg.clocks);
    }

    // ---- Move History ----

    function _addMove(color, san) {
        if (color === 'white') {
            moveCount++;
            moveHistory.push({ num: moveCount, white: san, black: '' });
        } else {
            if (moveHistory.length === 0) {
                moveCount++;
                moveHistory.push({ num: moveCount, white: '...', black: san });
            } else {
                moveHistory[moveHistory.length - 1].black = san;
            }
        }
        _renderMoveTable();
    }

    function _renderMoveTable() {
        var tbody = document.querySelector('#move-table tbody');
        // Clear safely
        while (tbody.firstChild) {
            tbody.removeChild(tbody.firstChild);
        }

        for (var i = 0; i < moveHistory.length; i++) {
            var row = document.createElement('tr');

            var numCell = document.createElement('td');
            numCell.textContent = moveHistory[i].num + '.';
            row.appendChild(numCell);

            var whiteCell = document.createElement('td');
            whiteCell.textContent = moveHistory[i].white;
            row.appendChild(whiteCell);

            var blackCell = document.createElement('td');
            blackCell.textContent = moveHistory[i].black;
            row.appendChild(blackCell);

            tbody.appendChild(row);
        }

        // Scroll to bottom
        var container = document.querySelector('.move-history');
        if (container) container.scrollTop = container.scrollHeight;
    }

    function _clearMoveTable() {
        var tbody = document.querySelector('#move-table tbody');
        if (tbody) {
            while (tbody.firstChild) {
                tbody.removeChild(tbody.firstChild);
            }
        }
    }

    function _hideStatus() {
        var el = document.getElementById('game-status');
        if (el) {
            el.className = 'game-status';
            el.textContent = '';
        }
    }

    function _updatePlayerBars(color) {
        var topName = document.getElementById('top-name');
        var bottomName = document.getElementById('bottom-name');
        if (topName) topName.textContent = 'Boudica';
        if (bottomName) bottomName.textContent = 'You';
    }

    function _formatTermination(t) {
        var map = {
            checkmate: 'Checkmate',
            stalemate: 'Stalemate',
            insufficient_material: 'Insufficient material',
            fifty_moves: '50-move rule',
            threefold_repetition: 'Threefold repetition',
            resignation: 'Resignation',
            flag: 'Time forfeit'
        };
        return map[t] || t;
    }

    function _uuid() {
        return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function (c) {
            var r = Math.random() * 16 | 0;
            var v = c === 'x' ? r : (r & 0x3 | 0x8);
            return v.toString(16);
        });
    }

    return {
        init: init,
        newGame: newGame,
        sendMove: sendMove,
        resign: resign,
        offerDraw: offerDraw,
        isPlayerTurn: isPlayerTurn,
        isGameOver: isGameOver,
        getPlayerColor: getPlayerColor
    };
})();
