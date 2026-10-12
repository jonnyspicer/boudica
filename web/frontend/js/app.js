/**
 * App initialization: wire UI controls to game/board/clock.
 */
document.addEventListener('DOMContentLoaded', function () {
    // Init modules
    Boudica.Board.init();
    Boudica.Game.init();

    // Start a new game from the settings fieldset
    document.getElementById('new-game-form').addEventListener('submit', function (e) {
        e.preventDefault();
        var skill = parseInt(document.getElementById('skill-select').value, 10);
        var timeParts = document.getElementById('time-select').value.split('|');
        var timeMs = parseInt(timeParts[0], 10);
        var incMs = parseInt(timeParts[1], 10);
        var color = document.getElementById('color-select').value;
        if (color === 'random') {
            color = Math.random() < 0.5 ? 'white' : 'black';
        }

        Boudica.Game.newGame(skill, timeMs, incMs, color);
    });

    // Resign sits inside a details element that says what it does, so no confirm box
    document.getElementById('btn-resign').addEventListener('click', function () {
        document.getElementById('resign-details').open = false;
        Boudica.Game.resign();
    });

    // Draw offer
    document.getElementById('btn-draw').addEventListener('click', function () {
        Boudica.Game.offerDraw();
    });
});
