#!/usr/bin/env bash
# Build the anchor engines for one rung from anchors.tsv into $CHESS_DIR/anchors/<id>.
#
# Usage: tests/strength/build-anchors.sh RUNG
set -euo pipefail

RUNG="$1"
CHESS_DIR="${CHESS_DIR:-/opt/chess}"
HERE="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$CHESS_DIR/anchors" "$CHESS_DIR/src/anchors"

grep -v '^#' "$HERE/anchors.tsv" | while IFS=$'\t' read -r id rung name _r4040 _rblitz repo ref build binary _opts; do
    [[ "$rung" == "$RUNG" ]] || continue
    if [[ "$repo" == "TBD" ]]; then
        echo "skip $id: source not confirmed yet"
        continue
    fi
    src="$CHESS_DIR/src/anchors/$id"
    if [[ ! -d "$src" ]]; then
        git clone -q "$repo" "$src"
    fi
    git -C "$src" checkout -q "$ref"
    (cd "$src" && bash -c "$build") >"$src.build.log" 2>&1 || { echo "FAILED $id (see $src.build.log)"; exit 1; }
    cp "$src/$binary" "$CHESS_DIR/anchors/$id"
    echo "built $id ($name) at $(git -C "$src" rev-parse --short HEAD)"
    echo "anchor $id $repo $(git -C "$src" rev-parse --short HEAD)" >> "$CHESS_DIR/versions.txt"
done
