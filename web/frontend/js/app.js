/**
 * App initialization: wire UI controls to game/board/clock.
 */
$(document).ready(function () {
    // Init modules
    Boudica.Board.init();
    Boudica.Game.init();

    // New Game button
    $('#btn-new-game').on('click', function () {
        var skill = parseInt($('#skill-select').val());
        var timeParts = $('#time-select').val().split('|');
        var timeMs = parseInt(timeParts[0]);
        var incMs = parseInt(timeParts[1]);
        var colorChoice = $('#color-select').val();
        var color = colorChoice;
        if (color === 'random') {
            color = Math.random() < 0.5 ? 'white' : 'black';
        }

        Boudica.Game.newGame(skill, timeMs, incMs, color);
    });

    // Resign
    $('#btn-resign').on('click', function () {
        if (confirm('Are you sure you want to resign?')) {
            Boudica.Game.resign();
        }
    });

    // Draw offer
    $('#btn-draw').on('click', function () {
        Boudica.Game.offerDraw();
    });

    // Dismiss promotion overlay on background click
    $('#promotion-overlay').on('click', function (e) {
        if (e.target === this) {
            $(this).removeClass('visible');
        }
    });

    // Time control info popup
    $('#time-info-btn').on('click', function (e) {
        e.preventDefault();
        $('#time-info-overlay').addClass('visible');
    });

    $('#time-info-overlay').on('click', function (e) {
        if (e.target === this) {
            $(this).removeClass('visible');
        }
    });

    $('#time-info-overlay .info-close').on('click', function () {
        $('#time-info-overlay').removeClass('visible');
    });
});
