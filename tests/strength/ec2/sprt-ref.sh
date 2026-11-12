#!/usr/bin/env bash
# Run an SPRT between two git refs of the GitHub repo, on the test box.
# Each ref is built once into $CHESS_DIR/builds/<sha>/boudica and reused.
#
# Usage: tests/strength/ec2/sprt-ref.sh DEV_REF [BASE_REF=main] [elo0=0] [elo1=5] [tc=10+0.1]
#   e.g. sprt-ref.sh origin/lmr-history origin/main
# Env:   REPO (https://github.com/jonnyspicer/boudica.git), CHESS_DIR (/opt/chess), CONCURRENCY
# Writes the result directory to $CHESS_DIR/results/sprt-<dev>-vs-<base>-<time>.
set -euo pipefail

DEV_REF="$1"; BASE_REF="${2:-origin/main}"
shift $(( $# < 2 ? $# : 2 ))
REPO="${REPO:-https://github.com/jonnyspicer/boudica.git}"
CHESS_DIR="${CHESS_DIR:-/opt/chess}"
SRC="$CHESS_DIR/src/boudica"
HERE="$(cd "$(dirname "$0")/.." && pwd)"

[[ -d "$SRC/.git" ]] || git clone -q "$REPO" "$SRC"
git -C "$SRC" fetch -q --prune origin

build() {  # ref -> prints the binary path
    local sha dir
    sha=$(git -C "$SRC" rev-parse --short=10 "$1^{commit}")
    dir="$CHESS_DIR/builds/$sha"
    if [[ ! -x "$dir/boudica" ]]; then
        rm -rf "$dir.tmp" && mkdir -p "$dir.tmp"
        git -C "$SRC" archive "$sha" | tar -x -C "$dir.tmp"
        make -C "$dir.tmp" -s CXX=g++ >/dev/null
        mv "$dir.tmp" "$dir"
    fi
    echo "$dir/boudica"
}

dev=$(build "$DEV_REF"); base=$(build "$BASE_REF")
name() { basename "$1" | tr '/' '-'; }
export OUT="$CHESS_DIR/results/sprt-$(name "$DEV_REF")-vs-$(name "$BASE_REF")-$(date -u +%Y%m%dT%H%M%S)"
mkdir -p "$OUT"
printf 'dev_ref=%s dev=%s\nbase_ref=%s base=%s\n' \
    "$DEV_REF" "$(git -C "$SRC" rev-parse --short "$DEV_REF")" \
    "$BASE_REF" "$(git -C "$SRC" rev-parse --short "$BASE_REF")" > "$OUT/refs.txt"
exec "$HERE/sprt.sh" "$dev" "$base" "$@"
