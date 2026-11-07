# Boudica strength results

Method: `docs/strength-testing-plan.md`. Newest entries at the bottom.

## Smoke tests

| Date | Machine | Dev | Base | TC | Games | Bad terminations | Result |
|---|---|---|---|---|---|---|---|
| 2026-11-06 | opus (2 vCPU Skylake), concurrency 2 | strength-testing-prep (uncommitted), Bench 15749109 | main 949e8e1 (own book always on) | 5+0.05, UHO | 200 | 0 (all "normal") | PASS. +35 ± 39 Elo, not meaningful: base still plays its own book |

## SPRT log

| Date | Dev commit | Dev Bench | Base Bench | Change | Bounds | TC | Games | LLR | Elo ± | Result |
|---|---|---|---|---|---|---|---|---|---|---|

## Gauntlets

<!-- One section per run: version, Bench, machine, scale f, list (blitz/4040), rung,
     anchors, games, Ordo rating ± error, per-anchor performance, notes. -->
