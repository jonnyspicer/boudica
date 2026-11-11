// Transposition table regression test.
// Build and run: make test (or g++ -std=c++17 -Isrc tests/tt_test.cpp src/tt.cpp)
//
// The bucket index comes from the high bits of the key (mul_hi64), so the
// 16-bit check stored in each entry must come from bits the index does not
// use. When it came from the top 16 bits it added no information: any two
// positions in the same bucket matched each other, and searches cut off on
// other positions' scores (2026-11-10 gauntlet: 56% of moves scored 0.00).
#include "tt.h"
#include <cstdio>

static int failures = 0;

static void check(bool ok, const char* what) {
    std::printf("%s: %s\n", ok ? "ok  " : "FAIL", what);
    failures += !ok;
}

int main() {
    TT.resize(16);
    TT.clear();

    const Key a = 0x9a2825b8a547d63cULL;
    const Key b = a ^ 1;  // same bucket and same top 16 bits, different position
    check(TT.first_entry(a) == TT.first_entry(b), "keys a and b share a bucket");

    TT.store(a, Value(123), BOUND_EXACT, Depth(10), MOVE_NONE, Value(0));

    bool found = false;
    TTEntry* e = TT.probe(a, found);
    check(found && e->score() == Value(123), "probe finds the stored position");

    TT.probe(b, found);
    check(!found, "probe does not match a different position in the same bucket");

    return failures ? 1 : 0;
}
