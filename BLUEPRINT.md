# Building a 2000+ Elo chess engine in C++: A parallel development blueprint

A competitive chess engine reaching 2000+ Elo requires **five core systems working in harmony**: bitboard-based board representation, alpha-beta search with modern pruning, quality move ordering, a tapered evaluation function, and UCI protocol compliance. The good news: this architecture naturally divides into parallelizable modules suitable for multiple Claude Code instances using git worktrees. Research from the Chess Programming Wiki and Stockfish's codebase reveals a clear development path—starting from a ~1500 Elo foundation to a 2000+ Elo engine typically takes 4-6 months of focused development.

## The architecture that enables parallel development

Modern chess engines follow a layered architecture that Stockfish exemplifies. The foundation layer contains **bitboard operations and board representation**—12 bitboards (6 piece types × 2 colors) combined with a redundant 64-square mailbox array for O(1) piece lookups. This hybrid approach enables both fast pattern matching via bitwise operations and direct "what's on square X?" queries.

Above this sits three largely independent systems: **search** (alpha-beta with pruning), **evaluation** (static position assessment), and **UCI protocol handling**. This separation is crucial for parallel development—Stockfish keeps UCI logic completely isolated from engine computation, allowing one Claude instance to build the protocol handler while another develops the evaluation function.

The recommended module boundaries for parallel Claude Code instances are:

| Module | Hard Dependencies | Parallelizable? |
|--------|-------------------|-----------------|
| Board representation | None | Foundation (build first) |
| Move generation | Board rep | After foundation |
| UCI protocol | Board rep | ✅ Yes |
| Evaluation (HCE) | Board rep | ✅ Yes |
| Transposition table | Zobrist hashing | ✅ Yes |
| Search core | Move gen, board rep | Sequential |
| Move ordering | Move gen | ✅ Yes |
| Time management | None | ✅ Yes |

## Core components and implementation guidance

**Board representation** should use bitboards with magic multiplication for sliding piece attacks. Magic bitboards provide perfect hashing—multiply the relevant occupancy by a "magic number," shift right, and index into a precomputed attack table. This yields rook/bishop attacks in ~3 instructions. The total lookup table requires 800KB-2MB. Include Zobrist hashing from the start: generate random 64-bit numbers for each (piece, square) combination and XOR incrementally during make/unmake operations.

**Move generation** should be pseudo-legal (generate without checking if king is left in check) with legality verified at execution time. This approach wastes less computation since many generated moves are pruned by alpha-beta before execution. Use a compact 16-bit move encoding: bits 0-5 for source square, 6-11 for destination, 12-15 for flags (promotion piece, castling, en passant).

**Search** starts with alpha-beta and iterative deepening—essential because it provides time management (always have a move ready), populates the transposition table, and informs move ordering for subsequent iterations. Add Principal Variation Search next: search the first move with a full window, subsequent moves with a null window, and re-search only if the null window fails. The key pruning techniques and their measured Elo impact are:

- **Quiescence search**: Extends tactical lines until quiet positions. Impact: **+145 Elo**
- **Null move pruning**: Skip your turn to detect obvious refutations. Impact: **+116 Elo**
- **Late move reductions (LMR)**: Search later moves at reduced depth. Impact: **+229 Elo**
- **Aspiration windows**: Start with narrow bounds around previous score. Impact: **+101 Elo**

**Move ordering** is critical—perfect ordering reduces the alpha-beta tree from b^n to b^(n/2) nodes. The optimal order is: hash move (TT), winning captures (MVV-LVA), equal captures, killer moves (2 per ply), countermoves, history-sorted quiet moves, losing captures. MVV-LVA alone contributes **~495 Elo**; history heuristic adds **~94 Elo**; killer moves contribute **~22 Elo**.

**Evaluation** for 2000+ Elo requires tapered evaluation (blending middlegame and endgame scores based on material). Essential components: material values, piece-square tables, pawn structure (doubled/isolated/passed pawns), basic king safety, and mobility. A well-tuned hand-crafted evaluation reaches ~2200 Elo. NNUE provides an additional 100-300 Elo over good HCE but requires training infrastructure—defer this for the initial engine.

## Development sequence and dependency graph

The build order that minimizes blocking and maximizes parallel work follows three phases:

**Phase 1: Foundation (~1500 Elo target)**
Build sequentially: Board representation → Move generation → Perft verification → Basic alpha-beta → Material + PST evaluation → Minimal UCI protocol. This creates a playable engine. Perft testing is essential before proceeding—it validates every aspect of move generation against known node counts (starting position depth 5 = 4,865,609 nodes; Kiwipete position depth 4 = 4,085,603 nodes).

**Phase 2: Core features (~2000 Elo target)**
After Phase 1, these can develop in parallel:
- Search enhancements: Iterative deepening, transposition table, quiescence search
- Move ordering: MVV-LVA, killer moves, history heuristic (each parallelizable)
- Evaluation: Tapered eval, king safety, pawn structure (each parallelizable)
- Infrastructure: Full UCI implementation, time management

**Phase 3: Advanced optimizations (2000+ Elo)**
- Null move pruning, LMR, futility pruning
- PVS/Negascout, aspiration windows
- SEE (Static Exchange Evaluation) for capture ordering
- Optional: Lazy SMP (parallel search), NNUE, Syzygy tablebases

The dependency graph shows clear parallelization opportunities:

```
Board Representation (foundation)
         │
    Move Generation + Zobrist Hashing
         │
    ┌────┴─────┬──────────┬──────────┐
    │          │          │          │
  Search     UCI      Evaluation   Testing
  Core     Protocol    (HCE)      (Perft)
    │          
    └──────────┬──────────┐
         Move Ordering    TT
    ┌─────┬─────┬─────┐   │
  MVV-LVA Killers History │
    └─────┴─────┴─────┴───┘
               │
        Quiescence + Pruning
```

## GitHub issues and git worktrees for coordination

Structure the repository with git worktrees enabling 4-5 parallel Claude Code instances:

```bash
git worktree add ../chess-foundation -b feature/board-representation main
git worktree add ../chess-evaluation -b feature/evaluation main
git worktree add ../chess-uci -b feature/uci-protocol main
git worktree add ../chess-search -b feature/search main
git worktree add ../chess-testing -b feature/testing-infrastructure main
```

Organize GitHub issues by epic to prevent overlap:

**Epic: Core Infrastructure** (Issues #1-4)
- #1: Bitboard implementation with magic bitboards
- #2: Move generation with perft validation
- #3: Zobrist hashing and position state
- #4: Minimal UCI protocol (uci, isready, position, go, stop, quit)

**Epic: Search Implementation** (Issues #5-9, depends on #1-3)
- #5: Alpha-beta with iterative deepening
- #6: Transposition table with replacement strategy
- #7: Quiescence search
- #8: Move ordering (MVV-LVA, killers, history)
- #9: PVS and aspiration windows

**Epic: Evaluation** (Issues #10-14, parallel after #1)
- #10: Material counting and piece-square tables
- #11: Tapered evaluation (midgame/endgame blending)
- #12: Pawn structure evaluation
- #13: King safety terms
- #14: Mobility calculation

**Epic: Testing** (Issues #15-18, parallel after #2)
- #15: Perft test suite with divide function
- #16: Benchmark positions for speed tracking
- #17: cutechess-cli SPRT testing infrastructure
- #18: WAC/STS test suite integration

## Testing methodology that ensures correctness

**Perft testing** validates move generation completely. Implement a `divide` command that shows per-move node counts—when counts differ from Stockfish, this isolates the bug to a specific move. Standard positions to verify: starting position (depth 6 = 119,060,324), Kiwipete (depth 5 = 193,690,690), and promotion-heavy Position 4 (depth 5 = 15,833,292).

**SPRT testing** statistically validates improvements. Use cutechess-cli with parameters: `elo0=0 elo1=10 alpha=0.05 beta=0.05` to test whether a change provides at least 10 Elo. Typical game counts: 500-2,000 games for large improvements (~20 Elo), 10,000-50,000 games for small improvements (~5 Elo). Use balanced opening books (8moves_v3.pgn) for reproducibility.

**Regression testing** tracks performance: implement a `bench` command that searches fixed positions at fixed depth, outputting total nodes and NPS. Any code change should maintain or improve these numbers. OpenBench provides distributed SPRT testing modeled on Stockfish's Fishtest.

## Expected Elo progression from techniques

Based on measured data from engine development communities:

| Stage | Techniques | Expected CCRL Elo |
|-------|------------|-------------------|
| Pure minimax + material | None | ~800-1000 |
| Alpha-beta + material | Basic pruning | ~1200-1400 |
| + Piece-square tables | Position awareness | ~1500-1700 |
| + Iterative deepening + TT | Search infrastructure | ~1700-1850 |
| + Quiescence + move ordering | Tactical stability | ~1850-2000 |
| + Null move + LMR | Advanced pruning | ~2000-2200 |
| + Tapered eval + king safety | Positional understanding | ~2200-2400 |
| + NNUE | Neural evaluation | ~2800+ |

The MORA Chess Engine reached **2182 CCRL Elo** in 6 months using techniques through the "advanced pruning" level—demonstrating that 2000+ Elo is achievable without NNUE.

## Stockfish patterns worth adopting

Stockfish's architecture offers several patterns ideal for a new engine:

**Staged move generation (MovePicker)**: Generate moves in phases—TT move first, then captures, then killers, then quiets. This enables cutoffs before generating all moves, saving significant time.

**StateInfo chain pattern**: Use a linked list of incremental state to enable O(1) undo without copying the full position. Each `do_move` pushes state; `undo_move` simply pops.

**Template-specialized search**: Define `search<NodeType>()` where NodeType is Root, PV, or NonPV. The compiler optimizes each variant separately, eliminating runtime branching.

**Lazy SMP for parallelism**: All threads search the same position but at different depths, sharing only the transposition table (lock-free). Each thread has independent history tables. This scales well without complex synchronization—threads naturally differentiate their work through TT sharing.

## Practical starting point

Begin with this file structure:

```
src/
├── types.h        // Square, Piece, Move, Value definitions
├── bitboard.cpp/h // Magic bitboards, attack tables
├── position.cpp/h // Board state, do_move/undo_move
├── movegen.cpp/h  // Legal move generation
├── search.cpp/h   // Alpha-beta with basic pruning
├── evaluate.cpp/h // Material + PST (expand later)
├── tt.cpp/h       // Transposition table
├── uci.cpp/h      // UCI protocol handler
└── main.cpp       // Entry point, init, UCI loop
```

The minimum UCI compliance for SPRT testing requires: `uci`/`uciok`, `isready`/`readyok`, `position startpos/fen [moves...]`, `go wtime btime winc binc`, `stop`, `quit`, `ucinewgame`. Time management can start simple: allocate time/20 + increment/2 per move.

With this architecture, parallel development across 4-5 Claude Code instances can complete Phase 1 in 2-3 weeks and reach 2000+ Elo within 2-3 months of focused development, validated by continuous SPRT testing against established engines.