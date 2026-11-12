#!/usr/bin/env python3
"""Turn PGN games into Texel tuning lines: "<FEN> | <result>", result from White's side.

Skips the book plies, positions in check, and positions where the move played
was a capture or promotion (rarely quiet). The tuner's --filter-quiet does the
exact quiet check afterwards.

Usage: pgn2epd.py games.pgn [skip_plies=16] > raw.epd
"""
import sys

import chess
import chess.pgn

RESULTS = {"1-0": "1.0", "0-1": "0.0", "1/2-1/2": "0.5"}


def main():
    path = sys.argv[1]
    skip = int(sys.argv[2]) if len(sys.argv) > 2 else 16
    with open(path, encoding="utf-8", errors="replace") as f:
        while (game := chess.pgn.read_game(f)) is not None:
            result = RESULTS.get(game.headers.get("Result", "*"))
            if result is None:
                continue
            board = game.board()
            for ply, move in enumerate(game.mainline_moves()):
                if (ply >= skip and not board.is_check()
                        and not board.is_capture(move) and not move.promotion):
                    print(f"{board.fen()} | {result}")
                board.push(move)


if __name__ == "__main__":
    main()
