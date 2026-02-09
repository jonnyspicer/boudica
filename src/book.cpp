#include "book.h"
#include "position.h"
#include "movegen.h"

#include <random>
#include <ctime>

namespace Boudica {

namespace Book {

// Book storage: position hash -> list of (move, weight) pairs
static std::unordered_map<uint64_t, std::vector<BookEntry>> book_data;

// Random number generator for move selection
static std::mt19937 rng;

// Starting FEN
static const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Helper to add a book line (sequence of moves from starting position)
static void add_line(const std::vector<std::string>& moves, int weight = 100);

// Parse a move in long algebraic notation (e.g., "e2e4")
static Move parse_move(const Position& pos, const std::string& str);

void init() {
    // Seed the random number generator
    rng.seed(static_cast<unsigned>(std::time(nullptr)));

    book_data.clear();

    // =========================================================================
    // WHITE OPENINGS (when we play first)
    // =========================================================================

    // Primary opening moves with high weights
    add_line({"e2e4"}, 200);  // King's pawn - most popular
    add_line({"d2d4"}, 180);  // Queen's pawn - second most popular

    // King's Pawn Openings (1.e4)
    // Italian Game
    add_line({"e2e4", "e7e5", "g1f3", "b8c6", "f1c4"}, 100);
    add_line({"e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "f8c5", "c2c3"}, 100);
    add_line({"e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "g8f6", "d2d3"}, 100);

    // Ruy Lopez
    add_line({"e2e4", "e7e5", "g1f3", "b8c6", "f1b5"}, 120);
    add_line({"e2e4", "e7e5", "g1f3", "b8c6", "f1b5", "a7a6", "b5a4"}, 120);
    add_line({"e2e4", "e7e5", "g1f3", "b8c6", "f1b5", "a7a6", "b5a4", "g8f6", "e1g1"}, 120);

    // Scotch Game
    add_line({"e2e4", "e7e5", "g1f3", "b8c6", "d2d4"}, 90);
    add_line({"e2e4", "e7e5", "g1f3", "b8c6", "d2d4", "e5d4", "f3d4"}, 90);

    // Four Knights
    add_line({"e2e4", "e7e5", "g1f3", "b8c6", "b1c3", "g8f6"}, 80);

    // Sicilian Defense responses
    add_line({"e2e4", "c7c5", "g1f3"}, 100);
    add_line({"e2e4", "c7c5", "g1f3", "d7d6", "d2d4"}, 100);
    add_line({"e2e4", "c7c5", "g1f3", "b8c6", "d2d4"}, 100);
    add_line({"e2e4", "c7c5", "g1f3", "e7e6", "d2d4"}, 100);

    // French Defense responses
    add_line({"e2e4", "e7e6", "d2d4"}, 100);
    add_line({"e2e4", "e7e6", "d2d4", "d7d5", "b1c3"}, 100);
    add_line({"e2e4", "e7e6", "d2d4", "d7d5", "e4e5"}, 90);

    // Caro-Kann responses
    add_line({"e2e4", "c7c6", "d2d4"}, 100);
    add_line({"e2e4", "c7c6", "d2d4", "d7d5", "b1c3"}, 100);

    // Pirc/Modern responses (lower weight - not recommended as Black)
    add_line({"e2e4", "d7d6", "d2d4"}, 50);
    add_line({"e2e4", "g7g6", "d2d4"}, 50);

    // Scandinavian responses (low weight - risky opening)
    add_line({"e2e4", "d7d5", "e4d5"}, 30);

    // Against 1...Nc6 (Nimzovich Defense - low weight, dubious)
    add_line({"e2e4", "b8c6", "d2d4"}, 30);
    add_line({"e2e4", "b8c6", "g1f3"}, 30);

    // Queen's Pawn Openings (1.d4)
    // Queen's Gambit
    add_line({"d2d4", "d7d5", "c2c4"}, 110);
    add_line({"d2d4", "d7d5", "c2c4", "e7e6", "b1c3"}, 110);
    add_line({"d2d4", "d7d5", "c2c4", "c7c6", "g1f3"}, 110);

    // Indian Defenses
    add_line({"d2d4", "g8f6", "c2c4"}, 110);
    add_line({"d2d4", "g8f6", "c2c4", "e7e6", "g1f3"}, 110);
    add_line({"d2d4", "g8f6", "c2c4", "g7g6", "b1c3"}, 110);

    // London System (solid choice)
    add_line({"d2d4", "d7d5", "c1f4"}, 100);
    add_line({"d2d4", "g8f6", "c1f4"}, 100);
    add_line({"d2d4", "d7d5", "g1f3", "g8f6", "c1f4"}, 100);

    // English Opening (1.c4)
    add_line({"c2c4"}, 80);
    add_line({"c2c4", "e7e5", "b1c3"}, 80);
    add_line({"c2c4", "g8f6", "b1c3"}, 80);

    // =========================================================================
    // BLACK RESPONSES (when opponent plays first)
    // =========================================================================

    // Against 1.e4 - e5 (Open Games - safest for engine)
    add_line({"e2e4", "e7e5"}, 150);
    add_line({"e2e4", "e7e5", "g1f3", "b8c6"}, 150);
    add_line({"e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "f8c5"}, 150);
    add_line({"e2e4", "e7e5", "g1f3", "b8c6", "f1b5", "a7a6"}, 150);
    add_line({"e2e4", "e7e5", "g1f3", "b8c6", "d2d4", "e5d4"}, 150);

    // Against 1.e4 - Sicilian Defense
    add_line({"e2e4", "c7c5"}, 120);
    add_line({"e2e4", "c7c5", "g1f3", "d7d6"}, 120);
    add_line({"e2e4", "c7c5", "g1f3", "b8c6"}, 120);

    // Against 1.e4 - French Defense
    add_line({"e2e4", "e7e6"}, 100);
    add_line({"e2e4", "e7e6", "d2d4", "d7d5"}, 100);

    // Against 1.e4 - Caro-Kann
    add_line({"e2e4", "c7c6"}, 100);
    add_line({"e2e4", "c7c6", "d2d4", "d7d5"}, 100);

    // Against 1.d4 - Queen's Gambit Declined
    add_line({"d2d4", "d7d5"}, 100);
    add_line({"d2d4", "d7d5", "c2c4", "e7e6"}, 100);
    add_line({"d2d4", "d7d5", "c2c4", "e7e6", "b1c3", "g8f6"}, 100);

    // Against 1.d4 - Indian Defenses
    add_line({"d2d4", "g8f6"}, 110);
    add_line({"d2d4", "g8f6", "c2c4", "e7e6"}, 110);
    add_line({"d2d4", "g8f6", "c2c4", "g7g6"}, 100);
    add_line({"d2d4", "g8f6", "c2c4", "e7e6", "g1f3", "b7b6"}, 100);

    // Against 1.d4 - Slav
    add_line({"d2d4", "d7d5", "c2c4", "c7c6"}, 100);

    // Against 1.c4 - English
    add_line({"c2c4", "e7e5"}, 90);
    add_line({"c2c4", "g8f6"}, 90);
    add_line({"c2c4", "c7c5"}, 90);

    // Against 1.e4 e5 2.Nc3 (common TSCP response)
    add_line({"e2e4", "e7e5", "b1c3", "g8f6"}, 120);
    add_line({"e2e4", "e7e5", "b1c3", "b8c6"}, 120);
    add_line({"e2e4", "e7e5", "b1c3", "g8f6", "g1f3", "b8c6"}, 120);

    // Against 1.e4 e6 2.Nc3 (French vs Nc3)
    add_line({"e2e4", "e7e6", "b1c3", "d7d5"}, 100);
    add_line({"e2e4", "e7e6", "b1c3", "d7d5", "d2d4", "g8f6"}, 100);

    // Against 1.Nf3 (low weight to not pollute White's first move choice)
    add_line({"g1f3", "d7d5"}, 40);
    add_line({"g1f3", "g8f6"}, 40);

    // Against 1.Nc3 (low weight to not pollute White's first move choice)
    add_line({"b1c3", "d7d5"}, 10);
    add_line({"b1c3", "e7e5"}, 10);
    add_line({"b1c3", "g8f6"}, 10);
}

Move probe(const Position& pos) {
    uint64_t key = pos.key();

    auto it = book_data.find(key);
    if (it == book_data.end()) {
        return MOVE_NONE;
    }

    const std::vector<BookEntry>& entries = it->second;
    if (entries.empty()) {
        return MOVE_NONE;
    }

    // Calculate total weight
    int total_weight = 0;
    for (const auto& entry : entries) {
        total_weight += entry.weight;
    }

    // Select a move based on weights
    std::uniform_int_distribution<int> dist(0, total_weight - 1);
    int r = dist(rng);

    int cumulative = 0;
    for (const auto& entry : entries) {
        cumulative += entry.weight;
        if (r < cumulative) {
            // Verify the move is legal
            MoveList<LEGAL> legal_moves(pos);
            for (const auto& m : legal_moves) {
                if (m.move == entry.move) {
                    return entry.move;
                }
            }
            // Move not legal (shouldn't happen with correct book)
            break;
        }
    }

    return MOVE_NONE;
}

bool in_book(const Position& pos) {
    return book_data.find(pos.key()) != book_data.end();
}

static Move parse_move(const Position& pos, const std::string& str) {
    if (str.length() < 4) return MOVE_NONE;

    File from_file = File(str[0] - 'a');
    Rank from_rank = Rank(str[1] - '1');
    File to_file = File(str[2] - 'a');
    Rank to_rank = Rank(str[3] - '1');

    if (from_file > FILE_H || to_file > FILE_H ||
        from_rank > RANK_8 || to_rank > RANK_8) {
        return MOVE_NONE;
    }

    Square from = make_square(from_file, from_rank);
    Square to = make_square(to_file, to_rank);

    // Check for promotion
    PieceType promo = NO_PIECE_TYPE;
    if (str.length() >= 5) {
        switch (str[4]) {
            case 'q': promo = QUEEN; break;
            case 'r': promo = ROOK; break;
            case 'b': promo = BISHOP; break;
            case 'n': promo = KNIGHT; break;
        }
    }

    // Generate legal moves and find the matching one
    MoveList<LEGAL> legal_moves(pos);

    for (const auto& m : legal_moves) {
        if (from_sq(m.move) == from && to_sq(m.move) == to) {
            if (promo != NO_PIECE_TYPE) {
                if (type_of(m.move) == PROMOTION && promotion_type(m.move) == promo) {
                    return m.move;
                }
            } else {
                return m.move;
            }
        }
    }

    return MOVE_NONE;
}

static void add_line(const std::vector<std::string>& moves, int weight) {
    // Create a position and state info for walking through the line
    Position pos;
    StateInfo states[128];  // Enough for a long line
    int state_idx = 0;

    // Set starting position
    pos.set(START_FEN, &states[state_idx++]);

    for (size_t i = 0; i < moves.size(); ++i) {
        uint64_t key = pos.key();
        Move m = parse_move(pos, moves[i]);

        if (m == MOVE_NONE) {
            // Invalid move in line, stop
            break;
        }

        // Add to book
        auto it = book_data.find(key);
        if (it == book_data.end()) {
            book_data[key] = {{m, weight}};
        } else {
            // Check if move already exists
            bool found = false;
            for (auto& entry : it->second) {
                if (entry.move == m) {
                    entry.weight = std::max(entry.weight, weight);
                    found = true;
                    break;
                }
            }
            if (!found) {
                it->second.push_back({m, weight});
            }
        }

        // Make the move
        pos.do_move(m, states[state_idx++]);
    }
}

} // namespace Book

} // namespace Boudica
