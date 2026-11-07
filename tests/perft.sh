#!/usr/bin/env bash
# Perft regression suite: checks move generation against known node counts.
# Usage: tests/perft.sh [path/to/boudica]
set -euo pipefail

ENGINE="${1:-./boudica}"

# FEN | depth | expected nodes (chessprogramming.org perft results)
CASES=(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1|6|119060324"
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1|5|193690690"
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1|6|11030083"
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1|5|15833292"
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8|4|2103487"
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10|4|3894594"
)

failed=0
for case in "${CASES[@]}"; do
    IFS='|' read -r fen depth expected <<< "$case"
    nodes=$(printf 'position fen %s\nperft %s\nquit\n' "$fen" "$depth" \
        | "$ENGINE" | sed -n 's/^Nodes: //p')
    if [[ "$nodes" == "$expected" ]]; then
        echo "ok    d$depth $nodes  $fen"
    else
        echo "FAIL  d$depth got '${nodes}' expected $expected  $fen"
        failed=1
    fi
done

if [[ $failed -ne 0 ]]; then
    echo "Perft suite FAILED"
    exit 1
fi
echo "Perft suite passed"
