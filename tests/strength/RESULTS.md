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
| 2026-11-12 | 39b32cc (#25) | 8744851 | — | Texel tune 1 (73k games) | [0, 5] | 10+0.1 | — | — | +64.6 ± 18.0 | H1, merged |
| 2026-11-12 | d7fe518 (#26) | 6847405 | — | TT probe in qsearch | [0, 5] | 10+0.1 | 3880 | 2.98 | +14.8 ± 7.7 | H1, merged |
| 2026-11-12 | f5a0e02 (#27) | 21832786 | — | Singular extension fix | [0, 5] | 10+0.1 | 2444 | 2.95 | +24.9 ± 9.6 | H1, merged |
| 2026-11-12 | beffefa (#29) | 20961535 | — | LMR adjustments | [0, 5] | 10+0.1 | 10150 | 2.98 | +6.8 ± 4.8 | H1, merged |
| 2026-11-12 | 995cbd4 (#30) | 22127970 | — | Texel tune 2 | [0, 5] | 10+0.1 | 1236 | 2.95 | +44.9 ± 14.3 | H1, merged |
| 2026-11-13 | cont-hist-2 | — | — | Continuation history | [0, 5] | 10+0.1 | ~138000 | — | ≈ 0 | No verdict after 33 h; abandoned, led to the 20k cap (#31) |
| 2026-11-23 | efad24d (#32) | 22505789 | 22127970 | Reuse TT static eval | [0, 5] | 10+0.1 | 6332 | 2.96 | +9.3 ± 5.8 | H1, merged |
| 2026-11-23 | ed16457 (#33) | 22558697 | 22127970 | History malus only on searched quiets | [-3, 1] | 10+0.1 | 11226 | 2.95 | +4.6 ± 4.3 | H1, merged |
| 2026-11-23 | a3f1431 (#34) | 22634871 | 22505789 | cut_node propagation fix | [-3, 1] | 10+0.1 | 12340 | 2.95 | +4.1 ± 4.1 | H1, merged |
| 2026-11-23 | c55606b (#35) | 20919619 | 19891997 | No-pawn endgame scale + mop-up | [-3, 1] | 10+0.1 | 2608 | 2.95 | +16.1 ± 6.3 | H1, merged |
| 2026-11-23 | texel-tune-3 | 22570555 | 18397157 | Texel tune 3 (17.9M positions, MSE 0.09932 → 0.09917) | [0, 5] | 10+0.1 | 13464 | -2.95 | -2.0 ± 4.1 | H0, not merged |

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
| 2026-11-09 | SPRT queue and datagen, 2026-11-12 to 2026-11-14 (incl. cont-hist-2's 33 h) | c7a.8xlarge spot/on-demand | — | ~$40 (estimate) |
| 2026-11-23 | SPRT queue, 11:52Z to 23:05Z, terminated | c7a.8xlarge on-demand, ~$1.64/h | ~11.2 | ~$18.4 |
| 2026-11-23 | Old spot box i-02af2950bf36c88a6, stopped, terminated 2026-11-25 | EBS only | — | under $1 |
| 2026-11-23 | Rung B gauntlet, 2026-11-24 11:17Z to 22:17Z (OOM-stalled after 30 min, idle until the 11 h cap) and 2026-11-25 08:56Z to 15:25Z, terminated | c7a.8xlarge on-demand, ~$1.64/h | ~17.5 | ~$28.7 |

Week of 2026-11-23 total: about $47.

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

### 2026-11-24/25: rung B blitz, main c55606b

- **Bench:** 20631620. **Machine:** EC2 c7a.8xlarge on-demand, concurrency 31, scale f=0.783, TC 94.0+0.78, Hash 256, 3-4-5 TB, 8moves_v3 openings.
- **Anchors:** wyldchess, blunder760, stash21, velvet120, halogen8, weiawaga5, weiss10, koivisto3, topple081, asymptote08 (300 games each). Zahak 5.0 and Blunder 8.5.5 replaced: see anchors.tsv.
- **Result:** **2951 ± 11** CCRL blitz, 1928/3000 (64.3%). White advantage 13.7 ± 5.5, draw rate 39.9%. Up from 2633 on rung A, but that number was an extrapolation; this one is inside the anchors' range (2602–3035).

| Anchor | CCRL blitz | Boudica score | % |
|---|---|---|---|
| topple081 | 3035 | 112/300 | 37.3% |
| koivisto3 | 3020 | 138.5/300 | 46.2% |
| asymptote08 | 2941 | 169.5/300 | 56.5% |
| weiss10 | 2896 | 190/300 | 63.3% |
| weiawaga5 | 2872 | 165/300 | 55.0% |
| halogen8 | 2826 | 178/300 | 59.3% |
| velvet120 | 2764 | 228/300 | 76.0% |
| stash21 | 2713 | 249/300 | 83.0% |
| blunder760 | 2618 | 256.5/300 | 85.5% |
| wyldchess | 2602 | 241.5/300 | 80.5% |

- **Problem 1, OOM:** fastchess keeps every engine it starts alive by default, one per worker per opponent (31 × 11 processes at 256 MB+ hash each). The 64 GB box ran out of memory after 226 games; an anchor was killed and fastchess hung until the 11 h shutdown cap. Fixed with `restart=on` in gauntlet.sh; the run resumed with memory flat at 17 GB.
- **Problem 2, replayed games:** the resume replayed the first 226 games, so the PGN held 3226. Ordo was re-run without the 226 pre-crash games (2952.1 ± 10.2 with them, 2951.4 ± 10.9 without).
- **Data lost:** the PGN and Ordo h2h file were not copied off before teardown (a failed scp followed by an unconditional teardown). The ratings table above is from the Ordo output captured during the run.
