#!/usr/bin/env bash
# Tier 0 smoke match: fast games between two Boudica builds. Fails if any game
# ends in a time loss, illegal move, crash or stalled engine.
#
# Usage: tests/strength/smoke.sh DEV_BINARY BASE_BINARY [games=200] [concurrency]
set -euo pipefail

DEV="$1"; BASE="$2"
GAMES="${3:-200}"
CONCURRENCY="${4:-$(( $(nproc) > 1 ? $(nproc) - 1 : 1 ))}"
CHESS_DIR="${CHESS_DIR:-/opt/chess}"
OUT="${OUT:-$CHESS_DIR/results/smoke-$(date -u +%Y%m%dT%H%M%S)}"
mkdir -p "$OUT"

# No -recover: a crash must end the match, not be retried
"$CHESS_DIR/bin/fastchess" \
    -engine cmd="$DEV" name=dev \
    -engine cmd="$BASE" name=base \
    -each tc=5+0.05 option.Hash=16 \
    -openings file="$CHESS_DIR/books/UHO_Lichess_4852_v1.epd" format=epd order=random \
    -repeat -rounds $(( GAMES / 2 )) -concurrency "$CONCURRENCY" \
    -pgnout file="$OUT/games.pgn" \
    | tee "$OUT/fastchess.log"

# Adjudications other than normal results show up in the Termination tag
bad=$(grep -ciE 'Termination "(time forfeit|illegal move|abandoned|stalled connection)' "$OUT/games.pgn" || true)
played=$(grep -c '^\[Result ' "$OUT/games.pgn" || true)
echo "Smoke: $played games, $bad bad terminations (log: $OUT)"
if [[ $played -lt $GAMES || $bad -gt 0 ]]; then
    grep -iE 'Termination "(time forfeit|illegal move|abandoned|stalled connection)' "$OUT/games.pgn" | sort | uniq -c || true
    echo "SMOKE FAILED"
    exit 1
fi
echo "SMOKE PASSED"
