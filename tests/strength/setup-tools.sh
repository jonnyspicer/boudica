#!/usr/bin/env bash
# Install the strength-testing toolchain on a test machine (opus or an EC2 box).
# Everything goes under $CHESS_DIR. Exact versions/commits are recorded in
# $CHESS_DIR/versions.txt so results can be traced to the tools that made them.
#
# Usage: sudo tests/strength/setup-tools.sh [--with-tb] [--with-sf10]
#   --with-tb    also download 3-4-5 piece Syzygy tablebases (~1 GB, gauntlets only)
#   --with-sf10  also build Stockfish 10 for time-control scaling (gauntlets only)
#   --with-rust  also install cargo (Ubuntu package) for Rust anchor engines
set -euo pipefail

CHESS_DIR="${CHESS_DIR:-/opt/chess}"
FASTCHESS_TAG="v1.8.2-alpha"
SF10_TAG="sf_10"
RUST_VERSION="1.91"
BOOKS_URL="https://github.com/official-stockfish/books/raw/master"
SYZYGY_URL="https://tablebase.lichess.ovh/tables/standard"

WITH_TB=0; WITH_SF10=0; WITH_RUST=0
for arg in "$@"; do
    case "$arg" in
        --with-tb) WITH_TB=1 ;;
        --with-sf10) WITH_SF10=1 ;;
        --with-rust) WITH_RUST=1 ;;
        *) echo "unknown option: $arg" >&2; exit 2 ;;
    esac
done

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq build-essential cmake git unzip wget rsync >/dev/null

mkdir -p "$CHESS_DIR"/{books,syzygy,src,bin}
cd "$CHESS_DIR/src"
: > "$CHESS_DIR/versions.txt"

if [[ $WITH_RUST -eq 1 ]]; then
    # Ubuntu's default cargo (1.75) is too old: anchors without a Cargo.lock pull
    # current crates that need edition 2024 (Rust >= 1.85). Use a versioned package.
    apt-get install -y -qq "cargo-$RUST_VERSION" >/dev/null
    ln -sf "/usr/lib/rust-$RUST_VERSION/bin/cargo" "/usr/lib/rust-$RUST_VERSION/bin/rustc" /usr/local/bin/
    echo "$(cargo --version)" >> "$CHESS_DIR/versions.txt"
fi

# fastchess: SPRT and gauntlet runner
if [[ ! -d fastchess ]]; then
    git clone -q --depth 1 --branch "$FASTCHESS_TAG" https://github.com/Disservin/fastchess.git
fi
make -C fastchess -j"$(nproc)" >/dev/null
cp fastchess/fastchess "$CHESS_DIR/bin/"
echo "fastchess $FASTCHESS_TAG $(git -C fastchess rev-parse --short HEAD)" >> "$CHESS_DIR/versions.txt"

# Ordo: rating calculation with fixed anchors
if [[ ! -d Ordo ]]; then
    git clone -q --depth 1 https://github.com/michiguel/Ordo.git
fi
make -C Ordo >/dev/null
cp Ordo/ordo "$CHESS_DIR/bin/"
echo "ordo $(git -C Ordo rev-parse --short HEAD)" >> "$CHESS_DIR/versions.txt"

# Opening books: UHO for SPRT self-play, 8moves for gauntlets
cd "$CHESS_DIR/books"
for book in UHO_Lichess_4852_v1.epd 8moves_v3.pgn; do
    if [[ ! -f "$book" ]]; then
        wget -q "$BOOKS_URL/$book.zip" -O "$book.zip"
        unzip -q -o "$book.zip" && rm "$book.zip"
    fi
    echo "book $book $(sha256sum "$book" | cut -c1-16)" >> "$CHESS_DIR/versions.txt"
done

if [[ $WITH_SF10 -eq 1 ]]; then
    cd "$CHESS_DIR/src"
    if [[ ! -d Stockfish ]]; then
        git clone -q --depth 1 --branch "$SF10_TAG" https://github.com/official-stockfish/Stockfish.git
    fi
    make -C Stockfish/src build ARCH=x86-64-modern -j"$(nproc)" >/dev/null
    cp Stockfish/src/stockfish "$CHESS_DIR/bin/stockfish10"
    echo "stockfish $SF10_TAG $(git -C Stockfish rev-parse --short HEAD)" >> "$CHESS_DIR/versions.txt"
fi

if [[ $WITH_TB -eq 1 ]]; then
    cd "$CHESS_DIR/syzygy"
    # robots.txt blocks recursive wget on /tables, so use Lichess's published URL list
    wget -q -O - "$SYZYGY_URL/download.txt" | grep -E '/3-4-5-(wdl|dtz)/.*\.rtb[wz]$' \
        | wget -q -nc -i -
    count=$(ls -1 *.rtb? | wc -l)
    echo "syzygy 3-4-5 files=$count" >> "$CHESS_DIR/versions.txt"
    # The full 3-4-5 piece set is 290 files (145 WDL + 145 DTZ)
    [[ $count -eq 290 ]] || { echo "expected 290 Syzygy files, got $count" >&2; exit 1; }
fi

echo "Tools installed in $CHESS_DIR:"
cat "$CHESS_DIR/versions.txt"
