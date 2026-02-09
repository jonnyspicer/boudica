#include "bitboard.h"
#include <algorithm>
#include <cstring>
#include <sstream>

// Boudica Chess Engine - Bitboard Implementation
// Magic bitboard attack generation with pre-computed tables

namespace Boudica {

// ============================================================================
// Global Lookup Tables
// ============================================================================

Bitboard SquareBB[SQUARE_NB];
Bitboard FileBB[FILE_NB];
Bitboard RankBB[RANK_NB];
Bitboard AdjacentFilesBB[FILE_NB];
Bitboard ForwardRanksBB[COLOR_NB][RANK_NB];
Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];
Bitboard LineBB[SQUARE_NB][SQUARE_NB];
Bitboard PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];
Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];
Bitboard PassedPawnSpan[COLOR_NB][SQUARE_NB];
Bitboard ForwardFileBB[COLOR_NB][SQUARE_NB];

// Magic bitboard structures
Magic RookMagics[SQUARE_NB];
Magic BishopMagics[SQUARE_NB];

// Attack tables for magic bitboards
Bitboard RookTable[0x19000];   // 102,400 entries
Bitboard BishopTable[0x1480];  // 5,248 entries

// ============================================================================
// Pre-computed Magic Numbers
// These are known-good magic numbers that produce perfect hash functions
// for the magic bitboard attack lookup tables
// ============================================================================

namespace {

// Rook magic numbers (64 squares)
constexpr Bitboard RookMagicNumbers[64] = {
    0x0080001020400080ULL, 0x0040001000200040ULL, 0x0080081000200080ULL, 0x0080040800100080ULL,
    0x0080020400080080ULL, 0x0080010200040080ULL, 0x0080008001000200ULL, 0x0080002040800100ULL,
    0x0000800020400080ULL, 0x0000400020005000ULL, 0x0000801000200080ULL, 0x0000800800100080ULL,
    0x0000800400080080ULL, 0x0000800200040080ULL, 0x0000800100020080ULL, 0x0000800040800100ULL,
    0x0000208000400080ULL, 0x0000404000201000ULL, 0x0000808010002000ULL, 0x0000808008001000ULL,
    0x0000808004000800ULL, 0x0000808002000400ULL, 0x0000010100020004ULL, 0x0000020000408104ULL,
    0x0000208080004000ULL, 0x0000200040005000ULL, 0x0000100080200080ULL, 0x0000080080100080ULL,
    0x0000040080080080ULL, 0x0000020080040080ULL, 0x0000010080800200ULL, 0x0000800080004100ULL,
    0x0000204000800080ULL, 0x0000200040401000ULL, 0x0000100080802000ULL, 0x0000080080801000ULL,
    0x0000040080800800ULL, 0x0000020080800400ULL, 0x0000020001010004ULL, 0x0000800040800100ULL,
    0x0000204000808000ULL, 0x0000200040008080ULL, 0x0000100020008080ULL, 0x0000080010008080ULL,
    0x0000040008008080ULL, 0x0000020004008080ULL, 0x0000010002008080ULL, 0x0000004081020004ULL,
    0x0000204000800080ULL, 0x0000200040008080ULL, 0x0000100020008080ULL, 0x0000080010008080ULL,
    0x0000040008008080ULL, 0x0000020004008080ULL, 0x0000800100020080ULL, 0x0000800041000080ULL,
    0x00FFFCDDFCED714AULL, 0x007FFCDDFCED714AULL, 0x003FFFCDFFD88096ULL, 0x0000040810002101ULL,
    0x0001000204080011ULL, 0x0001000204000801ULL, 0x0001000082000401ULL, 0x0001FFFAABFAD1A2ULL
};

// Bishop magic numbers (64 squares)
constexpr Bitboard BishopMagicNumbers[64] = {
    0x0002020202020200ULL, 0x0002020202020000ULL, 0x0004010202000000ULL, 0x0004040080000000ULL,
    0x0001104000000000ULL, 0x0000821040000000ULL, 0x0000410410400000ULL, 0x0000104104104000ULL,
    0x0000040404040400ULL, 0x0000020202020200ULL, 0x0000040102020000ULL, 0x0000040400800000ULL,
    0x0000011040000000ULL, 0x0000008210400000ULL, 0x0000004104104000ULL, 0x0000002082082000ULL,
    0x0004000808080800ULL, 0x0002000404040400ULL, 0x0001000202020200ULL, 0x0000800802004000ULL,
    0x0000800400A00000ULL, 0x0000200100884000ULL, 0x0000400082082000ULL, 0x0000200041041000ULL,
    0x0002080010101000ULL, 0x0001040008080800ULL, 0x0000208004010400ULL, 0x0000404004010200ULL,
    0x0000840000802000ULL, 0x0000404002011000ULL, 0x0000808001041000ULL, 0x0000404000820800ULL,
    0x0001041000202000ULL, 0x0000820800101000ULL, 0x0000104400080800ULL, 0x0000020080080080ULL,
    0x0000404040040100ULL, 0x0000808100020100ULL, 0x0001010100020800ULL, 0x0000808080010400ULL,
    0x0000820820004000ULL, 0x0000410410002000ULL, 0x0000082088001000ULL, 0x0000002011000800ULL,
    0x0000080100400400ULL, 0x0001010101000200ULL, 0x0002020202000400ULL, 0x0001010101000200ULL,
    0x0000410410400000ULL, 0x0000208208200000ULL, 0x0000002084100000ULL, 0x0000000020880000ULL,
    0x0000001002020000ULL, 0x0000040408020000ULL, 0x0004040404040000ULL, 0x0002020202020000ULL,
    0x0000104104104000ULL, 0x0000002082082000ULL, 0x0000000020841000ULL, 0x0000000000208800ULL,
    0x0000000010020200ULL, 0x0000000404080200ULL, 0x0000040404040400ULL, 0x0002020202020200ULL
};

// Bit counts for magic bitboard shifts
constexpr int RookBits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};

constexpr int BishopBits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};

// ============================================================================
// Helper Functions for Initialization
// ============================================================================

// Safe step: returns the destination square if moving from s in direction d
// is within one king move, otherwise returns SQ_NONE
Square safe_destination(Square s, int step) {
    Square to = Square(s + step);
    return (to >= SQ_A1 && to <= SQ_H8 &&
            std::abs(file_of(s) - file_of(to)) <= 2 &&
            std::abs(rank_of(s) - rank_of(to)) <= 2) ? to : SQ_NONE;
}

// Calculate sliding attacks for a given square and occupancy
// Used during initialization to build attack tables
Bitboard sliding_attack(PieceType pt, Square sq, Bitboard occupied) {
    Bitboard attacks = 0;

    // Direction arrays for rooks and bishops
    const Direction RookDirections[4] = { NORTH, SOUTH, EAST, WEST };
    const Direction BishopDirections[4] = { NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST };

    const Direction* directions = (pt == ROOK) ? RookDirections : BishopDirections;

    for (int i = 0; i < 4; ++i) {
        Direction d = directions[i];
        Square s = sq;

        while (safe_destination(s, d) != SQ_NONE) {
            s += d;
            attacks |= s;
            if (occupied & s) break;
        }
    }

    return attacks;
}

// Generate relevant occupancy mask for a square (excludes edge squares)
Bitboard relevant_occupancy_mask(PieceType pt, Square s) {
    Bitboard result = 0;

    int rk = rank_of(s);
    int fl = file_of(s);

    if (pt == ROOK) {
        // Rank attacks (exclude edges unless we're on them)
        for (int f = fl + 1; f <= 6; ++f) result |= square_bb(make_square(File(f), Rank(rk)));
        for (int f = fl - 1; f >= 1; --f) result |= square_bb(make_square(File(f), Rank(rk)));
        // File attacks (exclude edges unless we're on them)
        for (int r = rk + 1; r <= 6; ++r) result |= square_bb(make_square(File(fl), Rank(r)));
        for (int r = rk - 1; r >= 1; --r) result |= square_bb(make_square(File(fl), Rank(r)));
    }
    else { // BISHOP
        for (int r = rk + 1, f = fl + 1; r <= 6 && f <= 6; ++r, ++f)
            result |= square_bb(make_square(File(f), Rank(r)));
        for (int r = rk + 1, f = fl - 1; r <= 6 && f >= 1; ++r, --f)
            result |= square_bb(make_square(File(f), Rank(r)));
        for (int r = rk - 1, f = fl + 1; r >= 1 && f <= 6; --r, ++f)
            result |= square_bb(make_square(File(f), Rank(r)));
        for (int r = rk - 1, f = fl - 1; r >= 1 && f >= 1; --r, --f)
            result |= square_bb(make_square(File(f), Rank(r)));
    }

    return result;
}

// Set the nth bit in the occupancy according to the mask
Bitboard index_to_occupancy(int index, int bits, Bitboard mask) {
    Bitboard result = 0;
    for (int i = 0; i < bits; ++i) {
        int j = lsb(mask);
        mask &= mask - 1;
        if (index & (1 << i))
            result |= (1ULL << j);
    }
    return result;
}

// Initialize magic bitboard for a single square
void init_magic(Square s, Magic* magics, Bitboard* table,
                const Bitboard* magicNumbers, const int* bits, PieceType pt) {

    Magic& m = magics[s];

    // Set up the magic structure
    m.mask = relevant_occupancy_mask(pt, s);
    m.magic = magicNumbers[s];
    m.shift = 64 - bits[s];

    // Point to the appropriate location in the shared table
    // For the first square, start at the beginning of the table
    if (s == SQ_A1) {
        m.attacks = table;
    } else {
        // Continue from where the previous square left off
        Magic& prev = magics[s - 1];
        m.attacks = prev.attacks + (1 << (64 - prev.shift));
    }

    // Fill in the attack table for all possible occupancies
    int numOccupancies = 1 << bits[s];
    for (int i = 0; i < numOccupancies; ++i) {
        Bitboard occ = index_to_occupancy(i, bits[s], m.mask);
        unsigned idx = m.index(occ);
        m.attacks[idx] = sliding_attack(pt, s, occ);
    }
}

} // anonymous namespace

// ============================================================================
// Initialization Function
// ============================================================================

void init_bitboards() {

    // Initialize square bitboards
    for (Square s = SQ_A1; s <= SQ_H8; ++s)
        SquareBB[s] = 1ULL << s;

    // Initialize file and rank bitboards
    for (File f = FILE_A; f <= FILE_H; ++f)
        FileBB[f] = FileABB << f;

    for (Rank r = RANK_1; r <= RANK_8; ++r)
        RankBB[r] = Rank1BB << (8 * r);

    // Initialize adjacent files
    for (File f = FILE_A; f <= FILE_H; ++f)
        AdjacentFilesBB[f] = (f > FILE_A ? FileBB[f - 1] : 0) |
                             (f < FILE_H ? FileBB[f + 1] : 0);

    // Initialize forward ranks
    for (Rank r = RANK_1; r < RANK_8; ++r)
        ForwardRanksBB[WHITE][r] = ~(ForwardRanksBB[BLACK][r + 1] = ForwardRanksBB[BLACK][r] | RankBB[r]);

    // Initialize forward file and passed pawn span
    for (Color c : { WHITE, BLACK })
        for (Square s = SQ_A1; s <= SQ_H8; ++s) {
            ForwardFileBB[c][s] = ForwardRanksBB[c][rank_of(s)] & FileBB[file_of(s)];
            PassedPawnSpan[c][s] = ForwardRanksBB[c][rank_of(s)] &
                                   (FileBB[file_of(s)] | AdjacentFilesBB[file_of(s)]);
        }

    // Initialize knight attacks
    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
        const int knightMoves[8] = { 17, 15, 10, 6, -6, -10, -15, -17 };

        for (int km : knightMoves) {
            Square to = safe_destination(s, km);
            if (to != SQ_NONE && std::abs(file_of(s) - file_of(to)) <= 2)
                PseudoAttacks[KNIGHT][s] |= to;
        }
    }

    // Initialize king attacks
    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
        const int kingMoves[8] = { 9, 8, 7, 1, -1, -7, -8, -9 };

        for (int km : kingMoves) {
            Square to = safe_destination(s, km);
            if (to != SQ_NONE)
                PseudoAttacks[KING][s] |= to;
        }
    }

    // Initialize pawn attacks
    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
        Bitboard bb = square_bb(s);
        PawnAttacks[WHITE][s] = shift<NORTH_WEST>(bb) | shift<NORTH_EAST>(bb);
        PawnAttacks[BLACK][s] = shift<SOUTH_WEST>(bb) | shift<SOUTH_EAST>(bb);
    }

    // Initialize magic bitboards for sliding pieces
    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
        init_magic(s, BishopMagics, BishopTable, BishopMagicNumbers, BishopBits, BISHOP);
        init_magic(s, RookMagics, RookTable, RookMagicNumbers, RookBits, ROOK);
    }

    // Initialize pseudo attacks for sliding pieces (attacks on empty board)
    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
        PseudoAttacks[BISHOP][s] = attacks_bb<BISHOP>(s, 0);
        PseudoAttacks[ROOK][s]   = attacks_bb<ROOK>(s, 0);
        PseudoAttacks[QUEEN][s]  = PseudoAttacks[BISHOP][s] | PseudoAttacks[ROOK][s];
    }

    // Initialize line and between bitboards
    for (Square s1 = SQ_A1; s1 <= SQ_H8; ++s1) {
        for (Square s2 = SQ_A1; s2 <= SQ_H8; ++s2) {

            // Check if squares are on a diagonal or straight line
            if (PseudoAttacks[BISHOP][s1] & s2) {
                LineBB[s1][s2] = (attacks_bb<BISHOP>(s1, 0) & attacks_bb<BISHOP>(s2, 0)) | s1 | s2;
                BetweenBB[s1][s2] = attacks_bb<BISHOP>(s1, square_bb(s2)) &
                                   attacks_bb<BISHOP>(s2, square_bb(s1));
            }
            else if (PseudoAttacks[ROOK][s1] & s2) {
                LineBB[s1][s2] = (attacks_bb<ROOK>(s1, 0) & attacks_bb<ROOK>(s2, 0)) | s1 | s2;
                BetweenBB[s1][s2] = attacks_bb<ROOK>(s1, square_bb(s2)) &
                                   attacks_bb<ROOK>(s2, square_bb(s1));
            }
            else {
                LineBB[s1][s2] = 0;
                BetweenBB[s1][s2] = 0;
            }
        }
    }
}

// ============================================================================
// Debug Utilities
// ============================================================================

std::string pretty(Bitboard b) {
    std::ostringstream ss;

    ss << "+---+---+---+---+---+---+---+---+\n";

    for (Rank r = RANK_8; r >= RANK_1; --r) {
        for (File f = FILE_A; f <= FILE_H; ++f) {
            Square s = make_square(f, r);
            ss << "| " << ((b & s) ? "X " : ". ");
        }
        ss << "| " << (1 + r) << "\n";
        ss << "+---+---+---+---+---+---+---+---+\n";
    }

    ss << "  a   b   c   d   e   f   g   h\n";

    return ss.str();
}

} // namespace Boudica
