# Boudica

Boudica is a UCI chess engine written in C++, vibe-coded almost entirely by [Claude Code](https://claude.ai/claude-code) as an experiment to see how strong an engine could get with minimal human oversight.

## Strength

~2100-2170 ELO (CCRL-equivalent), estimated via gauntlet tournaments against engines of known strength using [ordo](https://github.com/michiguel/Ordo) and bayeselo.

## Building

```
make
```

Requires a C++17 compiler. Produces a `boudica` binary that speaks the UCI protocol.

## Usage

Boudica can be used with any UCI-compatible chess GUI (Arena, CuteChess, etc.) or from the command line:

```
./boudica
uci
isready
position startpos
go movetime 5000
```
