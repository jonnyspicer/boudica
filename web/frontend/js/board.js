/**
 * Chessboard.js wrapper: drag-drop, highlights, orientation.
 */
var Boudica = window.Boudica || {};

Boudica.Board = (function () {
    var board = null;
    var chessGame = null;  // chess.js instance for legal move display
    var playerColor = 'white';
    var lastMoveFrom = null;
    var lastMoveTo = null;
    var selectedSquare = null;
    var pendingPromotion = null;
    var _awaitingServer = false; // true after sending a move, prevents snapback

    function init() {
        chessGame = new Chess();

        var config = {
            draggable: true,
            position: 'start',
            pieceTheme: 'img/chesspieces/wikipedia/{piece}.png',
            onDragStart: _onDragStart,
            onDrop: _onDrop,
            onSnapEnd: _onSnapEnd,
            onMouseoutSquare: _onMouseoutSquare,
            onMouseoverSquare: _onMouseoverSquare
        };

        board = Chessboard('board', config);

        // Make board responsive
        $(window).on('resize', function () {
            board.resize();
        });

        // Click-to-move support
        $('#board').on('click', '.square-55d63', function () {
            var sq = $(this).attr('data-square');
            if (!sq) return;
            _onSquareClick(sq);
        });
    }

    function setPosition(fen, animate) {
        if (animate === undefined) animate = true;
        _awaitingServer = false;
        if (board) board.position(fen, animate);
        if (chessGame) chessGame.load(fen);
    }

    function setOrientation(color) {
        playerColor = color;
        if (board) board.orientation(color);
    }

    function getOrientation() {
        return playerColor;
    }

    function highlightLastMove(from, to) {
        _clearHighlights();
        lastMoveFrom = from;
        lastMoveTo = to;
        if (from) $('#board .square-' + from).addClass('highlight-last-move');
        if (to) $('#board .square-' + to).addClass('highlight-last-move');
    }

    function highlightCheck() {
        if (!chessGame) return;
        if (chessGame.in_check()) {
            var board_arr = chessGame.board();
            for (var r = 0; r < 8; r++) {
                for (var c = 0; c < 8; c++) {
                    var piece = board_arr[r][c];
                    if (piece && piece.type === 'k' && piece.color === chessGame.turn()) {
                        var file = String.fromCharCode(97 + c);
                        var rank = String(8 - r);
                        $('#board .square-' + file + rank).addClass('highlight-check');
                    }
                }
            }
        }
    }

    function reset() {
        if (board) board.start(false);
        if (chessGame) chessGame.reset();
        _clearHighlights();
        lastMoveFrom = null;
        lastMoveTo = null;
        selectedSquare = null;
        _awaitingServer = false;
    }

    function cancelPending() {
        _awaitingServer = false;
        if (chessGame && board) board.position(chessGame.fen());
    }

    // ---- Private ----

    function _onDragStart(source, piece) {
        if (Boudica.Game && Boudica.Game.isGameOver()) return false;
        if (Boudica.Game && !Boudica.Game.isPlayerTurn()) return false;
        if (playerColor === 'white' && piece.search(/^b/) !== -1) return false;
        if (playerColor === 'black' && piece.search(/^w/) !== -1) return false;
        return true;
    }

    function _onDrop(source, target) {
        // Check for promotion
        if (chessGame) {
            var piece = chessGame.get(source);
            if (piece && piece.type === 'p') {
                var targetRank = parseInt(target[1]);
                if ((piece.color === 'w' && targetRank === 8) ||
                    (piece.color === 'b' && targetRank === 1)) {
                    pendingPromotion = { from: source, to: target };
                    _showPromotionDialog(piece.color);
                    return 'snapback';
                }
            }
        }

        // Validate with chess.js
        var move = chessGame ? chessGame.move({
            from: source,
            to: target,
            promotion: 'q'
        }) : null;

        if (move === null) return 'snapback';

        // Undo the chess.js move (server is authoritative)
        chessGame.undo();

        // Send move to server
        _awaitingServer = true;
        var uci = source + target;
        if (Boudica.Game) Boudica.Game.sendMove(uci);

        selectedSquare = null;
        return undefined;
    }

    function _onSnapEnd() {
        // Skip snapback when we've sent a move to the server —
        // the piece is already visually at the target from the drag.
        if (_awaitingServer) return;
        if (chessGame) board.position(chessGame.fen());
    }

    function _onSquareClick(square) {
        if (Boudica.Game && Boudica.Game.isGameOver()) return;
        if (Boudica.Game && !Boudica.Game.isPlayerTurn()) return;

        if (selectedSquare) {
            var from = selectedSquare;
            var to = square;
            selectedSquare = null;
            _clearSelectionHighlights();

            // Check promotion
            if (chessGame) {
                var piece = chessGame.get(from);
                if (piece && piece.type === 'p') {
                    var targetRank = parseInt(to[1]);
                    if ((piece.color === 'w' && targetRank === 8) ||
                        (piece.color === 'b' && targetRank === 1)) {
                        var legalMoves = chessGame.moves({ square: from, verbose: true });
                        var isLegal = legalMoves.some(function(m) { return m.to === to; });
                        if (isLegal) {
                            pendingPromotion = { from: from, to: to };
                            _showPromotionDialog(piece.color);
                            return;
                        }
                    }
                }
            }

            // Validate
            var testMove = chessGame ? chessGame.move({ from: from, to: to, promotion: 'q' }) : null;
            if (testMove) {
                chessGame.undo();
                _awaitingServer = true;
                var uci = from + to;
                if (Boudica.Game) Boudica.Game.sendMove(uci);
            }
        } else {
            if (!chessGame) return;
            var p = chessGame.get(square);
            if (!p) return;
            var pColor = p.color === 'w' ? 'white' : 'black';
            if (pColor !== playerColor) return;

            selectedSquare = square;
            $('#board .square-' + square).addClass('highlight-selected');

            var moves = chessGame.moves({ square: square, verbose: true });
            for (var i = 0; i < moves.length; i++) {
                $('#board .square-' + moves[i].to).addClass('highlight-legal');
            }
        }
    }

    function _onMouseoverSquare() {}
    function _onMouseoutSquare() {}

    function _clearHighlights() {
        $('#board .square-55d63').removeClass(
            'highlight-last-move highlight-legal highlight-check highlight-selected'
        );
    }

    function _clearSelectionHighlights() {
        $('#board .square-55d63').removeClass('highlight-legal highlight-selected');
    }

    function _showPromotionDialog(pieceColor) {
        var prefix = pieceColor === 'w' ? 'w' : 'b';
        var pieces = ['Q', 'R', 'B', 'N'];
        var dialog = document.getElementById('promotion-dialog');

        // Clear existing children safely
        while (dialog.firstChild) {
            dialog.removeChild(dialog.firstChild);
        }

        for (var i = 0; i < pieces.length; i++) {
            var img = document.createElement('img');
            img.src = 'img/chesspieces/wikipedia/' + prefix + pieces[i] + '.png';
            img.alt = pieces[i];
            img.dataset.piece = pieces[i].toLowerCase();
            img.addEventListener('click', _onPromotionChoice);
            dialog.appendChild(img);
        }

        document.getElementById('promotion-overlay').classList.add('visible');
    }

    function _onPromotionChoice(e) {
        document.getElementById('promotion-overlay').classList.remove('visible');
        var piece = e.target.dataset.piece;
        if (pendingPromotion) {
            var uci = pendingPromotion.from + pendingPromotion.to + piece;
            pendingPromotion = null;
            if (Boudica.Game) Boudica.Game.sendMove(uci);
        }
    }

    return {
        init: init,
        setPosition: setPosition,
        setOrientation: setOrientation,
        getOrientation: getOrientation,
        highlightLastMove: highlightLastMove,
        highlightCheck: highlightCheck,
        reset: reset,
        cancelPending: cancelPending
    };
})();
