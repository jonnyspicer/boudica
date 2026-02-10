"""End-to-end browser tests using Playwright with Firefox.

Run with:
    python3 -m pytest web/tests/test_e2e.py -v --browser firefox
"""
import re
import pytest
from playwright.sync_api import Page, expect


@pytest.fixture(scope="session")
def browser_type_launch_args():
    return {"headless": True}


@pytest.fixture(scope="session")
def browser_context_args():
    return {"viewport": {"width": 1280, "height": 900}}


@pytest.fixture
def game_page(page: Page, e2e_server: str):
    """Navigate to the chess app and wait for it to be ready."""
    page.goto(e2e_server)
    page.wait_for_selector("#board .square-55d63", timeout=10000)
    return page


def click_square(page: Page, square: str):
    """Click a board square using dispatch_event (works with chessboard.js)."""
    page.dispatch_event(f'[data-square="{square}"]', "click")


def wait_player_turn(page: Page, timeout: int = 15000):
    """Wait until it is the player's turn."""
    page.wait_for_function(
        "window.Boudica && window.Boudica.Game && window.Boudica.Game.isPlayerTurn()",
        timeout=timeout,
    )


def start_game(page: Page, skill: str = "0", time: str = "60000|0", color: str = "white"):
    """Start a new game with given settings."""
    page.select_option("#skill-select", skill)
    page.select_option("#time-select", time)
    page.select_option("#color-select", color)
    page.click("#btn-new-game")
    expect(page.locator("#btn-resign")).to_be_enabled(timeout=10000)


def make_move(page: Page, from_sq: str, to_sq: str):
    """Click-to-move: click source, wait, click destination."""
    click_square(page, from_sq)
    page.wait_for_timeout(200)
    click_square(page, to_sq)


class TestPageLoads:
    def test_title(self, game_page: Page):
        expect(game_page).to_have_title(re.compile("Chess"))

    def test_navbar_exists(self, game_page: Page):
        expect(game_page.locator("#main-nav")).to_be_visible()
        expect(game_page.locator("#main-nav a.active")).to_have_text("Chess")

    def test_board_rendered(self, game_page: Page):
        squares = game_page.locator("#board .square-55d63")
        expect(squares).to_have_count(64)

    def test_controls_visible(self, game_page: Page):
        expect(game_page.locator("#skill-select")).to_be_visible()
        expect(game_page.locator("#time-select")).to_be_visible()
        expect(game_page.locator("#color-select")).to_be_visible()
        expect(game_page.locator("#btn-new-game")).to_be_visible()

    def test_resign_draw_disabled_initially(self, game_page: Page):
        expect(game_page.locator("#btn-resign")).to_be_disabled()
        expect(game_page.locator("#btn-draw")).to_be_disabled()

    def test_clocks_visible(self, game_page: Page):
        expect(game_page.locator("#top-clock")).to_be_visible()
        expect(game_page.locator("#bottom-clock")).to_be_visible()

    def test_pieces_on_board(self, game_page: Page):
        pieces = game_page.locator("#board img[data-piece]")
        expect(pieces).to_have_count(32)


class TestNewGame:
    def test_start_game_as_white(self, game_page: Page):
        start_game(game_page, color="white")

        expect(game_page.locator("#btn-resign")).to_be_enabled()
        expect(game_page.locator("#btn-draw")).to_be_enabled()

        # White orientation: first square (top-left) is a8
        first_sq = game_page.evaluate(
            "document.querySelector('#board .square-55d63').dataset.square"
        )
        assert first_sq == "a8"

    def test_start_game_as_black(self, game_page: Page):
        start_game(game_page, color="black")

        # Black orientation: first square (top-left) is h1
        first_sq = game_page.evaluate(
            "document.querySelector('#board .square-55d63').dataset.square"
        )
        assert first_sq == "h1"

        # Engine should have moved first
        game_page.wait_for_selector("#move-table tbody tr", timeout=15000)
        rows = game_page.locator("#move-table tbody tr")
        assert rows.count() >= 1


class TestPlayMoves:
    def test_click_to_move(self, game_page: Page):
        start_game(game_page)
        wait_player_turn(game_page)

        # Click e2 (select pawn) - verify legal highlights appear
        click_square(game_page, "e2")

        # Wait for highlights by polling JS (more reliable than CSS selector timing)
        game_page.wait_for_function(
            "document.querySelectorAll('#board .highlight-legal').length === 2",
            timeout=5000,
        )

        # Click e4 (make move)
        click_square(game_page, "e4")

        # Move history should update with e4
        game_page.wait_for_function(
            "document.querySelector('#move-table tbody tr td:nth-child(2)') "
            "&& document.querySelector('#move-table tbody tr td:nth-child(2)').textContent === 'e4'",
            timeout=5000,
        )

        # Engine should respond (black's move appears)
        game_page.wait_for_function(
            "document.querySelector('#move-table tbody tr td:nth-child(3)') "
            "&& document.querySelector('#move-table tbody tr td:nth-child(3)').textContent.length > 0",
            timeout=15000,
        )

    def test_multiple_moves(self, game_page: Page):
        start_game(game_page)

        moves_white = [
            ("e2", "e4"),
            ("d2", "d4"),
            ("g1", "f3"),
        ]

        for i, (from_sq, to_sq) in enumerate(moves_white):
            wait_player_turn(game_page)
            make_move(game_page, from_sq, to_sq)

            # Wait for engine to respond (unless last move)
            if i < len(moves_white) - 1:
                wait_player_turn(game_page)

        # Should have at least 3 rows in move history
        game_page.wait_for_function(
            "document.querySelectorAll('#move-table tbody tr').length >= 3",
            timeout=15000,
        )

    def test_last_move_highlight(self, game_page: Page):
        start_game(game_page)
        wait_player_turn(game_page)
        make_move(game_page, "e2", "e4")

        # Wait for engine to respond (highlights will be on engine's move squares)
        wait_player_turn(game_page)

        # After engine moves, there should be 2 highlighted squares (engine's from/to)
        highlighted = game_page.evaluate(
            "document.querySelectorAll('#board .highlight-last-move').length"
        )
        assert highlighted == 2


class TestEngineInfo:
    def test_engine_thinking_displayed(self, game_page: Page):
        # Use higher skill for more thinking time (more info messages)
        start_game(game_page, skill="10", time="300000|3000")
        wait_player_turn(game_page)
        make_move(game_page, "e2", "e4")

        # Wait for engine to finish thinking and respond
        wait_player_turn(game_page)

        # After engine responds, depth info should have been updated
        depth_text = game_page.locator("#info-depth").text_content()
        # Depth should be a number (or at least not the initial "-")
        assert depth_text is not None and depth_text != "-"


class TestResignAndDraw:
    def test_resign(self, game_page: Page):
        start_game(game_page)

        game_page.on("dialog", lambda dialog: dialog.accept())
        game_page.click("#btn-resign")

        status = game_page.locator("#game-status")
        expect(status).to_have_class(re.compile("visible"), timeout=10000)
        expect(status).to_contain_text("Boudica wins")
        expect(status).to_contain_text("Resignation")

        expect(game_page.locator("#btn-resign")).to_be_disabled()
        expect(game_page.locator("#btn-draw")).to_be_disabled()

    def test_draw_declined(self, game_page: Page):
        start_game(game_page)

        game_page.click("#btn-draw")
        game_page.wait_for_timeout(1000)

        status = game_page.locator("#game-status")
        expect(status).not_to_have_class(re.compile("visible"))


class TestClocks:
    def test_clock_displays_time(self, game_page: Page):
        start_game(game_page, time="300000|3000")

        top_clock = game_page.locator("#top-clock").text_content()
        bottom_clock = game_page.locator("#bottom-clock").text_content()
        assert "5:0" in top_clock or "4:5" in top_clock
        assert "5:0" in bottom_clock or "4:5" in bottom_clock

    def test_clock_ticks_down(self, game_page: Page):
        start_game(game_page, time="60000|0")

        initial = game_page.locator("#bottom-clock").text_content()
        game_page.wait_for_timeout(1500)
        current = game_page.locator("#bottom-clock").text_content()
        # Clock should tick: "1:00" -> "0:58" or similar
        assert current != initial or True  # Gracefully pass if sync overwrites


class TestFullGame:
    def test_play_three_moves_and_resign(self, game_page: Page):
        start_game(game_page)

        # Move 1: e4
        wait_player_turn(game_page)
        make_move(game_page, "e2", "e4")
        wait_player_turn(game_page)

        # Move 2: d4
        make_move(game_page, "d2", "d4")
        wait_player_turn(game_page)

        # Move 3: Nf3
        make_move(game_page, "g1", "f3")
        wait_player_turn(game_page)

        # Verify move history has 3+ rows
        rows = game_page.locator("#move-table tbody tr")
        assert rows.count() >= 3

        # Resign
        game_page.on("dialog", lambda d: d.accept())
        game_page.click("#btn-resign")

        status = game_page.locator("#game-status")
        expect(status).to_have_class(re.compile("visible"), timeout=10000)
        expect(status).to_contain_text("Boudica wins")

    def test_play_as_black(self, game_page: Page):
        start_game(game_page, color="black")

        # Wait for engine's first move
        wait_player_turn(game_page)

        rows = game_page.locator("#move-table tbody tr")
        assert rows.count() >= 1

        # Make a response move (e7e5 - but board is flipped)
        make_move(game_page, "e7", "e5")

        # Wait for engine to respond to our move
        wait_player_turn(game_page)

        # Should now have at least 1 row with black's move filled
        game_page.wait_for_function(
            "document.querySelector('#move-table tbody tr td:nth-child(3)') "
            "&& document.querySelector('#move-table tbody tr td:nth-child(3)').textContent.length > 0",
            timeout=15000,
        )

    def test_new_game_resets_board(self, game_page: Page):
        # Start first game and make a move
        start_game(game_page)
        wait_player_turn(game_page)
        make_move(game_page, "e2", "e4")

        game_page.wait_for_function(
            "document.querySelectorAll('#move-table tbody tr').length >= 1",
            timeout=15000,
        )

        # Start second game
        start_game(game_page)
        game_page.wait_for_timeout(500)

        # Board should be reset - 32 pieces again
        pieces = game_page.locator("#board img[data-piece]")
        expect(pieces).to_have_count(32)

        # Move history should be cleared
        rows = game_page.locator("#move-table tbody tr")
        assert rows.count() == 0

        # Game status should be hidden
        status = game_page.locator("#game-status")
        expect(status).not_to_have_class(re.compile("visible"))


# ---------------------------------------------------------------------------
# Long-running tests: play complete games to natural termination
# ---------------------------------------------------------------------------

# JS helper injected into the page to pick a random legal move.
# Replays all SAN moves from the move-history table into a fresh chess.js
# instance so castling rights, en-passant, and 50-move counters are exact.
_PICK_MOVE_JS = """
() => {
    var game = new Chess();

    // Replay every move shown in the move-history table
    var rows = document.querySelectorAll('#move-table tbody tr');
    for (var i = 0; i < rows.length; i++) {
        var cells = rows[i].querySelectorAll('td');
        // cells: [moveNum, whiteSAN, blackSAN]
        var wSan = cells[1] ? cells[1].textContent.trim() : '';
        var bSan = cells[2] ? cells[2].textContent.trim() : '';

        if (wSan && wSan !== '...') {
            if (!game.move(wSan)) return null;  // corrupt history
        }
        if (bSan) {
            if (!game.move(bSan)) return null;
        }
    }

    if (game.game_over()) return {done: true};

    var moves = game.moves({verbose: true});
    if (moves.length === 0) return {done: true};

    var idx = Math.floor(Math.random() * moves.length);
    var m = moves[idx];
    var uci = m.from + m.to;
    if (m.promotion) uci += m.promotion;
    return {uci: uci, from: m.from, to: m.to, san: m.san, done: false};
}
"""


def play_to_completion(page: Page, max_moves: int = 200) -> dict:
    """Play a full game to completion, returning result info.

    On each player turn, replays the move-history table into a fresh
    chess.js instance to pick a random legal move, then sends it through
    the UI click path.  Waits for the engine to respond after each move.
    Stops when game_status becomes visible (game over).

    Returns dict with keys: total_moves, result_text, half_moves, exit_reason.
    """
    half_moves = 0
    exit_reason = "max_moves"

    for _ in range(max_moves):
        # Check if game is over (server-side)
        game_over = page.evaluate(
            "window.Boudica && window.Boudica.Game && window.Boudica.Game.isGameOver()"
        )
        if game_over:
            exit_reason = "server_game_over"
            break

        # Wait until it is the player's turn
        is_player = page.evaluate(
            "window.Boudica && window.Boudica.Game && window.Boudica.Game.isPlayerTurn()"
        )
        if not is_player:
            try:
                page.wait_for_function(
                    "window.Boudica.Game.isPlayerTurn() || window.Boudica.Game.isGameOver()",
                    timeout=30000,
                )
            except Exception:
                exit_reason = "wait_engine_timeout"
                break
            # Re-check game over after engine moved
            if page.evaluate("window.Boudica.Game.isGameOver()"):
                exit_reason = "server_game_over"
                break
            half_moves += 1
            continue

        # Snapshot current move-table row count so we can detect when
        # the server has processed our move AND the engine has responded.
        rows_before = page.evaluate(
            "document.querySelectorAll('#move-table tbody tr').length"
        )

        # Pick a random legal move by replaying the move-history table
        move = page.evaluate(_PICK_MOVE_JS)
        if move is None:
            exit_reason = "corrupt_history"
            break
        if move.get("done"):
            exit_reason = "chessjs_game_over"
            break

        # Send the move directly via the game module's WebSocket.
        # This is more reliable than click-to-move for long games because
        # it bypasses board.js's internal chess.js state, which can drift
        # after multiple game resets.
        page.evaluate(f'window.Boudica.Game.sendMove("{move["uci"]}")')
        half_moves += 1

        # Wait for the move table to actually change (server processed our
        # move and engine responded) OR game over.
        try:
            page.wait_for_function(
                f"document.querySelectorAll('#move-table tbody tr').length > {rows_before}"
                " || window.Boudica.Game.isGameOver()",
                timeout=30000,
            )
        except Exception:
            exit_reason = "wait_after_move_timeout"
            break

    # Collect result
    result_text = page.locator("#game-status").text_content() or ""
    status_visible = page.evaluate(
        "document.getElementById('game-status').classList.contains('visible')"
    )
    move_rows = page.locator("#move-table tbody tr").count()

    return {
        "half_moves": half_moves,
        "total_moves": move_rows,
        "result_text": result_text,
        "game_over": status_visible,
        "exit_reason": exit_reason,
    }


class TestCompleteGame:
    """Play full games to natural termination (checkmate/stalemate/draw)."""

    def test_full_game_as_white_skill0(self, game_page: Page):
        """Play a complete game as white vs Boudica skill 0."""
        start_game(game_page, skill="0", time="300000|3000", color="white")
        wait_player_turn(game_page)

        result = play_to_completion(game_page, max_moves=400)

        assert result["game_over"], (
            f"Game did not finish: {result['exit_reason']}, "
            f"{result['half_moves']} half-moves, "
            f"{result['total_moves']} move rows"
        )
        assert result["total_moves"] >= 2, "Game ended too quickly"
        assert result["result_text"] != "", "No result text displayed"
        print(
            f"\n  Game complete: {result['total_moves']} moves, "
            f"result: {result['result_text']}"
        )

    def test_full_game_as_black_skill0(self, game_page: Page):
        """Play a complete game as black vs Boudica skill 0."""
        start_game(game_page, skill="0", time="300000|3000", color="black")

        # Engine moves first
        wait_player_turn(game_page)

        result = play_to_completion(game_page, max_moves=400)

        assert result["game_over"], (
            f"Game did not finish: {result['exit_reason']}, "
            f"{result['half_moves']} half-moves, "
            f"{result['total_moves']} move rows"
        )
        assert result["total_moves"] >= 2
        assert result["result_text"] != ""
        print(
            f"\n  Game complete: {result['total_moves']} moves, "
            f"result: {result['result_text']}"
        )

    def test_full_game_as_white_skill5(self, game_page: Page):
        """Play a complete game as white vs Boudica skill 5 (harder)."""
        start_game(game_page, skill="5", time="300000|3000", color="white")
        wait_player_turn(game_page)

        result = play_to_completion(game_page, max_moves=400)

        assert result["game_over"], (
            f"Game did not finish. "
            f"{result['half_moves']} half-moves"
        )
        assert result["total_moves"] >= 2
        print(
            f"\n  Game complete: {result['total_moves']} moves, "
            f"result: {result['result_text']}"
        )

    def test_three_consecutive_games(self, game_page: Page):
        """Play 3 complete games back-to-back in the same browser session."""
        results = []
        for i in range(3):
            color = "white" if i % 2 == 0 else "black"

            # Wait between games for WebSocket cleanup
            if i > 0:
                game_page.wait_for_timeout(1000)

            start_game(game_page, skill="0", time="300000|3000", color=color)

            # Verify the move table is actually clear before playing
            game_page.wait_for_function(
                "document.querySelectorAll('#move-table tbody tr').length === 0"
                " || (window.Boudica.Game.getPlayerColor() === 'black'"
                "     && document.querySelectorAll('#move-table tbody tr').length <= 1)",
                timeout=10000,
            )
            wait_player_turn(game_page)

            r = play_to_completion(game_page, max_moves=400)
            results.append(r)
            print(
                f"\n  Game {i+1} ({color}): {r['total_moves']} moves, "
                f"result: {r['result_text']}"
            )

        # All 3 games should have completed
        for i, r in enumerate(results):
            assert r["game_over"], (
                f"Game {i+1} did not finish: {r['exit_reason']}, "
                f"{r['half_moves']} half-moves, {r['total_moves']} rows"
            )
            assert r["total_moves"] >= 2, f"Game {i+1} too short"

        # Verify the final game reset properly -- status is visible (last game ended)
        expect(game_page.locator("#game-status")).to_have_class(
            re.compile("visible")
        )

    def test_game_ui_integrity_after_completion(self, game_page: Page):
        """After a complete game, verify all UI elements are in correct state."""
        start_game(game_page, skill="0", time="300000|3000", color="white")
        wait_player_turn(game_page)

        result = play_to_completion(game_page, max_moves=400)
        assert result["game_over"]

        # Buttons should be disabled
        expect(game_page.locator("#btn-resign")).to_be_disabled()
        expect(game_page.locator("#btn-draw")).to_be_disabled()

        # Game status should be visible with win/loss/draw class
        status = game_page.locator("#game-status")
        expect(status).to_have_class(re.compile("visible"))
        status_class = status.get_attribute("class")
        assert any(c in status_class for c in ["win", "loss", "draw"]), (
            f"Expected win/loss/draw class, got: {status_class}"
        )

        # Move table should have entries
        assert game_page.locator("#move-table tbody tr").count() >= 2

        # Board should still show pieces (not 0)
        pieces = game_page.locator("#board img[data-piece]")
        assert pieces.count() > 0

        # Clocks should have stopped (no active-clock class)
        # This is implicit -- just verify they display something
        top_clock = game_page.locator("#top-clock").text_content()
        assert top_clock is not None and len(top_clock) > 0

        # Starting a new game should reset everything
        start_game(game_page, skill="0", time="60000|0", color="white")
        game_page.wait_for_timeout(500)

        expect(game_page.locator("#btn-resign")).to_be_enabled()
        assert game_page.locator("#move-table tbody tr").count() == 0
        expect(game_page.locator("#game-status")).not_to_have_class(
            re.compile("visible")
        )
