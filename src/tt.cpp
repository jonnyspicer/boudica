#include "tt.h"
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#endif

// Global transposition table instance
TranspositionTable TT;

// Default table size in MB
constexpr size_t DefaultHashMB = 64;


// Platform-independent aligned memory allocation
void* TranspositionTable::aligned_alloc(size_t alignment, size_t size) {
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#elif defined(__APPLE__) || defined(__ANDROID__)
    // posix_memalign available on these platforms
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0)
        ptr = nullptr;
    return ptr;
#else
    // C11 aligned_alloc (size must be multiple of alignment)
    size = ((size + alignment - 1) / alignment) * alignment;
    return ::aligned_alloc(alignment, size);
#endif
}

void TranspositionTable::aligned_free(void* ptr) {
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}


// Resize the transposition table to the given size in megabytes
// This allocates new memory and clears the table
void TranspositionTable::resize(size_t mbSize) {

    // Ensure minimum size
    if (mbSize < 1)
        mbSize = 1;

    // Calculate number of clusters that fit
    // Each cluster is 32 bytes
    size_t newClusterCount = (mbSize * 1024 * 1024) / sizeof(TTCluster);

    // Don't reallocate if size hasn't changed
    if (newClusterCount == clusterCount)
        return;

    // Free existing table
    aligned_free(table);

    clusterCount = newClusterCount;

    // Allocate with cache line alignment (64 bytes)
    table = static_cast<TTCluster*>(
        aligned_alloc(64, clusterCount * sizeof(TTCluster)));

    if (!table) {
        clusterCount = 0;
        return;
    }

    clear();
}


// Clear all entries in the table
void TranspositionTable::clear() {

    if (table && clusterCount > 0) {
        std::memset(table, 0, clusterCount * sizeof(TTCluster));
    }

    generation8 = 0;
}


// Probe the transposition table for an entry
// Returns pointer to matching entry or best replacement candidate
// Sets 'found' to true if the entry matches the position
TTEntry* TranspositionTable::probe(Key key, bool& found) const {

    TTEntry* const tte = first_entry(key);
    const uint16_t key16 = uint16_t(key >> 48);

    // Search all entries in the cluster
    for (int i = 0; i < ClusterSize; ++i) {
        if (tte[i].key16 == key16) {
            found = true;
            return &tte[i];
        }

        // Empty entry means position not found
        if (!tte[i].depth8 && !tte[i].key16) {
            found = false;
            return &tte[i];
        }
    }

    // Position not found - find best replacement candidate
    // Prefer entries that are: older generation, shallower depth
    found = false;

    TTEntry* replace = tte;
    for (int i = 1; i < ClusterSize; ++i) {
        // Replacement score: lower is more replaceable
        // Favor replacing older, shallower entries
        int replaceScore = replace->depth8 - ((generation8 - replace->generation()) & 0xFC);
        int currentScore = tte[i].depth8 - ((generation8 - tte[i].generation()) & 0xFC);

        if (currentScore < replaceScore)
            replace = &tte[i];
    }

    return replace;
}


// Store an entry in the transposition table
void TranspositionTable::store(Key key, Value value, Bound bound, Depth depth,
                               Move move, Value staticEval) {

    TTEntry* const tte = first_entry(key);
    TTEntry* replace = tte;
    const uint16_t key16 = uint16_t(key >> 48);

    // Search for matching position or find replacement slot
    for (int i = 0; i < ClusterSize; ++i) {

        // Found matching position or empty slot
        if (!tte[i].depth8 || tte[i].key16 == key16) {
            replace = &tte[i];

            // Don't overwrite better entry unless move is available
            if (tte[i].key16 == key16 &&
                !move &&
                bound != BOUND_EXACT &&
                tte[i].depth8 >= depth - DEPTH_NONE + 3) {
                return;
            }
            break;
        }

        // Find worst entry for replacement
        // Score based on depth and age
        int replaceScore = replace->depth8 - ((generation8 - replace->generation()) & 0xFC);
        int currentScore = tte[i].depth8 - ((generation8 - tte[i].generation()) & 0xFC);

        if (currentScore < replaceScore)
            replace = &tte[i];
    }

    replace->save(key, value, bound, depth, move, staticEval, generation8);
}


// Return hash table fill rate in per mille (0-1000)
// Samples first 1000 entries for efficiency
int TranspositionTable::hashfull() const {

    if (!table || clusterCount == 0)
        return 0;

    int count = 0;
    const int sampleSize = std::min(size_t(1000), clusterCount);

    for (int i = 0; i < sampleSize; ++i) {
        for (int j = 0; j < ClusterSize; ++j) {
            // Count entries from the current generation
            if (table[i].entry[j].depth8 &&
                (table[i].entry[j].genBound8 >> 2) == (generation8 >> 2)) {
                ++count;
            }
        }
    }

    return count * 1000 / (sampleSize * ClusterSize);
}
