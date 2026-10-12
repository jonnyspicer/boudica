/**
 * Game state machine: new game, moves, history, game over.
 */
var Boudica = window.Boudica || {};

Boudica.Game = (function () {
    var gameId = null;
    var playerColorStr = 'white';
    var _isPlayerTurn = false;
    var _isGameOver = false;
    var moveHistory = []; // [{num, white: san, black: san}]
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
        _isPlayerTurn = false;
        moveHistory = [];
        moveCount = 0;
        playerColorStr = color;

        // Reset UI
        Boudica.Board.reset();
        Boudica.Board.setOrientation(color);
        Boudica.EngineInfo.clear();
        Boudica.Clock.stop();
        _renderMoveTable();
        _setStatus(['Starting a new game…']);

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
        // Our move is shown at once; until the server answers, it's Boudica's turn.
        _isPlayerTurn = false;
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
        _setStatus(['You offered a draw. Waiting for Boudica…']);
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

        _setStatus([_isPlayerTurn ? 'You play White. Your move.' : 'You play Black. Boudica is thinking…']);
    }

    function _onMoveMade(msg) {
        var isEngineMove = (msg.color === 'white') !== (playerColorStr === 'white');
        Boudica.Board.setPosition(msg.fen);
        _isPlayerTurn = msg.is_player_turn;

        // Highlight last move
        if (msg.uci && msg.uci.length >= 4) {
            Boudica.Board.highlightLastMove(msg.uci.substring(0, 2), msg.uci.substring(2, 4));
        }

        // Sync clocks
        Boudica.Clock.syncTime(msg.clocks);
        var activeColor = msg.is_player_turn ? playerColorStr :
            (playerColorStr === 'white' ? 'black' : 'white');
        Boudica.Clock.setActive(activeColor);

        // Add to move history
        var numbered = _addMove(msg.color, msg.san);

        if (isEngineMove) {
            Boudica.EngineInfo.afterEngineMove(msg.uci);
            _setStatus([
                'Boudica played ', _mono(numbered), '.',
                Boudica.Board.isInCheck() ? " You're in check - your move." : ' Your move.'
            ]);
        } else {
            _setStatus(['Boudica is thinking…']);
        }
    }

    function _onEngineThinking(msg) {
        Boudica.EngineInfo.update(msg);
    }

    function _onGameOver(msg) {
        _isGameOver = true;
        _isPlayerTurn = false;
        Boudica.Clock.stop();
        Boudica.Clock.clearActive();

        document.getElementById('btn-resign').disabled = true;
        document.getElementById('btn-draw').disabled = true;

        Boudica.Board.setPosition(msg.fen);

        var result, playerWon = false;
        if (msg.result === 'draw') {
            result = 'Draw';
        } else {
            playerWon =
                (msg.result === 'white_wins' && playerColorStr === 'white') ||
                (msg.result === 'black_wins' && playerColorStr === 'black');
            result = playerWon ? 'You win' : 'Boudica wins';
        }

        _setStatus([
            _bold(result),
            _describeEnding(msg.termination, playerWon),
            ' Choose your settings and press Start to play again.'
        ]);
    }

    function _onError(msg) {
        console.warn('Game error:', msg.code, msg.message);
        // Put the board back on an illegal move
        if (msg.code === 'illegal_move') {
            _isPlayerTurn = true;
            Boudica.Board.cancelPending();
            _setStatus(["That move isn't allowed. Your move."]);
        }
    }

    function _onDrawDeclined() {
        _setStatus(['Boudica declined the draw.', _isPlayerTurn ? ' Your move.' : '']);
    }

    function _onClockUpdate(msg) {
        Boudica.Clock.syncTime(msg.clocks);
    }

    // ---- Status ----

    // The status line is one sentence or two, built from text and elements.
    function _setStatus(parts) {
        var el = document.getElementById('game-status');
        el.replaceChildren.apply(el, parts.map(function (p) {
            return typeof p === 'string' ? document.createTextNode(p) : p;
        }));
    }

    function _describeEnding(termination, playerWon) {
        var endings = {
            checkmate: ' by checkmate.',
            flag: ' on time.',
            resignation: playerWon ? ' - Boudica resigned.' : ' - you resigned.',
            stalemate: ' by stalemate.',
            insufficient_material: ' - neither side has enough pieces left to checkmate.',
            fifty_moves: ' by the 50-move rule.',
            threefold_repetition: ' by threefold repetition.',
            agreement: ' by agreement.'
        };
        if (endings[termination]) return endings[termination];
        return termination ? ' - ' + String(termination).replace(/_/g, ' ') + '.' : '.';
    }

    function _bold(text) {
        var b = document.createElement('b');
        b.textContent = text;
        return b;
    }

    function _mono(text) {
        var span = document.createElement('span');
        span.className = 'mono';
        span.textContent = text;
        return span;
    }

    // ---- Move History ----

    // Records a move and returns it numbered, e.g. "8...d6".
    function _addMove(color, san) {
        if (color === 'white') {
            moveCount++;
            moveHistory.push({ num: moveCount, white: san, black: '' });
            _renderMoveTable();
            return moveCount + '.' + san;
        }
        if (moveHistory.length === 0) {
            moveCount++;
            moveHistory.push({ num: moveCount, white: '...', black: san });
        } else {
            moveHistory[moveHistory.length - 1].black = san;
        }
        _renderMoveTable();
        return moveHistory[moveHistory.length - 1].num + '...' + san;
    }

    function _renderMoveTable() {
        var tbody = document.querySelector('#move-table tbody');
        var rows = moveHistory.map(function (m) {
            var row = document.createElement('tr');
            [m.num + '.', m.white, m.black].forEach(function (text) {
                var cell = document.createElement('td');
                cell.textContent = text;
                row.appendChild(cell);
            });
            return row;
        });
        tbody.replaceChildren.apply(tbody, rows);

        document.getElementById('moves-empty').hidden = moveHistory.length > 0;

        // Keep the latest move in view
        var container = document.getElementById('moves');
        if (container) container.scrollTop = container.scrollHeight;
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
