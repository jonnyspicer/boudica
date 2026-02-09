#ifndef BITBOARD_H
#define BITBOARD_H

#include "types.h"
#include <string>

// Boudica Chess Engine - Bitboard Implementation
// Uses magic bitboards for sliding piece attack generation

namespace Boudica {

// ============================================================================
// Bitboard Constants
// ============================================================================

constexpr Bitboard AllSquares  = ~Bitboard(0);
constexpr Bitboard DarkSquares = 0xAA55AA55AA55AA55ULL;
constexpr Bitboard LightSquares = ~DarkSquares;

// File bitboards
constexpr Bitboard FileABB = 0x0101010101010101ULL;
constexpr Bitboard FileBBB = FileABB << 1;
constexpr Bitboard FileCBB = FileABB << 2;
constexpr Bitboard FileDBB = FileABB << 3;
constexpr Bitboard FileEBB = FileABB << 4;
constexpr Bitboard FileFBB = FileABB << 5;
constexpr Bitboard FileGBB = FileABB << 6;
constexpr Bitboard FileHBB = FileABB << 7;

// Rank bitboards
constexpr Bitboard Rank1BB = 0x00000000000000FFULL;
constexpr Bitboard Rank2BB = Rank1BB << (8 * 1);
constexpr Bitboard Rank3BB = Rank1BB << (8 * 2);
constexpr Bitboard Rank4BB = Rank1BB << (8 * 3);
constexpr Bitboard Rank5BB = Rank1BB << (8 * 4);
constexpr Bitboard Rank6BB = Rank1BB << (8 * 5);
constexpr Bitboard Rank7BB = Rank1BB << (8 * 6);
constexpr Bitboard Rank8BB = Rank1BB << (8 * 7);

// Center squares
constexpr Bitboard Center      = (FileDBB | FileEBB) & (Rank4BB | Rank5BB);
constexpr Bitboard QueenSide   = FileABB | FileBBB | FileCBB | FileDBB;
constexpr Bitboard KingSide    = FileEBB | FileFBB | FileGBB | FileHBB;
constexpr Bitboard CenterFiles = FileCBB | FileDBB | FileEBB | FileFBB;

// ============================================================================
// Lookup Tables (extern declarations)
// ============================================================================

extern Bitboard SquareBB[SQUARE_NB];
extern Bitboard FileBB[FILE_NB];
extern Bitboard RankBB[RANK_NB];
extern Bitboard AdjacentFilesBB[FILE_NB];
extern Bitboard ForwardRanksBB[COLOR_NB][RANK_NB];
extern Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];
extern Bitboard LineBB[SQUARE_NB][SQUARE_NB];
extern Bitboard PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];
extern Bitboard PassedPawnSpan[COLOR_NB][SQUARE_NB];
extern Bitboard ForwardFileBB[COLOR_NB][SQUARE_NB];

// ============================================================================
// Magic Bitboard Structures
// ============================================================================

struct Magic {
    Bitboard  mask;      // Mask for relevant occupancy bits
    Bitboard  magic;     // Magic multiplier
    Bitboard* attacks;   // Pointer into attack table
    unsigned  shift;     // Right shift amount (64 - bits in index)

    // Compute index into attack table
    unsigned index(Bitboard occupied) const {
        return unsigned(((occupied & mask) * magic) >> shift);
    }
};

extern Magic RookMagics[SQUARE_NB];
extern Magic BishopMagics[SQUARE_NB];

// Attack tables (allocated in bitboard.cpp)
extern Bitboard RookTable[0x19000];   // ~102KB
extern Bitboard BishopTable[0x1480];  // ~5KB

// ============================================================================
// Inline Bitboard Utilities
// ============================================================================

// Get bitboard with single bit set at square
constexpr Bitboard square_bb(Square s) {
    return Bitboard(1ULL) << s;
}

// Overload operators for square to bitboard conversion
constexpr Bitboard operator&(Bitboard b, Square s) { return b & square_bb(s); }
constexpr Bitboard operator|(Bitboard b, Square s) { return b | square_bb(s); }
constexpr Bitboard operator^(Bitboard b, Square s) { return b ^ square_bb(s); }
constexpr Bitboard& operator&=(Bitboard& b, Square s) { return b &= square_bb(s); }
constexpr Bitboard& operator|=(Bitboard& b, Square s) { return b |= square_bb(s); }
constexpr Bitboard& operator^=(Bitboard& b, Square s) { return b ^= square_bb(s); }

constexpr Bitboard operator|(Square s1, Square s2) { return square_bb(s1) | s2; }

// Test if bit is set
constexpr bool more_than_one(Bitboard b) {
    return b & (b - 1);
}

// Shift bitboard in a direction
template<Direction D>
constexpr Bitboard shift(Bitboard b) {
    return D == NORTH      ? b << 8
         : D == SOUTH      ? b >> 8
         : D == NORTH+NORTH? b << 16
         : D == SOUTH+SOUTH? b >> 16
         : D == EAST       ? (b & ~FileHBB) << 1
         : D == WEST       ? (b & ~FileABB) >> 1
         : D == NORTH_EAST ? (b & ~FileHBB) << 9
         : D == NORTH_WEST ? (b & ~FileABB) << 7
         : D == SOUTH_EAST ? (b & ~FileHBB) >> 7
         : D == SOUTH_WEST ? (b & ~FileABB) >> 9
         : 0;
}

// ============================================================================
// Population Count and Bit Scanning
// ============================================================================

// Count number of set bits
inline int popcount(Bitboard b) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(b);
#elif defined(_MSC_VER)
    return int(__popcnt64(b));
#else
    // Fallback: Kernighan's method
    int count = 0;
    while (b) {
        count++;
        b &= b - 1;
    }
    return count;
#endif
}

// Get least significant bit (returns square index)
inline Square lsb(Bitboard b) {
    assert(b);
#if defined(__GNUC__) || defined(__clang__)
    return Square(__builtin_ctzll(b));
#elif defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward64(&idx, b);
    return Square(idx);
#else
    // Fallback: de Bruijn sequence
    constexpr uint64_t debruijn = 0x03f79d71b4cb0a89ULL;
    constexpr int index64[64] = {
        0,  1, 48,  2, 57, 49, 28,  3,
       61, 58, 50, 42, 38, 29, 17,  4,
       62, 55, 59, 36, 53, 51, 43, 22,
       45, 39, 33, 30, 24, 18, 12,  5,
       63, 47, 56, 27, 60, 41, 37, 16,
       54, 35, 52, 21, 44, 32, 23, 11,
       46, 26, 40, 15, 34, 20, 31, 10,
       25, 14, 19,  9, 13,  8,  7,  6
    };
    return Square(index64[((b & -b) * debruijn) >> 58]);
#endif
}

// Get most significant bit (returns square index)
inline Square msb(Bitboard b) {
    assert(b);
#if defined(__GNUC__) || defined(__clang__)
    return Square(63 ^ __builtin_clzll(b));
#elif defined(_MSC_VER)
    unsigned long idx;
    _BitScanReverse64(&idx, b);
    return Square(idx);
#else
    // Fallback: binary search
    int r = 0;
    if (b > 0xFFFFFFFF) { b >>= 32; r = 32; }
    if (b > 0xFFFF)     { b >>= 16; r += 16; }
    if (b > 0xFF)       { b >>=  8; r +=  8; }
    if (b > 0xF)        { b >>=  4; r +=  4; }
    if (b > 0x3)        { b >>=  2; r +=  2; }
    if (b > 0x1)        {           r +=  1; }
    return Square(r);
#endif
}

// Pop the least significant bit and return its square
inline Square pop_lsb(Bitboard& b) {
    assert(b);
    Square s = lsb(b);
    b &= b - 1;
    return s;
}

// Get frontmost square for a color
inline Square frontmost_sq(Color c, Bitboard b) {
    assert(b);
    return c == WHITE ? msb(b) : lsb(b);
}

inline Square backmost_sq(Color c, Bitboard b) {
    assert(b);
    return c == WHITE ? lsb(b) : msb(b);
}

// ============================================================================
// File and Rank Bitboards
// ============================================================================

inline Bitboard file_bb(File f) {
    return FileBB[f];
}

inline Bitboard file_bb(Square s) {
    return FileBB[file_of(s)];
}

inline Bitboard rank_bb(Rank r) {
    return RankBB[r];
}

inline Bitboard rank_bb(Square s) {
    return RankBB[rank_of(s)];
}

inline Bitboard adjacent_files_bb(Square s) {
    return AdjacentFilesBB[file_of(s)];
}

// ============================================================================
// Forward and Pawn Span Bitboards
// ============================================================================

// Squares in front of a square on the same file
inline Bitboard forward_file_bb(Color c, Square s) {
    return ForwardFileBB[c][s];
}

// All squares that a pawn could potentially attack on its way forward
inline Bitboard pawn_attack_span(Color c, Square s) {
    return ForwardFileBB[c][s] | (adjacent_files_bb(s) & ForwardRanksBB[c][rank_of(s)]);
}

// Span of squares where an enemy pawn would stop our pawn from being passed
inline Bitboard passed_pawn_span(Color c, Square s) {
    return PassedPawnSpan[c][s];
}

// ============================================================================
// Attack Generation
// ============================================================================

// Pawn attacks for a single square
inline Bitboard pawn_attacks_bb(Color c, Square s) {
    return PawnAttacks[c][s];
}

// Pawn attacks for multiple pawns
template<Color C>
constexpr Bitboard pawn_attacks_bb(Bitboard b) {
    return C == WHITE ? shift<NORTH_WEST>(b) | shift<NORTH_EAST>(b)
                      : shift<SOUTH_WEST>(b) | shift<SOUTH_EAST>(b);
}

// Double pawn pushes
template<Color C>
constexpr Bitboard pawn_double_attacks_bb(Bitboard b) {
    return C == WHITE ? shift<NORTH_WEST>(b) & shift<NORTH_EAST>(b)
                      : shift<SOUTH_WEST>(b) & shift<SOUTH_EAST>(b);
}

// Non-slider attacks (knights, kings) - direct lookup
template<PieceType Pt>
inline Bitboard attacks_bb(Square s) {
    static_assert(Pt != PAWN, "Use pawn_attacks_bb for pawns");
    static_assert(Pt == KNIGHT || Pt == KING, "Use attacks_bb with occupancy for sliders");
    return PseudoAttacks[Pt][s];
}

// Sliding piece attacks using magic bitboards
template<PieceType Pt>
inline Bitboard attacks_bb(Square s, Bitboard occupied) {
    static_assert(Pt == BISHOP || Pt == ROOK || Pt == QUEEN,
                  "attacks_bb with occupancy only for sliding pieces");

    if constexpr (Pt == BISHOP) {
        return BishopMagics[s].attacks[BishopMagics[s].index(occupied)];
    }
    else if constexpr (Pt == ROOK) {
        return RookMagics[s].attacks[RookMagics[s].index(occupied)];
    }
    else { // QUEEN
        return attacks_bb<BISHOP>(s, occupied) | attacks_bb<ROOK>(s, occupied);
    }
}

// Generic attack function for any piece type
inline Bitboard attacks_bb(PieceType pt, Square s, Bitboard occupied) {
    switch (pt) {
        case KNIGHT: return attacks_bb<KNIGHT>(s);
        case BISHOP: return attacks_bb<BISHOP>(s, occupied);
        case ROOK:   return attacks_bb<ROOK>(s, occupied);
        case QUEEN:  return attacks_bb<QUEEN>(s, occupied);
        case KING:   return attacks_bb<KING>(s);
        default:     return 0;
    }
}

// ============================================================================
// Line and Between Bitboards
// ============================================================================

// Returns a bitboard of all squares on the line through s1 and s2
// Returns 0 if squares are not on a line (diagonal, rank, or file)
inline Bitboard line_bb(Square s1, Square s2) {
    return LineBB[s1][s2];
}

// Returns a bitboard of all squares strictly between s1 and s2
// Returns 0 if squares are not on a line or are adjacent
inline Bitboard between_bb(Square s1, Square s2) {
    return BetweenBB[s1][s2];
}

// Check if three squares are aligned (on same rank, file, or diagonal)
inline bool aligned(Square s1, Square s2, Square s3) {
    return line_bb(s1, s2) & s3;
}

// ============================================================================
// Pawn Structure Helpers
// ============================================================================

// Squares that would make pawns on bb backward
template<Color C>
constexpr Bitboard pawn_attack_span_bb(Bitboard b) {
    return C == WHITE ? shift<NORTH_WEST>(b) | shift<NORTH_EAST>(b)
                      : shift<SOUTH_WEST>(b) | shift<SOUTH_EAST>(b);
}

// ============================================================================
// Initialization
// ============================================================================

void init_bitboards();

// ============================================================================
// Debug Utilities
// ============================================================================

// Pretty print a bitboard
std::string pretty(Bitboard b);

} // namespace Boudica

#endif // BITBOARD_H
