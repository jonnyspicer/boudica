package move

import (
	"github.com/jonnyspicer/boudica/internal/state"
)

const (
	fileA          state.BitBoard = 0b_0000000100000001000000010000000100000001000000010000000100000001
	fileH          state.BitBoard = 0b_1000000010000000100000001000000010000000100000001000000010000000
	fileAB         state.BitBoard = 0b_0000001100000011000000110000001100000011000000110000001100000011
	fileGH         state.BitBoard = 0b_1100000011000000110000001100000011000000110000001100000011000000
	rank1          state.BitBoard = 0b_1111111100000000000000000000000000000000000000000000000000000000
	rank4          state.BitBoard = 0b_0000000000000000000000001111111100000000000000000000000000000000
	rank5          state.BitBoard = 0b_0000000000000000000000000000000011111111000000000000000000000000
	rank8          state.BitBoard = 0b_0000000000000000000000000000000000000000000000000000000011111111
	centre         state.BitBoard = 0b_0000000000000000000000000001100000011000000000000000000000000000
	extendedCentre state.BitBoard = 0b_0000000000000000001111000011110000111100001111000000000000000000
	kingSide       state.BitBoard = 0b_1111000011110000111100001111000011110000111100001111000011110000
	queenSide      state.BitBoard = 0b_0000111100001111000011110000111100001111000011110000111100001111
	kingB7         state.BitBoard = 0b_0000000000000000000000000000000000000000000001110000010100000111
	knightC6       state.BitBoard = 0b_0000000000000000000000000000101000010001000000000001000100001010
)

type Generator struct {
	// TODO: should this be refactored to be a Board, and can decouple moveGen and State?
	Board                 state.Board
	whiteAvailableSquares state.BitBoard
	blackPieces           state.BitBoard
	emptySquares          state.BitBoard
	history               string
	// TODO: there has to be a better way to do this... a move struct? [][x1, y1, x2, y2]? is it worth benchmarking this?
	possibleMoves string
}

func NewGenerator(b state.Board, history string) Generator {
	mg := Generator{Board: b, history: history}
	// All squares that white pieces could conceivably move to, ie ones that aren't occupied by white pieces or the black king
	mg.whiteAvailableSquares = ^(b.WhitePawns | b.WhiteKnights | b.WhiteBishops | b.WhiteRooks | b.WhiteQueens | b.WhiteKing | b.BlackKing)
	// All squares occupied by black pieces, bar the black king
	mg.blackPieces = b.BlackPawns | b.BlackKnights | b.BlackBishops | b.BlackRooks | b.BlackQueens
	mg.emptySquares = ^(b.WhitePawns | b.WhiteKnights | b.WhiteBishops | b.WhiteRooks | b.WhiteQueens | b.WhiteKing | b.BlackPawns | b.BlackKnights | b.BlackBishops | b.BlackRooks | b.BlackQueens | b.BlackKing)

	return mg
}

func (g *Generator) PossibleMoves() {
	g.WhitePawnMoves()
}
