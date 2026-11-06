#!/usr/bin/env bash
# Tier 1 SPRT: does DEV beat BASE? This alone decides whether a change lands.
#
# Usage: tests/strength/sprt.sh DEV_BINARY BASE_BINARY [elo0=0] [elo1=5] [tc=10+0.1]
#   Non-regression test for simplifications: sprt.sh dev base -3 1
#   Long time control confirmation:           sprt.sh dev base 0 5 60+0.6
set -euo pipefail

DEV="$1"; BASE="$2"
ELO0="${3:-0}"; ELO1="${4:-5}"; TC="${5:-10+0.1}"
CONCURRENCY="${CONCURRENCY:-$(( $(nproc) > 1 ? $(nproc) - 1 : 1 ))}"
CHESS_DIR="${CHESS_DIR:-/opt/chess}"
OUT="${OUT:-$CHESS_DIR/results/sprt-$(date -u +%Y%m%dT%H%M%S)}"
mkdir -p "$OUT"

# Bench signatures identify exactly which builds were compared
dev_bench=$("$DEV" bench | sed -n 's/^Nodes searched *: //p')
base_bench=$("$BASE" bench | sed -n 's/^Nodes searched *: //p')
echo "dev Bench: $dev_bench  base Bench: $base_bench  bounds [$ELO0, $ELO1]  tc $TC" | tee "$OUT/header.txt"

"$CHESS_DIR/bin/fastchess" \
    -engine cmd="$DEV" name=dev \
    -engine cmd="$BASE" name=base \
    -each tc="$TC" option.Hash=16 \
    -openings file="$CHESS_DIR/books/UHO_Lichess_4852_v1.epd" format=epd order=random \
    -repeat -rounds 100000 -concurrency "$CONCURRENCY" \
    -sprt elo0="$ELO0" elo1="$ELO1" alpha=0.05 beta=0.05 \
    -report penta=true \
    -config outname="$OUT/config.json" \
    -pgnout file="$OUT/games.pgn" \
    | tee "$OUT/fastchess.log"

# Resume an interrupted run with:
#   fastchess -config file=$OUT/config.json
echo "Finished. Results in $OUT. Add a line to tests/strength/RESULTS.md."
