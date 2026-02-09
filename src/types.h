#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>
#include <cassert>

// Boudica Chess Engine - Core Type Definitions

// Basic types
using Bitboard = uint64_t;
using Key = uint64_t;  // Zobrist hash key

// Value type for evaluation scores
enum Value : int {
    VALUE_ZERO = 0,
    VALUE_DRAW = 0,
    VALUE_KNOWN_WIN = 10000,
    VALUE_MATE = 32000,
    VALUE_INFINITE = 32001,
    VALUE_NONE = 32002,

    VALUE_MATE_IN_MAX_PLY = VALUE_MATE - 256,
    VALUE_MATED_IN_MAX_PLY = -VALUE_MATE_IN_MAX_PLY,

    // Piece values (centipawns)
    PawnValue   = 100,
    KnightValue = 320,
    BishopValue = 330,
    RookValue   = 500,
    QueenValue  = 900
};

// Depth type for search depth
enum Depth : int {
    DEPTH_ZERO = 0,
    DEPTH_QS_CHECKS = 0,
    DEPTH_QS_NO_CHECKS = -1,
    DEPTH_NONE = -6,
    DEPTH_MAX = 128
};

// Color
enum Color : int {
    WHITE, BLACK, COLOR_NB = 2
};

constexpr Color operator~(Color c) { return Color(c ^ BLACK); }

// Piece types
enum PieceType : int {
    NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    ALL_PIECES = 0, PIECE_TYPE_NB = 8
};

// Pieces (combine type + color)
enum Piece : int {
    NO_PIECE,
    W_PAWN = PAWN,     W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = PAWN + 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
};

constexpr Piece make_piece(Color c, PieceType pt) {
    return Piece((c << 3) + pt);
}

constexpr Color color_of(Piece p) {
    return Color(p >> 3);
}

constexpr PieceType type_of(Piece p) {
    return PieceType(p & 7);
}

// Squares
enum Square : int {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE,
    SQUARE_NB = 64
};

// Files and Ranks
enum File : int { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB };
enum Rank : int { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB };

constexpr Square make_square(File f, Rank r) {
    return Square((r << 3) + f);
}

constexpr File file_of(Square s) {
    return File(s & 7);
}

constexpr Rank rank_of(Square s) {
    return Rank(s >> 3);
}

constexpr Rank relative_rank(Color c, Rank r) {
    return Rank(r ^ (c * 7));
}

constexpr Rank relative_rank(Color c, Square s) {
    return relative_rank(c, rank_of(s));
}

constexpr Square relative_square(Color c, Square s) {
    return Square(s ^ (c * 56));
}

// Directions for piece movement
enum Direction : int {
    NORTH = 8,
    EAST  = 1,
    SOUTH = -NORTH,
    WEST  = -EAST,

    NORTH_EAST = NORTH + EAST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST,
    NORTH_WEST = NORTH + WEST
};

constexpr Direction pawn_push(Color c) {
    return c == WHITE ? NORTH : SOUTH;
}

// Move encoding: 16-bit
// bits 0-5: from square
// bits 6-11: to square
// bits 12-13: promotion piece type (0=knight, 1=bishop, 2=rook, 3=queen)
// bits 14-15: move type (0=normal, 1=promotion, 2=en passant, 3=castling)
enum Move : uint16_t {
    MOVE_NONE = 0,
    MOVE_NULL = 65
};

enum MoveType {
    NORMAL,
    PROMOTION = 1 << 14,
    EN_PASSANT = 2 << 14,
    CASTLING = 3 << 14
};

constexpr Move make_move(Square from, Square to) {
    return Move((from) | (to << 6));
}

template<MoveType T>
constexpr Move make(Square from, Square to, PieceType pt = KNIGHT) {
    return Move(T + ((pt - KNIGHT) << 12) + (from) + (to << 6));
}

constexpr Square from_sq(Move m) {
    return Square(m & 0x3F);
}

constexpr Square to_sq(Move m) {
    return Square((m >> 6) & 0x3F);
}

constexpr MoveType type_of(Move m) {
    return MoveType(m & (3 << 14));
}

constexpr PieceType promotion_type(Move m) {
    return PieceType(((m >> 12) & 3) + KNIGHT);
}

constexpr bool is_ok(Move m) {
    return from_sq(m) != to_sq(m);
}

// Castling rights
enum CastlingRights {
    NO_CASTLING,
    WHITE_OO = 1,
    WHITE_OOO = 2,
    BLACK_OO = 4,
    BLACK_OOO = 8,

    KING_SIDE = WHITE_OO | BLACK_OO,
    QUEEN_SIDE = WHITE_OOO | BLACK_OOO,
    WHITE_CASTLING = WHITE_OO | WHITE_OOO,
    BLACK_CASTLING = BLACK_OO | BLACK_OOO,
    ANY_CASTLING = WHITE_CASTLING | BLACK_CASTLING,

    CASTLING_RIGHT_NB = 16
};

constexpr CastlingRights operator&(CastlingRights a, CastlingRights b) {
    return CastlingRights(int(a) & int(b));
}

constexpr CastlingRights operator|(CastlingRights a, CastlingRights b) {
    return CastlingRights(int(a) | int(b));
}

constexpr CastlingRights& operator&=(CastlingRights& a, CastlingRights b) {
    return a = a & b;
}

constexpr CastlingRights& operator|=(CastlingRights& a, CastlingRights b) {
    return a = a | b;
}

constexpr CastlingRights operator~(CastlingRights c) {
    return CastlingRights(~int(c));
}

// Bound type for transposition table
enum Bound {
    BOUND_NONE,
    BOUND_UPPER,
    BOUND_LOWER,
    BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};

// Enable arithmetic on enums
#define ENABLE_INCR_OPERATORS_ON(T)                                 \
constexpr T& operator++(T& d) { return d = T(int(d) + 1); }         \
constexpr T& operator--(T& d) { return d = T(int(d) - 1); }

ENABLE_INCR_OPERATORS_ON(Square)
ENABLE_INCR_OPERATORS_ON(File)
ENABLE_INCR_OPERATORS_ON(Rank)
ENABLE_INCR_OPERATORS_ON(PieceType)
ENABLE_INCR_OPERATORS_ON(Piece)

#define ENABLE_BASE_OPERATORS_ON(T)                                 \
constexpr T operator+(T d1, int d2) { return T(int(d1) + d2); }     \
constexpr T operator-(T d1, int d2) { return T(int(d1) - d2); }     \
constexpr T operator-(T d) { return T(-int(d)); }                   \
constexpr T& operator+=(T& d1, int d2) { return d1 = d1 + d2; }     \
constexpr T& operator-=(T& d1, int d2) { return d1 = d1 - d2; }

ENABLE_BASE_OPERATORS_ON(Value)
ENABLE_BASE_OPERATORS_ON(Depth)
ENABLE_BASE_OPERATORS_ON(Direction)
ENABLE_BASE_OPERATORS_ON(Square)

constexpr Square operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
constexpr Square operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
constexpr Square& operator+=(Square& s, Direction d) { return s = s + d; }
constexpr Square& operator-=(Square& s, Direction d) { return s = s - d; }

// Max moves per position (generous upper bound)
constexpr int MAX_MOVES = 256;
constexpr int MAX_PLY = 128;

// Score structure for tapered eval
struct Score {
    int mg;  // Middlegame
    int eg;  // Endgame

    constexpr Score() : mg(0), eg(0) {}
    constexpr Score(int m, int e) : mg(m), eg(e) {}

    constexpr Score operator+(Score s) const { return Score(mg + s.mg, eg + s.eg); }
    constexpr Score operator-(Score s) const { return Score(mg - s.mg, eg - s.eg); }
    constexpr Score operator*(int i) const { return Score(mg * i, eg * i); }
    constexpr Score& operator+=(Score s) { mg += s.mg; eg += s.eg; return *this; }
    constexpr Score& operator-=(Score s) { mg -= s.mg; eg -= s.eg; return *this; }
};

constexpr Score S(int mg, int eg) { return Score(mg, eg); }

// Utility functions
inline std::string square_to_string(Square s) {
    return std::string{char('a' + file_of(s)), char('1' + rank_of(s))};
}

inline Square string_to_square(const std::string& s) {
    return make_square(File(s[0] - 'a'), Rank(s[1] - '1'));
}

#undef ENABLE_INCR_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON

#endif // TYPES_H
