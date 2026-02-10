"""Tests for GameManager and ClockManager."""
import time
import chess
import pytest
from web.backend.game import GameManager, ClockManager


class TestClockManager:
    def test_initial_remaining(self):
        clock = ClockManager(time_ms=300000, increment_ms=3000)
        assert clock.get_remaining(chess.WHITE) == 300000
        assert clock.get_remaining(chess.BLACK) == 300000

    def test_start_and_stop(self):
        clock = ClockManager(time_ms=300000, increment_ms=0)
        clock.start(chess.WHITE)
        time.sleep(0.05)  # 50ms
        clock.stop_and_increment()
        remaining = clock.get_remaining(chess.WHITE)
        assert remaining < 300000
        assert remaining > 299000  # Should have lost ~50ms

    def test_increment_on_stop(self):
        clock = ClockManager(time_ms=300000, increment_ms=3000)
        clock.start(chess.WHITE)
        time.sleep(0.01)
        clock.stop_and_increment()
        # Should gain ~3000ms increment minus ~10ms elapsed
        remaining = clock.get_remaining(chess.WHITE)
        assert remaining > 300000  # Gained increment

    def test_flag_fallen(self):
        clock = ClockManager(time_ms=50, increment_ms=0)
        clock.start(chess.WHITE)
        time.sleep(0.1)  # 100ms > 50ms
        assert clock.is_flag_fallen(chess.WHITE)

    def test_to_dict(self):
        clock = ClockManager(time_ms=300000, increment_ms=3000)
        d = clock.to_dict()
        assert "white" in d
        assert "black" in d
        assert d["white"]["remaining_ms"] == 300000


class TestGameManager:
    def test_new_game_white(self):
        game = GameManager(player_color="white")
        assert game.is_player_turn
        assert not game.is_engine_turn
        assert game.fen == chess.STARTING_FEN

    def test_new_game_black(self):
        game = GameManager(player_color="black")
        assert not game.is_player_turn
        assert game.is_engine_turn

    def test_valid_player_move(self):
        game = GameManager(player_color="white")
        ok, san, err = game.try_player_move("e2e4")
        assert ok
        assert san == "e4"
        assert err == ""
        assert not game.is_player_turn  # Now engine's turn

    def test_illegal_move(self):
        game = GameManager(player_color="white")
        ok, san, err = game.try_player_move("e2e5")  # Pawn can't go 3 squares
        assert not ok
        assert err == "illegal_move"

    def test_not_your_turn(self):
        game = GameManager(player_color="black")
        ok, san, err = game.try_player_move("e2e4")
        assert not ok
        assert err == "not_your_turn"

    def test_invalid_format(self):
        game = GameManager(player_color="white")
        ok, san, err = game.try_player_move("xyz")
        assert not ok
        assert err == "invalid_move_format"

    def test_apply_engine_move(self):
        game = GameManager(player_color="white")
        game.try_player_move("e2e4")
        san = game.apply_engine_move("e7e5")
        assert san == "e5"
        assert game.is_player_turn

    def test_resign(self):
        game = GameManager(player_color="white")
        game.resign(chess.WHITE)
        assert game.result == "black_wins"
        assert game.termination == "resignation"

    def test_draw_declined(self):
        game = GameManager(player_color="white")
        assert not game.offer_draw()

    def test_checkmate_detection(self):
        """Scholar's mate."""
        game = GameManager(player_color="white")
        moves = [
            ("e2e4", True), ("e7e5", None),
            ("d1h5", True), ("b8c6", None),
            ("f1c4", True), ("g8f6", None),
            ("h5f7", True),
        ]
        for uci, is_player in moves:
            if is_player:
                ok, _, _ = game.try_player_move(uci)
                assert ok, f"Failed: {uci}"
            else:
                game.apply_engine_move(uci)

        assert game.result == "white_wins"
        assert game.termination == "checkmate"

    def test_game_over_blocks_moves(self):
        game = GameManager(player_color="white")
        game.resign(chess.WHITE)
        ok, _, err = game.try_player_move("e2e4")
        assert not ok
        assert err == "game_over"

    def test_clock_params(self):
        game = GameManager(time_ms=300000, increment_ms=3000)
        params = game.get_engine_clock_params()
        assert params["wtime"] == 300000
        assert params["btime"] == 300000
        assert params["winc"] == 3000
        assert params["binc"] == 3000
