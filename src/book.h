#ifndef BOOK_H
#define BOOK_H

#include "types.h"
#include <string>
#include <vector>
#include <unordered_map>

// Forward declaration
class Position;

namespace Boudica {

namespace Book {

// Initialize the opening book
void init();

// Probe the book for the current position
// Returns MOVE_NONE if no book move found
Move probe(const Position& pos);

// Check if a position is in the book
bool in_book(const Position& pos);

// Book entry: maps position hash to list of moves with weights
struct BookEntry {
    Move move;
    int weight;  // Higher = more likely to be chosen
};

} // namespace Book

} // namespace Boudica

#endif // BOOK_H
