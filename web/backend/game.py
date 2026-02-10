"""Game state management with python-chess validation and clock tracking."""
import time
import uuid

import chess


class ClockManager:
    """Server-authoritative chess clock."""

    def __init__(self, time_ms: int = 300000, increment_ms: int = 3000):
        self.remaining = {chess.WHITE: time_ms, chess.BLACK: time_ms}
        self.increment = increment_ms
        self._running_color: chess.Color | None = None
        self._last_tick: float = 0.0

    def start(self, color: chess.Color) -> None:
        self._running_color = color
        self._last_tick = time.monotonic()

    def stop_and_increment(self) -> None:
        if self._running_color is not None:
            elapsed_ms = int((time.monotonic() - self._last_tick) * 1000)
            self.remaining[self._running_color] -= elapsed_ms
            self.remaining[self._running_color] += self.increment
            self.remaining[self._running_color] = max(
                0, self.remaining[self._running_color]
            )
            self._running_color = None

    def is_flag_fallen(self, color: chess.Color) -> bool:
        if self._running_color == color:
            elapsed_ms = int((time.monotonic() - self._last_tick) * 1000)
            return (self.remaining[color] - elapsed_ms) <= 0
        return self.remaining[color] <= 0

    def get_remaining(self, color: chess.Color) -> int:
        if self._running_color == color:
            elapsed_ms = int((time.monotonic() - self._last_tick) * 1000)
            return max(0, self.remaining[color] - elapsed_ms)
        return max(0, self.remaining[color])

    def to_dict(self) -> dict:
        return {
            "white": {"remaining_ms": self.get_remaining(chess.WHITE)},
            "black": {"remaining_ms": self.get_remaining(chess.BLACK)},
        }


class GameManager:
    """Manages a single chess game with validation and state tracking."""

    def __init__(
        self,
        player_color: str = "white",
        skill_level: int = 20,
        time_ms: int = 300000,
        increment_ms: int = 3000,
    ):
        self.game_id = str(uuid.uuid4())
        self.board = chess.Board()
        self.player_color = chess.WHITE if player_color == "white" else chess.BLACK
        self.engine_color = not self.player_color
        self.skill_level = skill_level
        self.clock = ClockManager(time_ms, increment_ms)
        self.moves: list[str] = []  # UCI move strings
        self.result: str | None = None
        self.termination: str | None = None

    @property
    def is_player_turn(self) -> bool:
        return self.board.turn == self.player_color

    @property
    def is_engine_turn(self) -> bool:
        return self.board.turn == self.engine_color

    @property
    def fen(self) -> str:
        return self.board.fen()

    def try_player_move(self, uci_str: str) -> tuple[bool, str, str]:
        """Validate and apply a player move. Returns (ok, san, error_msg)."""
        if not self.is_player_turn:
            return False, "", "not_your_turn"
        if self.result is not None:
            return False, "", "game_over"

        # Check flag
        if self.clock.is_flag_fallen(self.player_color):
            self._set_result_flag(self.player_color)
            return False, "", "flag_fallen"

        try:
            move = chess.Move.from_uci(uci_str)
        except (ValueError, chess.InvalidMoveError):
            return False, "", "invalid_move_format"

        if move not in self.board.legal_moves:
            return False, "", "illegal_move"

        san = self.board.san(move)
        self.clock.stop_and_increment()
        self.board.push(move)
        self.moves.append(uci_str)
        self._check_game_over()

        # Start engine clock
        if self.result is None:
            self.clock.start(self.engine_color)

        return True, san, ""

    def apply_engine_move(self, uci_str: str) -> str:
        """Apply engine's chosen move. Returns SAN."""
        move = chess.Move.from_uci(uci_str)
        san = self.board.san(move)
        self.clock.stop_and_increment()
        self.board.push(move)
        self.moves.append(uci_str)
        self._check_game_over()

        # Start player clock
        if self.result is None:
            self.clock.start(self.player_color)

        return san

    def resign(self, resigning_color: chess.Color) -> None:
        if resigning_color == chess.WHITE:
            self.result = "black_wins"
        else:
            self.result = "white_wins"
        self.termination = "resignation"

    def offer_draw(self) -> bool:
        """Engine always declines draws for now."""
        return False

    def _check_game_over(self) -> None:
        if self.board.is_checkmate():
            if self.board.turn == chess.WHITE:
                self.result = "black_wins"
            else:
                self.result = "white_wins"
            self.termination = "checkmate"
        elif self.board.is_stalemate():
            self.result = "draw"
            self.termination = "stalemate"
        elif self.board.is_insufficient_material():
            self.result = "draw"
            self.termination = "insufficient_material"
        elif self.board.is_fifty_moves():
            self.result = "draw"
            self.termination = "fifty_moves"
        elif self.board.is_repetition(3):
            self.result = "draw"
            self.termination = "threefold_repetition"

    def _set_result_flag(self, flagged_color: chess.Color) -> None:
        if flagged_color == chess.WHITE:
            self.result = "black_wins"
        else:
            self.result = "white_wins"
        self.termination = "flag"

    def get_engine_clock_params(self) -> dict:
        """Get clock values formatted for UCI go command."""
        return {
            "wtime": self.clock.get_remaining(chess.WHITE),
            "btime": self.clock.get_remaining(chess.BLACK),
            "winc": self.clock.increment,
            "binc": self.clock.increment,
        }
