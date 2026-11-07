#!/usr/bin/env bash
# Tier 2/3 gauntlet: Boudica against one rung of CCRL-rated anchors, then an
# Ordo rating with every anchor fixed at its CCRL rating.
#
# Usage: tests/strength/gauntlet.sh BOUDICA_BINARY RUNG (blitz|4040) [games_per_anchor=300]
# Needs: setup-tools.sh --with-tb --with-sf10, scale.sh, build-anchors.sh RUNG
set -euo pipefail

BOUDICA="$1"; RUNG="$2"; LIST="$3"; PER_ANCHOR="${4:-300}"
CHESS_DIR="${CHESS_DIR:-/opt/chess}"
HERE="$(cd "$(dirname "$0")" && pwd)"
CONCURRENCY="${CONCURRENCY:-$(( $(nproc) > 1 ? $(nproc) - 1 : 1 ))}"
OUT="${OUT:-$CHESS_DIR/results/gauntlet-$LIST-$RUNG-$(date -u +%Y%m%dT%H%M%S)}"
mkdir -p "$OUT"

source "$CHESS_DIR/scale.env"
case "$LIST" in
    blitz) TC="$TC_BLITZ"; col=5 ;;
    4040)  TC="$TC_4040";  col=4 ;;
    *) echo "list must be blitz or 4040" >&2; exit 2 ;;
esac

# Anchor engines and Ordo's fixed-rating file, for anchors rated on this list
engines=()
: > "$OUT/anchors.csv"
while IFS=$'\t' read -r -a f; do
    id="${f[0]}"; rating="${f[$((col - 1))]}"
    [[ "${f[1]}" == "$RUNG" && "$rating" != "-" && -x "$CHESS_DIR/anchors/$id" ]] || continue
    engine=(-engine cmd="$CHESS_DIR/anchors/$id" name="$id")
    opts="${f[9]:--}"
    if [[ "$opts" != "-" ]]; then
        read -r -a extra <<< "${opts//@TB@/$CHESS_DIR/syzygy}"
        engine+=("${extra[@]}")
    fi
    engines+=("${engine[@]}")
    echo "\"$id\",$rating" >> "$OUT/anchors.csv"
done < <(grep -v '^#' "$HERE/anchors.tsv")
[[ ${#engines[@]} -gt 0 ]] || { echo "no built anchors rated on $LIST in rung $RUNG" >&2; exit 1; }

bench=$("$BOUDICA" bench | sed -n 's/^Nodes searched *: //p')
{
    echo "Boudica Bench: $bench  list: $LIST  rung: $RUNG  tc: $TC  scale f=$SCALE_F"
    echo "anchors: $(cut -d, -f1 "$OUT/anchors.csv" | tr -d '"' | tr '\n' ' ')"
    cat "$CHESS_DIR/versions.txt"
} > "$OUT/header.txt"
cat "$OUT/header.txt"

# CCRL conditions: same hash for all, ponder off, engine books off, 3-4-5 TB
# for every engine that supports them. Boudica first so it is the gauntlet seed.
"$CHESS_DIR/bin/fastchess" \
    -tournament gauntlet -seeds 1 \
    -engine cmd="$BOUDICA" name=boudica option.OwnBook=false \
    "${engines[@]}" \
    -each tc="$TC" option.Hash=256 \
    -openings file="$CHESS_DIR/books/8moves_v3.pgn" format=pgn order=random \
    -repeat -rounds $(( PER_ANCHOR / 2 )) -concurrency "$CONCURRENCY" -use-affinity \
    -tb "$CHESS_DIR/syzygy" -tbpieces 5 \
    -draw movenumber=40 movecount=8 score=10 \
    -resign movecount=3 score=1000 twosided=true \
    -report penta=true \
    -config outname="$OUT/config.json" \
    -pgnout file="$OUT/games.pgn" \
    | tee "$OUT/fastchess.log"

"$CHESS_DIR/bin/ordo" -p "$OUT/games.pgn" -m "$OUT/anchors.csv" -W -D -s 1000 \
    -o "$OUT/ratings.txt" -j "$OUT/h2h.txt"
cat "$OUT/ratings.txt"
echo "Results in $OUT. Resume an interrupted run with: fastchess -config file=$OUT/config.json"
