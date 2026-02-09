#ifndef TT_H
#define TT_H

#include "types.h"
#include <cstdlib>
#include <cstring>

// Boudica Chess Engine - Transposition Table
//
// The transposition table stores previously searched positions to avoid
// redundant work. Each entry contains the hash key verification, best move,
// search score, depth, and bound type.

// TTEntry: 10 bytes packed for cache efficiency
// Layout:
//   key16    (2 bytes) - upper 16 bits of hash for verification
//   move     (2 bytes) - best move found
//   value    (2 bytes) - search score
//   staticEval (2 bytes) - static evaluation
//   depth    (1 byte)  - search depth
//   genBound (1 byte)  - generation (6 bits) + bound type (2 bits)

struct TTEntry {

    Move  move()       const { return Move(move16); }
    Value score()      const { return Value(value16); }
    Value static_eval() const { return Value(staticEval16); }
    Depth depth()      const { return Depth(depth8 + DEPTH_NONE); }
    Bound bound()      const { return Bound(genBound8 & 0x3); }

    // Check if this entry matches the given key
    bool key_matches(Key k) const { return key16 == uint16_t(k >> 48); }

    // Get the generation (age) of this entry
    uint8_t generation() const { return genBound8 >> 2; }

    // Save data to this entry
    void save(Key k, Value v, Bound b, Depth d, Move m, Value ev, uint8_t gen) {

        // Preserve move if we don't have a new one
        if (m || key16 != uint16_t(k >> 48))
            move16 = uint16_t(m);

        // Don't overwrite entries from deeper searches with same position
        // unless the new entry is exact or from a new generation
        if (b == BOUND_EXACT
            || key16 != uint16_t(k >> 48)
            || d + 4 > depth8
            || gen != generation()) {

            key16       = uint16_t(k >> 48);
            value16     = int16_t(v);
            staticEval16 = int16_t(ev);
            depth8      = uint8_t(d - DEPTH_NONE);
            genBound8   = uint8_t((gen << 2) | b);
        }
    }

private:
    friend class TranspositionTable;

    uint16_t key16;
    uint16_t move16;
    int16_t  value16;
    int16_t  staticEval16;
    uint8_t  depth8;
    uint8_t  genBound8;
};

// Ensure TTEntry is exactly 10 bytes for cache efficiency
static_assert(sizeof(TTEntry) == 10, "TTEntry must be 10 bytes");


// TTCluster: A bucket of entries for the same hash index
// Using 3 entries per cluster gives good hit rate while fitting in cache lines
// 3 entries * 10 bytes = 30 bytes, padded to 32 bytes for alignment

constexpr int ClusterSize = 3;

struct TTCluster {
    TTEntry entry[ClusterSize];
    char    padding[2];  // Pad to 32 bytes for cache line alignment
};

static_assert(sizeof(TTCluster) == 32, "TTCluster must be 32 bytes");


// TranspositionTable: Main hash table class
//
// Features:
// - Lock-free design (safe for single-threaded use)
// - Bucket-based replacement with 3 entries per index
// - Generation-based aging for better replacement decisions
// - Cache-aligned memory allocation

class TranspositionTable {

public:
    TranspositionTable() : clusterCount(0), table(nullptr), generation8(0) {}
    ~TranspositionTable() { aligned_free(table); }

    // Non-copyable
    TranspositionTable(const TranspositionTable&) = delete;
    TranspositionTable& operator=(const TranspositionTable&) = delete;

    // Increment generation counter for new search
    // Called at the start of each new search from root
    void new_search() { generation8 += 4; } // +4 because lower 2 bits are for bound

    // Return the current generation
    uint8_t generation() const { return generation8; }

    // Probe the table for an entry matching the given key
    // Returns pointer to entry (valid or empty slot)
    // Sets 'found' to true if a matching entry was found
    TTEntry* probe(Key key, bool& found) const;

    // Store a new entry in the table
    // Handles replacement decisions internally
    void store(Key key, Value value, Bound bound, Depth depth,
               Move move, Value staticEval);

    // Resize the table to the given size in megabytes
    // Clears all existing entries
    void resize(size_t mbSize);

    // Clear all entries (zero memory)
    void clear();

    // Return approximate fill rate in per mille (0-1000)
    int hashfull() const;

    // Get pointer to first entry in cluster for given key
    TTEntry* first_entry(Key key) const {
        return &table[mul_hi64(key, clusterCount)].entry[0];
    }

private:
    // Platform-independent aligned memory allocation
    static void* aligned_alloc(size_t alignment, size_t size);
    static void  aligned_free(void* ptr);

    // Multiply two 64-bit values and return high 64 bits
    // Used for fast modulo operation: (key * count) >> 64 gives index in [0, count)
    static size_t mul_hi64(uint64_t a, uint64_t b) {
#if defined(__SIZEOF_INT128__)
        return (size_t)(((__uint128_t)a * (__uint128_t)b) >> 64);
#elif defined(_MSC_VER) && defined(_M_X64)
        uint64_t hi;
        _umul128(a, b, &hi);
        return (size_t)hi;
#else
        // Fallback: standard 64-bit multiplication split
        uint64_t aL = (uint32_t)a, aH = a >> 32;
        uint64_t bL = (uint32_t)b, bH = b >> 32;
        uint64_t c1 = (aL * bL) >> 32;
        uint64_t c2 = aH * bL + c1;
        uint64_t c3 = aL * bH + (uint32_t)c2;
        return (size_t)(aH * bH + (c2 >> 32) + (c3 >> 32));
#endif
    }

    size_t      clusterCount;
    TTCluster*  table;
    uint8_t     generation8;
};

// Global transposition table instance
extern TranspositionTable TT;

// Utility: adjust mate scores for storage/retrieval across different plies
// When storing: convert mate-in-N from current ply to mate-in-N from root
// When retrieving: convert back

inline Value value_to_tt(Value v, int ply) {
    return v >= VALUE_MATE_IN_MAX_PLY  ? Value(v + ply)
         : v <= VALUE_MATED_IN_MAX_PLY ? Value(v - ply)
         : v;
}

inline Value value_from_tt(Value v, int ply) {
    return v >= VALUE_MATE_IN_MAX_PLY  ? Value(v - ply)
         : v <= VALUE_MATED_IN_MAX_PLY ? Value(v + ply)
         : v;
}

#endif // TT_H
