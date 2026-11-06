#ifndef BENCH_H
#define BENCH_H

namespace Boudica {

// Default depth for "bench" - chosen so a run takes a few seconds
constexpr int BENCH_DEFAULT_DEPTH = 13;

// Search a fixed set of positions to a fixed depth and print the total node
// count and speed. With the same binary the node count is identical on every
// run, so it identifies a build ("Bench: <nodes>" in commit messages).
//
// Resizes the global TT to 16 MB and clears search state; callers that care
// about the previous hash size must restore it afterwards.
void bench(int depth = BENCH_DEFAULT_DEPTH);

} // namespace Boudica

#endif // BENCH_H
