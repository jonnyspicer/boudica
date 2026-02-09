#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.h"
#include <cstddef>
#include <iterator>

// Boudica Chess Engine - Move Generation
// Generates pseudo-legal moves; legality checked later by Position::legal()

namespace Boudica {

// Forward declarations
class Position;

// Extended move structure: Move + score for move ordering
struct ExtMove {
    Move move;
    int value;

    operator Move() const { return move; }
    void operator=(Move m) { move = m; }

    // Comparison operators for sorting (highest score first)
    bool operator<(const ExtMove& other) const { return value > other.value; }
    bool operator>(const ExtMove& other) const { return value < other.value; }
    bool operator<=(const ExtMove& other) const { return value >= other.value; }
    bool operator>=(const ExtMove& other) const { return value <= other.value; }
};

inline bool operator<(const ExtMove& m, int v) { return m.value < v; }
inline bool operator<(int v, const ExtMove& m) { return v < m.value; }

// Move generation types
enum GenType {
    CAPTURES,      // All captures (including promotions with capture)
    QUIETS,        // All non-captures (including non-capture promotions)
    QUIET_CHECKS,  // Non-capture moves that give check
    EVASIONS,      // Moves when in check (captures of checker or blocking)
    NON_EVASIONS,  // All pseudo-legal moves when not in check
    LEGAL          // All legal moves (validated)
};

// Template function declarations for move generation
template<GenType T>
ExtMove* generate(const Position& pos, ExtMove* moveList);

// Explicit template specialization declarations
template<> ExtMove* generate<CAPTURES>(const Position& pos, ExtMove* moveList);
template<> ExtMove* generate<QUIETS>(const Position& pos, ExtMove* moveList);
template<> ExtMove* generate<QUIET_CHECKS>(const Position& pos, ExtMove* moveList);
template<> ExtMove* generate<EVASIONS>(const Position& pos, ExtMove* moveList);
template<> ExtMove* generate<NON_EVASIONS>(const Position& pos, ExtMove* moveList);
template<> ExtMove* generate<LEGAL>(const Position& pos, ExtMove* moveList);

// MoveList template class - holds generated moves with iterator support
template<GenType T>
class MoveList {
public:
    explicit MoveList(const Position& pos) : last(generate<T>(pos, moveList)) {}

    // Iterator interface
    const ExtMove* begin() const { return moveList; }
    const ExtMove* end() const { return last; }
    ExtMove* begin() { return moveList; }
    ExtMove* end() { return last; }

    // Size interface
    size_t size() const { return last - moveList; }
    bool empty() const { return moveList == last; }

    // Access operators
    const ExtMove& operator[](size_t i) const { return moveList[i]; }
    ExtMove& operator[](size_t i) { return moveList[i]; }

    // Check if a move exists in the list
    bool contains(Move m) const {
        for (const ExtMove* it = moveList; it != last; ++it) {
            if (it->move == m) return true;
        }
        return false;
    }

private:
    ExtMove moveList[MAX_MOVES];
    ExtMove* last;
};

} // namespace Boudica

#endif // MOVEGEN_H
