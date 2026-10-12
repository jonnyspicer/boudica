/**
 * The board: a table of square buttons with Unicode pieces (the design
 * system's Board recipe). Click a piece, then a square, to move.
 * chess.js validates moves locally; the server stays authoritative.
 */
var Boudica = window.Boudica || {};

Boudica.Board = (function () {
    var FILES = 'abcdefgh';
    // U+FE0E after each glyph stops phones drawing the pieces as emoji.
    var GLYPHS = {
        w: { k: '♔', q: '♕', r: '♖', b: '♗', n: '♘', p: '♙' },
        b: { k: '♚', q: '♛', r: '♜', b: '♝', n: '♞', p: '♟' }
    };
    var NAMES = { k: 'king', q: 'queen', r: 'rook', b: 'bishop', n: 'knight', p: 'pawn' };

    var table = null;
    var chessGame = null;   // chess.js instance for legal moves and display
    var serverFen = null;   // last position the server confirmed
    var playerColor = 'white';
    var lastMoveFrom = null;
    var lastMoveTo = null;
    var selectedSquare = null;
    var pendingPromotion = null;

    function init() {
        chessGame = new Chess();
        serverFen = chessGame.fen();
        table = document.getElementById('board');

        table.addEventListener('click', function (e) {
            var button = e.target.closest('button[data-square]');
            if (button) _onSquareClick(button.dataset.square);
        });

        document.getElementById('promotion-dialog')
            .addEventListener('close', _onPromotionClose);

        _render();
    }

    function setPosition(fen) {
        serverFen = fen;
        if (chessGame) chessGame.load(fen);
        selectedSquare = null;
        _render();
    }

    function setOrientation(color) {
        playerColor = color;
        _render();
    }

    function getOrientation() {
        return playerColor;
    }

    function getFen() {
        return chessGame ? chessGame.fen() : null;
    }

    function isInCheck() {
        return chessGame ? chessGame.in_check() : false;
    }

    function highlightLastMove(from, to) {
        lastMoveFrom = from;
        lastMoveTo = to;
        _render();
    }

    // Check is said in words (the move's "+" and the status line), not drawn.
    function highlightCheck() {}

    function reset() {
        if (chessGame) chessGame.reset();
        serverFen = chessGame ? chessGame.fen() : null;
        lastMoveFrom = null;
        lastMoveTo = null;
        selectedSquare = null;
        pendingPromotion = null;
        _render();
    }

    // The server rejected our move: go back to the last position it confirmed.
    function cancelPending() {
        if (chessGame && serverFen) chessGame.load(serverFen);
        selectedSquare = null;
        _render();
    }

    // ---- Private ----

    function _canMove() {
        if (Boudica.Game && Boudica.Game.isGameOver()) return false;
        if (Boudica.Game && !Boudica.Game.isPlayerTurn()) return false;
        return true;
    }

    function _isOwnPiece(square) {
        var p = chessGame.get(square);
        return !!p && (p.color === 'w' ? 'white' : 'black') === playerColor;
    }

    function _onSquareClick(square) {
        if (!chessGame || !_canMove()) return;

        if (selectedSquare) {
            var from = selectedSquare;

            // Clicking the picked-up piece again puts it down; another of
            // your own pieces picks that one up instead.
            if (square === from) {
                selectedSquare = null;
                _render();
                return;
            }
            if (_isOwnPiece(square)) {
                selectedSquare = square;
                _render(square);
                return;
            }

            selectedSquare = null;
            var legal = chessGame.moves({ square: from, verbose: true })
                .some(function (m) { return m.to === square; });
            if (!legal) {
                _render();
                return;
            }

            var piece = chessGame.get(from);
            var lastRank = piece.color === 'w' ? '8' : '1';
            if (piece.type === 'p' && square[1] === lastRank) {
                pendingPromotion = { from: from, to: square };
                _render();
                document.getElementById('promotion-dialog').showModal();
                return;
            }

            _play(from, square, '');
        } else if (_isOwnPiece(square)) {
            selectedSquare = square;
            _render(square);
        }
    }

    // Show the move straight away, then send it; the server's reply replaces it.
    function _play(from, to, promotion) {
        chessGame.move({ from: from, to: to, promotion: promotion || 'q' });
        lastMoveFrom = from;
        lastMoveTo = to;
        _render();
        if (Boudica.Game) Boudica.Game.sendMove(from + to + promotion);
    }

    function _onPromotionClose(e) {
        var choice = e.target.returnValue;
        var move = pendingPromotion;
        pendingPromotion = null;
        if (move && /^[qrbn]$/.test(choice)) {
            _play(move.from, move.to, choice);
        }
    }

    function _render(focusSquare) {
        if (!table || !chessGame) return;

        var dots = {};
        if (selectedSquare) {
            chessGame.moves({ square: selectedSquare, verbose: true })
                .forEach(function (m) { dots[m.to] = true; });
        }

        var ranks = [8, 7, 6, 5, 4, 3, 2, 1];
        var files = FILES.split('');
        if (playerColor === 'black') {
            ranks.reverse();
            files.reverse();
        }

        var rows = [];
        ranks.forEach(function (rank) {
            var tr = document.createElement('tr');
            var th = document.createElement('th');
            th.scope = 'row';
            th.textContent = rank;
            tr.appendChild(th);
            files.forEach(function (file) {
                tr.appendChild(_cell(file, rank, dots));
            });
            rows.push(tr);
        });

        var foot = document.createElement('tr');
        foot.appendChild(document.createElement('th'));
        files.forEach(function (file) {
            var th = document.createElement('th');
            th.scope = 'col';
            th.textContent = file;
            foot.appendChild(th);
        });
        rows.push(foot);

        table.replaceChildren.apply(table, rows);

        if (focusSquare) {
            var b = table.querySelector('button[data-square="' + focusSquare + '"]');
            if (b) b.focus();
        }
    }

    function _cell(file, rank, dots) {
        var square = file + rank;
        var td = document.createElement('td');
        if ((FILES.indexOf(file) + rank) % 2 === 1) td.className = 'dark';  // a1 is dark
        if (square === lastMoveFrom || square === lastMoveTo) td.className = 'last';

        var button = document.createElement('button');
        button.type = 'button';
        button.dataset.square = square;

        var piece = chessGame.get(square);
        var label = square;
        if (piece) {
            button.textContent = GLYPHS[piece.color][piece.type] + '︎';
            label += ' ' + (piece.color === 'w' ? 'white ' : 'black ') + NAMES[piece.type];
        } else if (dots[square]) {
            button.textContent = '·';
            button.className = 'dot';
            label += ', legal move';
        }
        if (square === selectedSquare) {
            button.classList.add('picked');
            button.setAttribute('aria-pressed', 'true');
        }
        button.setAttribute('aria-label', label);

        td.appendChild(button);
        return td;
    }

    return {
        init: init,
        setPosition: setPosition,
        setOrientation: setOrientation,
        getOrientation: getOrientation,
        getFen: getFen,
        isInCheck: isInCheck,
        highlightLastMove: highlightLastMove,
        highlightCheck: highlightCheck,
        reset: reset,
        cancelPending: cancelPending
    };
})();
