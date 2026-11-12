#!/usr/bin/env bash
# Generate Texel tuning data: fast fixed-node self-play from a balanced book,
# converted to "<FEN> | <result>" lines and filtered to quiet positions.
#
# Usage: tests/strength/datagen.sh ENGINE [games=40000] [nodes=20000]
# Env:   CHESS_DIR (/opt/chess), CONCURRENCY (nproc - 1), OUT (results dir)
# Output: $OUT/games.pgn, $OUT/raw.epd, $OUT/quiet.epd
set -euo pipefail

ENGINE="$(realpath "$1")"; GAMES="${2:-40000}"; NODES="${3:-20000}"
CHESS_DIR="${CHESS_DIR:-/opt/chess}"
CONCURRENCY="${CONCURRENCY:-$(( $(nproc) > 1 ? $(nproc) - 1 : 1 ))}"
OUT="${OUT:-$CHESS_DIR/results/datagen-$(date -u +%Y%m%dT%H%M%S)}"
HERE="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$OUT"
echo "engine Bench: $("$ENGINE" bench | sed -n 's/^Nodes searched *: //p') games $GAMES nodes $NODES" | tee "$OUT/header.txt"

# Adjudication keeps games short; the labels are still the game results
"$CHESS_DIR/bin/fastchess" \
    -engine cmd="$ENGINE" name=a \
    -engine cmd="$ENGINE" name=b \
    -each tc=inf nodes="$NODES" option.Hash=16 \
    -openings file="$CHESS_DIR/books/8moves_v3.pgn" format=pgn order=random \
    -rounds "$GAMES" -concurrency "$CONCURRENCY" \
    -draw movenumber=40 movecount=8 score=10 \
    -resign movecount=4 score=1000 twosided=true \
    -recover -pgnout file="$OUT/games.pgn" > "$OUT/fastchess.log" 2>&1

# python-chess: pip install chess (Ubuntu has no package for it)
python3 "$HERE/pgn2epd.py" "$OUT/games.pgn" > "$OUT/raw.epd"
"$HERE/../../tuner" --filter-quiet --epd "$OUT/raw.epd" --output "$OUT/quiet.epd"
wc -l "$OUT/raw.epd" "$OUT/quiet.epd"
