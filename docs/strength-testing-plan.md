# Boudica strength-testing plan: is it ~3000 Elo?

This is the yardstick for hillclimbing Boudica. It defines what "3000 Elo" means,
how a single change is accepted or rejected, and how the absolute rating is measured
at milestones. Ratings and anchor numbers below are a snapshot taken on **2026-11-06**;
re-pull them before each milestone run, as they drift by a few points as CCRL adds games.

## 1. What "3000" means

**Target: 3000 on the CCRL 40/15 list, single-CPU.**

CCRL 40/15 (computerchess.org.uk/4040) is the list engine authors quote by default
("CCRL Elo"). Its conditions, from the CCRL About page:

| Condition | CCRL 40/15 rule |
|---|---|
| Time control | 40 moves in 15 min, repeating, **scaled to an Intel i7-4770k** using a Stockfish 10 bench |
| Pondering | Off |
| Hash | Same for all engines, 256 or 512 MB (single-CPU) |
| Tablebases | 4–6 piece EGTB, given to engines that support them |
| Openings | Generic book, max 12 moves per side, same for all; engines' own books **disabled** |
| Learning | Off |
| Rating tool | Bayeselo over the whole game database |

Ratings for the same engine differ between lists (e.g. Zurichess Neuchatel is 2835 on
40/15 and 2919 on CCRL Blitz 2'+1"), so every number we report states which list's
scale it is on. A Blitz-scale estimate is a proxy, not the target.

For reference, Boudica's existing claim is about 2100–2170 on this scale. TSCP 1.81 is
rated 1600 on CCRL 40/15, so the old "96% vs TSCP" result implies ~2150 with a wide
error bar. The gap to 3000 is roughly 850 Elo.

## 2. Testing tiers

Absolute rating gauntlets are far too slow to run for every change (a 40/15 gauntlet
takes days), so hillclimbing uses relative tests and the absolute scale is checked at
milestones.

| Tier | Question it answers | When | Cost (8 cores, rough) |
|---|---|---|---|
| 0. Gates | Is the change correct and safe? | Every change | Minutes |
| 1. SPRT self-play | Is the new version stronger than the current best? | Every change | 1–10 hours |
| 2. Blitz gauntlet | Roughly where are we on the CCRL scale? | Every ~+100 Elo of accepted SPRT gains, or weekly | ~2 days |
| 3. 40/15 certification | Are we at 3000 on CCRL 40/15? | When Tier 2 says we are close | ~1–2 weeks |
| 4. Independent rating | Does CCRL agree? | After Tier 3 passes | Out of our hands |

### Tier 0: correctness gates (must all pass before any game testing)

1. Perft matches known node counts: startpos d6 = 119,060,324; Kiwipete d5 = 193,690,690;
   position 4 d5 = 15,833,292. Wire these into `make test` (currently a placeholder).
2. `bench` prints a deterministic node count (same binary, same count, every run).
   Record it as `Bench: <nodes>` in each commit message, so any result can be traced to
   an exact build. (Not implemented yet: `bench` currently prints "not yet implemented".)
3. A 200-game fast smoke match (e.g. 5+0.05) against the previous best with zero crashes,
   time losses, illegal moves or disconnects. Any of those fails the change outright.

### Tier 1: SPRT self-play (the hillclimbing signal)

A change is accepted only if it passes a sequential probability ratio test against the
current best version.

- Runner: fastchess (or cutechess-cli). Both engines identical except the change.
- Openings: an unbalanced book (`UHO_Lichess_4852_v1.epd` from the official-stockfish
  `books` repo) played with colours reversed (`-repeat`), random order.
- Time control **STC 10+0.1**, Hash 16 MB, 1 thread. Changes to search depth, pruning or
  time management also need an **LTC 60+0.6** run, because some gains vanish or reverse
  at longer time controls.
- Bounds, normalised Elo, `alpha=0.05 beta=0.05`:
  - `[0, 5]` for normal changes while Boudica is weak and gains are large.
  - `[-3, 1]` "non-regression" bounds for simplifications and refactors.
  - Tighten towards `[0, 2]` once typical gains shrink (above ~2800).
- Concurrency = physical performance cores minus one. Never mix core types (on the M3,
  the efficiency cores make games uneven; use a homogeneous Linux box for long runs).

Example (check flags against the installed fastchess version):

```bash
fastchess \
  -engine cmd=./boudica-dev name=dev \
  -engine cmd=./boudica-base name=base \
  -each tc=10+0.1 option.Hash=16 \
  -openings file=UHO_Lichess_4852_v1.epd format=epd order=random \
  -repeat -rounds 50000 -concurrency 7 \
  -sprt elo0=0 elo1=5 alpha=0.05 beta=0.05 \
  -pgnout file=sprt-dev-vs-base.pgn -recover
```

Self-play Elo overstates real gains (often by 1.5–2x), which is why Tier 2 exists: a sum of
SPRT gains is never reported as Boudica's rating.

### Tier 2: Blitz-scale calibration gauntlet

Measures absolute strength cheaply on the **CCRL Blitz** scale.

- Conditions: CCRL Blitz is 2'+1" on the same i7-4770k reference. Scale the time control to
  the test machine (section 3). Hash 256 MB for everyone, ponder off, 5-piece Syzygy given to
  every engine that supports it, Boudica's own book off.
- Openings: a balanced book, e.g. `8moves_v3.pgn` (official-stockfish `books` repo), each
  opening played twice with colours reversed. Not the UHO book used in Tier 1.
- Opponents: **6 anchors** from the current rung of the ladder (section 4), all within about
  ±150 Elo of Boudica's current estimate, so each scores 25–75% against it.
- Games: 300 per anchor (150 openings × 2 colours), 1,800 in total. That gives a 95%
  interval of roughly ±16 Elo before anchor uncertainty.
- Rating: Ordo with every anchor fixed at its CCRL Blitz rating (`-m anchors.csv`),
  white advantage and draw rate estimated from the games, errors from simulation.

```bash
cutechess-cli -tournament gauntlet \
  -engine conf=boudica \
  -engine conf=anchor1 -engine conf=anchor2 ... \
  -each proto=uci tc=<scaled 2+1> option.Hash=256 \
  -openings file=8moves_v3.pgn format=pgn order=random -repeat -games 2 -rounds 150 \
  -tb /path/to/syzygy -tbpieces 5 \
  -draw movenumber=40 movecount=8 score=10 \
  -resign movecount=3 score=1000 twosided=true \
  -concurrency 7 -pgnout gauntlet-blitz.pgn -recover

ordo -p gauntlet-blitz.pgn -m anchors-blitz.csv -W -D -s 1000 -o ratings.txt -j h2h.txt
```

- Consistency check: Boudica's performance against each individual anchor (from `h2h.txt`)
  should be within about ±75 of the pooled estimate. A large outlier means a style or bug
  interaction (e.g. a time-control bug that only one opponent exposes); investigate it
  before trusting the number.

### Tier 3: CCRL 40/15 certification

Same as Tier 2 with these differences:

- Time control: 40/15 repeating, scaled to the test machine.
- Anchors: rung D (section 4) using their **CCRL 40/15** ratings, including at least two
  engines that were not used in any Tier 2 run (held out, see section 5).
- 8 anchors × 200 games = 1,600 games.

**Pass criterion for "Boudica is ~3000 CCRL 40/15":**

1. Ordo point estimate ≥ 3000, and
2. lower end of the 95% interval ≥ 2960, and
3. every per-anchor performance within ±75 of the estimate, and
4. zero crashes, time losses or illegal moves across all games.

Report it in this form: *"Boudica <version> (Bench: N): 3012 ± 17, CCRL 40/15 conditions
scaled to <machine>, 1,600 games vs 8 anchors, 8moves_v3, ordo, 2026-MM-DD."*

### Tier 4: independent rating

Submit the certified version to CCRL (and optionally CEGT) via the CCRL forum. Their result,
on their hardware, is the number to quote publicly. Expect it to differ from ours by a few
tens of Elo.

## 3. Scaling the time control to the test machine

CCRL defines its time controls on an i7-4770k and uses Stockfish 10's `bench` to translate.
Reference figures reported by CCRL testers on talkchess (verify on the CCRL forum before
certification): Stockfish 10 `bench` on an i7-4770k, single thread, takes about
**2,054 ms** for **3,939,338 nodes** (~1.92 Mnps). The node count confirms you built the
right Stockfish 10; a different count means a different binary.

1. Build Stockfish 10 from its tagged source on the test machine.
2. Run `stockfish bench` 5 times while the machine is under the same load as during games
   (e.g. run `concurrency` copies at once), and take the median total time `T_local` in ms.
3. Scale factor `f = T_local / 2054`. A slower machine gets more time.
4. Scaled time controls:
   - 40/15: `40/(900 × f)` seconds, repeating.
   - Blitz 2'+1": base `120 × f` s, increment `1 × f` s.
5. Re-measure after any OS, kernel or hardware change, and record `f` with every result.

## 4. Anchor ladder

All are open source on CCRL and rated **single-CPU** (the "4CPU" ratings on the list are a
different configuration and must not be used). Ratings: CCRL 40/15 / CCRL Blitz, snapshot
2026-11-06, with games played in brackets for 40/15. "—" means no single-CPU rating
on that list, so the engine can't serve as an anchor on that scale.

Use the rung that brackets Boudica's current estimate. Move up a rung when Boudica's Tier 2
estimate passes the middle of the next rung.

**Rung A, ~2000–2350 (where Boudica is now)**

| Engine | 40/15 | Blitz |
|---|---|---|
| Vice 1.0 | 1997 (639) | 1977 |
| Stellar 1.4.3 | 2100 (759) | — |
| MORA 1.0.0 | 2149 (554) | — |
| Weini 0.0.23 | 2197 (1115) | — |
| Sungorus 1.4 | 2269 (1583) | 2241 |
| Cinnamon 2.5 | 2336 (840) | 2308 |

**Rung B, ~2400–2700**

| Engine | 40/15 | Blitz |
|---|---|---|
| Teki 2 | 2400 (1855) | 2364 |
| MinimalChess 0.6 | 2417 (483) | 2439 |
| Loki 3.5.0 | 2473 (1929) | — |
| Tantabus 2.0.0 | 2560 (2072) | 2554 |
| CeeChess 2.2 | 2600 (861) | 2584 |
| Sloppy 0.2.2 | 2607 (1476) | 2589 |
| Toad 3.0.0 | 2617 (1312) | — |
| WyldChess 1.51 | 2679 (1569) | 2603 |

**Rung C, ~2650–2900**

| Engine | 40/15 | Blitz |
|---|---|---|
| Blunder 8.5.5 | 2692 (2075) | 2662 |
| Fruit 2.1 | 2694 (505) | — |
| Fatalii 0.10.0 | 2739 (809) | 2735 |
| Trinket 3.5.0 | 2753 (1131) | 2754 |
| Glaurung 2.2 | 2793 (1396) | 2818 |
| GNU Chess 5.60 | 2822 (1399) | 2831 |
| Zurichess Neuchatel | 2835 (2669) | 2919 |
| Dumb 2.3 | 2856 (1709) | 2833 |
| OliThink 5.11.9 | 2890 (1196) | 2903 |

**Rung D, ~2900–3150 (the target)**

| Engine | 40/15 | Blitz |
|---|---|---|
| Scorpio 2.8 | 2904 (1822) | 2929 |
| Seawall 20250322 | 2905 (1455) | 2978 |
| Asymptote 0.8 | 2938 (1997) | 2940 |
| Bobcat 8.0 | 2945 (2242) | 2966 |
| Crafty 25.2 | 2951 (2822) | — |
| Rengar 2.1.1 | 2963 (2980) | 2997 |
| Puffin 5.0 | 2987 (1968) | 2978 |
| Polaris 1.8.1 | 3021 (1811) | 3049 |
| 4ku 4.0 | 3029 (1191) | 3035 |
| Topple 0.8.1 | 3031 (1748) | 3036 |
| Amoeba 3.4 | 3041 (1894) | 3034 |
| Inanis 1.6.0 | 3047 (1544) | 3091 |
| Wahoo 4.0.0 | 3059 (3052) | 3094 |

**Above target (for checking we're not stopping short)**: Demolito 2021-10-04 (3153 / 3244),
Simbelmyne 1.10.0 (3192 / 3239), Laser 1.7 (3254 / 3292).

Anchor hygiene:

- Build each anchor from the **exact tagged version** CCRL rated, and record the commit
  hash and build flags in `anchors.toml`. A newer commit is a different engine.
- Prefer anchors with ≥1,000 CCRL games (smaller rating error). Rows with fewer games are
  usable but count for less.
- Disable each anchor's own book. Give it Syzygy if it supports it.
- Before first use, play each anchor against Boudica for 20 games to check it builds, speaks
  UCI correctly and doesn't crash or lose on time on our hardware.

## 5. Rules for hillclimbing against this plan

1. **Only Tier 1 decides whether a change lands.** Gauntlets measure; they never accept or
   reject an individual change.
2. **Never tune against anchors.** Parameter tuning (SPSA, Texel tuning) uses self-play or
   position datasets only. Anchors are a measuring stick; optimising against specific
   anchors overfits to their weaknesses.
3. **Keep held-out anchors.** At least two rung-D engines are never used before Tier 3.
4. **Harness changes are separate.** Never change the engine and the test setup (book,
   time control, anchors, scripts) in the same commit.
5. **Log everything** in `tests/strength/RESULTS.md`: one line per SPRT (commit, Bench,
   change, bounds, games, LLR, Elo ± error, pass/fail) and a section per gauntlet
   (version, Bench, machine, `f`, list scale, anchors, games, Ordo output).
6. **Originality.** Don't copy code from other engines, especially strong open-source ones.
   CCRL won't rate clones, and a copied engine says nothing about the experiment. Before
   Tier 4, run a move-similarity test against the strongest open-source engines.

## 6. Work needed in Boudica before testing starts

Status as of branch `strength-testing-prep`:

1. **Done: `OwnBook` option**, default `false` (CCRL requires engine books disabled).
   `web/backend/engine.py` turns it on, so the website still plays book moves.
2. **Done: `bench [depth]`** (UCI command and `./boudica bench [depth]`). 27 fixed
   positions, depth 13, 16 MB hash, tables cleared per position. Prints `Nodes searched`,
   which is identical on every run of the same binary.
3. **Done: `make test`** runs `tests/perft.sh` (the six positions above) and then bench.
4. **Partly done: `Move Overhead` option** (default 30 ms). It is applied to `go movetime`.
   For clock time controls the hook `usable_clock_time()` in `src/search.cpp` doesn't
   reserve anything yet.
5. **Syzygy probing (optional, worth Elo).** Anchors that support tablebases get them under
   CCRL rules; Boudica doesn't support them yet.
6. **Single thread only.** Boudica has no `Threads` option, which matches the single-CPU
   target. Don't add SMP until the single-thread rating is certified.

## 7. Infrastructure

- **Test machine:** a dedicated, homogeneous Linux box (dedicated vCPUs, not shared) with
  8+ cores. The current servers are shared with live apps (`hetzner`: 4 cores, `yolobox`:
  8 Neoverse-N1 cores; `opus`: 2 cores) and the M3 laptop mixes performance and efficiency
  cores. Background load changes engine speed and biases results.
- **Tools:** fastchess (SPRT), cutechess-cli (gauntlets), Ordo (ratings), Stockfish 10
  (time-control scaling), Syzygy 3-4-5 piece files (~1 GB), `8moves_v3.pgn` and
  `UHO_Lichess_4852_v1.epd`.
- **Layout:** put everything under `tests/strength/`: `anchors.toml`, `anchors-4040.csv`,
  `anchors-blitz.csv`, `run_sprt.sh`, `run_gauntlet.sh`, `RESULTS.md`, and a `pgn/` folder
  (gitignored).

## 8. Rough timeline per milestone

At 7 concurrent games on a machine about half the speed of an i7-4770k (`f ≈ 2`):

- SPRT at 10+0.1: a clear +10 Elo change usually resolves in 1–3k games (1–3 hours); a
  marginal one can take 20k+ games (most of a day).
- Tier 2 blitz gauntlet, 1,800 games at ~12 min each: about 2 days.
- Tier 3 40/15 certification, 1,600 games at ~80 min each: about 12 days.

## Sources

- CCRL 40/15 list, complete list and conditions: <https://computerchess.org.uk/4040/>,
  <https://computerchess.org.uk/4040/about.html>
- CCRL Blitz complete list: <https://computerchess.org.uk/ccrl/404/rating_list_all.html>
- Stockfish 10 bench reference for i7-4770k and CCRL time-control scaling (forum posts):
  <https://talkchess.com/viewtopic.php?p=918869>, <https://talkchess.com/viewtopic.php?p=857309>
