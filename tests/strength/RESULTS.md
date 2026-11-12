# Boudica strength results

Method: `docs/strength-testing-plan.md`. Newest entries at the bottom.

## Smoke tests

| Date | Machine | Dev | Base | TC | Games | Bad terminations | Result |
|---|---|---|---|---|---|---|---|
| 2026-11-06 | opus (2 vCPU Skylake), concurrency 2 | strength-testing-prep (uncommitted), Bench 15749109 | main 949e8e1 (own book always on) | 5+0.05, UHO | 200 | 0 (all "normal") | PASS. +35 ± 39 Elo, not meaningful: base still plays its own book |

## SPRT log

| Date | Dev commit | Dev Bench | Base Bench | Change | Bounds | TC | Games | LLR | Elo ± | Result |
|---|---|---|---|---|---|---|---|---|---|---|
| 2026-11-12 | c9d0387 (#12) | 8785978 | 9771570 | TT generation/bound packing fix | [-3, 1] | 10+0.1 | 3354 | 2.94 | +18.0 ± 8.3 | H1, merged |
| 2026-11-12 | 1e42fc2 (#20) | 9930513 | 8785978 | Pawn-key correction history | [0, 5] | 10+0.1 | 956 | 2.95 | +61.3 ± 16.8 | H1, merged |
| 2026-11-12 | 72998fc (#22) | 9930513 | 9930513 | Stop at a hard time limit, not the soft target | [0, 5] | 10+0.1 | 1158 | 2.96 | +45.6 ± 13.8 | H1, merged |
| 2026-11-12 | 8d518e3 | 8320792 | 8785978 | Capture history | [0, 5] | 10+0.1 | 5838 | -1.42 | -2.4 ± 6.3 | Stopped by hand to re-run on new main |

Runs on 2026-11-12 that ended without a verdict are not in the table:
- **Spot interruptions:** these restarted the running test from game 0 until #21 added resume.
- **The stdout race:** fixed in #19. About one search in 1,500 lost its output and fastchess stopped the tournament.

  | Branch | Games | Elo when stopped |
  |---|---|---|
  | fix-singular | 2970 | +15.5 ± 9.0 |
  | time-soft-hard | 936 | +38.4 ± 16.0 |
  | qsearch-tt | 1350 | +29.4 ± 13.2 |
  | lmr-adjust | 812 | +10.3 ± 17.4 |

  All four are queued again on the current main.

## EC2 spend

The weekly budget is $50. The gauntlets on 2026-11-09 and 2026-11-11 were approved separately.

| Week of | Box | Type | Hours | ≈ Cost |
|---|---|---|---|---|
| 2026-11-09 | SPRT queue (from 2026-11-12 11:01Z) | c7a.8xlarge spot, $0.49–0.67/h | ~7.5 to 18:30Z | ~$4.5 |
| 2026-11-09 | Datagen (2026-11-12 12:57Z to 18:25Z, terminated) | c7a.8xlarge spot | ~5 | ~$3 |

## Gauntlets

<!-- One section per run: version, Bench, machine, scale f, list (blitz/4040), rung,
     anchors, games, Ordo rating ± error, per-anchor performance, notes. -->

### 2026-11-09: rung A blitz, main 37b10ac (before the TT fix)

- **Bench:** 15749109. **Machine:** EC2 c7a.4xlarge on-demand, concurrency 15, scale f=0.775, TC 93.0+0.78.
- **Anchors:** vice, smallbrain, fatalii05, sungorus, tantabus, fatalii06 (300 games each). MORA 1.1 excluded: it crashed in smoke testing.
- **Result:** **1646 ± 38** CCRL blitz, 104.5/1800 (5.8%).
- **Not valid as a baseline:** Boudica reported exactly 0.00 on 56% of its moves. The transposition table's 16-bit check used the same key bits as the bucket index, so any two positions in a bucket matched each other. This was fixed in PR #9 (adfaf29).

### 2026-11-11: rung A blitz, main 1ddeefa (after the TT fix)

- **Bench:** 9771570. **Machine:** EC2 c7a.4xlarge on-demand, concurrency 15, scale f=0.780, TC 93.6+0.78.
- **Anchors:** same as above, 300 games each. MORA 1.1 excluded.
- **Result:** **2633 ± 25** CCRL blitz, +1552 =169 −79 (90.9%). This is about +990 Elo over the 2026-11-09 run.
- **Problems:** no time forfeits or crashes. 1681 games ended by adjudication, 119 normally. 0.00 evals fell to 5.3% of moves (from 56%).

| Anchor | CCRL blitz | Boudica +/=/− | Score |
|---|---|---|---|
| tantabus | 2331 | 256 / 30 / 14 | 90.3% |
| fatalii06 | 2317 | 222 / 41 / 37 | 80.8% |
| sungorus | 2241 | 258 / 28 / 14 | 90.7% |
| fatalii05 | 2207 | 245 / 46 / 9 | 89.3% |
| smallbrain | 2114 | 279 / 19 / 2 | 96.2% |
| vice | 1977 | 292 / 5 / 3 | 98.2% |

- **Caveat:** 91% against anchors rated 1977–2331 is far outside their range, so 2633 is an extrapolation. The ± 25 does not include the error from extrapolating. A rung B gauntlet with anchors around 2500–2800 is needed for a reliable number.
- **Minor issue:** some `info` lines carry a PV well over 100 moves long, cycling between two king moves. The PV is probably rebuilt from the TT without a repetition or length check. It is cosmetic and does not affect play.
