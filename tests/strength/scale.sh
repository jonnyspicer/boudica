#!/usr/bin/env bash
# Measure this machine against CCRL's reference (Stockfish 10 bench on an
# i7-4770k: ~2054 ms, 3,939,338 nodes) and write the time-control scale factor.
# Runs CONCURRENCY benches at once so the machine is as loaded as during games.
#
# Usage: tests/strength/scale.sh [concurrency]
# Output: $CHESS_DIR/scale.env with SCALE_F, TC_4040, TC_BLITZ
set -euo pipefail

CHESS_DIR="${CHESS_DIR:-/opt/chess}"
CONCURRENCY="${1:-$(( $(nproc) > 1 ? $(nproc) - 1 : 1 ))}"
SF="$CHESS_DIR/bin/stockfish10"
REF_MS=2054
REF_NODES=3939338
RUNS=5

times=()
for run in $(seq $RUNS); do
    tmp=$(mktemp -d)
    for i in $(seq "$CONCURRENCY"); do
        "$SF" bench > "$tmp/$i.out" 2>&1 &
    done
    wait
    # Stockfish 10 prints its bench summary to stderr
    nodes=$(sed -n 's/^Nodes searched *: //p' "$tmp/1.out")
    [[ "$nodes" == "$REF_NODES" ]] || { echo "wrong Stockfish 10 build: $nodes nodes" >&2; exit 1; }
    # Median over the concurrent copies of this run
    t=$(sed -n 's/^Total time (ms) *: //p' "$tmp"/*.out | sort -n | awk '{a[NR]=$1} END {print a[int((NR+1)/2)]}')
    times+=("$t")
    rm -rf "$tmp"
done

median=$(printf '%s\n' "${times[@]}" | sort -n | awk '{a[NR]=$1} END {print a[int((NR+1)/2)]}')
f=$(awk -v t="$median" -v r=$REF_MS 'BEGIN {printf "%.3f", t / r}')
tc_4040=$(awk -v f="$f" 'BEGIN {printf "40/%.1f", 900 * f}')
tc_blitz=$(awk -v f="$f" 'BEGIN {printf "%.1f+%.2f", 120 * f, 1 * f}')

cat > "$CHESS_DIR/scale.env" <<EOF
# $(date -u +%Y-%m-%dT%H:%MZ) $(hostname) concurrency=$CONCURRENCY bench_ms=${times[*]}
SCALE_F=$f
TC_4040=$tc_4040
TC_BLITZ=$tc_blitz
EOF
cat "$CHESS_DIR/scale.env"
