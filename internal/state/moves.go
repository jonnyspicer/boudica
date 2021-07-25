package state

import (
	"math/bits"
	"strconv"
)

const (
	fileA          BitBoard = 0b_0000000100000001000000010000000100000001000000010000000100000001
	fileH          BitBoard = 0b_1000000010000000100000001000000010000000100000001000000010000000
	fileAB         BitBoard = 0b_0000001100000011000000110000001100000011000000110000001100000011
	fileGH         BitBoard = 0b_1100000011000000110000001100000011000000110000001100000011000000
	rank1          BitBoard = 0b_1111111100000000000000000000000000000000000000000000000000000000
	rank4          BitBoard = 0b_0000000000000000000000001111111100000000000000000000000000000000
	rank5          BitBoard = 0b_0000000000000000000000000000000011111111000000000000000000000000
	rank8          BitBoard = 0b_0000000000000000000000000000000000000000000000000000000011111111
	centre         BitBoard = 0b_0000000000000000000000000001100000011000000000000000000000000000
	extendedCentre BitBoard = 0b_0000000000000000001111000011110000111100001111000000000000000000
	kingSide       BitBoard = 0b_1111000011110000111100001111000011110000111100001111000011110000
	queenSide      BitBoard = 0b_0000111100001111000011110000111100001111000011110000111100001111
	kingB7         BitBoard = 0b_0000000000000000000000000000000000000000000001110000010100000111
	knightC6       BitBoard = 0b_0000000000000000000000000000101000010001000000000001000100001010
)

type MoveGenerator struct {
	state                 State
	whiteAvailableSquares BitBoard
	blackPieces           BitBoard
	emptySquares          BitBoard
	history               string
	// TODO: there has to be a better way to do this... a move struct? [][x1, y1, x2, y2]? is it worth benchmarking this?
	possibleMoves string
}

func NewMoveGenerator(s State, history string) MoveGenerator {
	mg := MoveGenerator{state: s, history: history}
	// All squares that white pieces could conceivably move to, ie ones that aren't occupied by white pieces or the black king
	mg.whiteAvailableSquares = ^(s.Board.WhitePawns | s.Board.WhiteKnights | s.Board.WhiteBishops | s.Board.WhiteRooks | s.Board.WhiteQueens | s.Board.WhiteKing | s.Board.BlackKing)
	// All squares occupied by black pieces, bar the black king
	mg.blackPieces = s.Board.BlackPawns | s.Board.BlackKnights | s.Board.BlackBishops | s.Board.BlackRooks | s.Board.BlackQueens
	mg.emptySquares = ^(s.Board.WhitePawns | s.Board.WhiteKnights | s.Board.WhiteBishops | s.Board.WhiteRooks | s.Board.WhiteQueens | s.Board.WhiteKing | s.Board.BlackPawns | s.Board.BlackKnights | s.Board.BlackBishops | s.Board.BlackRooks | s.Board.BlackQueens | s.Board.BlackKing)

	return mg
}

func (mg *MoveGenerator) GeneratePossibleMoves() {
	mg.PossibleWhitePawnMoves()
}

func (mg *MoveGenerator) PossibleWhitePawnMoves() {
	// TODO: refactor this to be separate functions for each type of move?
	wp := mg.state.Board.WhitePawns

	// TODO: unflip these boards
	// possible captures to the right
	pm := (wp >> 7) & mg.blackPieces &^ rank8 &^ fileA
	for i := bits.TrailingZeros64(uint64(pm)); i < 64-bits.LeadingZeros64(uint64(pm)); i++ {
		if (pm>>i)&1 == 1 {
			// if a capture is possible, appends a move to the string in the form "x1y1x2y2"
			// uses 8th rank and 1st file as 0, in keeping with using a8 = 0 and h1 = 63
			mg.possibleMoves += strconv.Itoa(i/8+1) + strconv.Itoa(i%8-1) + strconv.Itoa(i/8) + strconv.Itoa(i%8)
		}
	}

	// possible captures to the left
	pm = (wp >> 9) & mg.blackPieces &^ rank8 &^ fileH
	for i := bits.TrailingZeros64(uint64(pm)); i < 64-bits.LeadingZeros64(uint64(pm)); i++ {
		if (pm>>i)&1 == 1 {
			mg.possibleMoves += strconv.Itoa(i/8+1) + strconv.Itoa(i%8+1) + strconv.Itoa(i/8) + strconv.Itoa(i%8)
		}
	}

	// 1 square forward
	pm = (wp >> 8) & mg.emptySquares &^ rank8
	for i := bits.TrailingZeros64(uint64(pm)); i < 64-bits.LeadingZeros64(uint64(pm)); i++ {
		if (pm>>i)&1 == 1 {
			mg.possibleMoves += strconv.Itoa(i/8+1) + strconv.Itoa(i%8) + strconv.Itoa(i/8) + strconv.Itoa(i%8)
		}
	}

	// 2 squares forward
	pm = (wp >> 16) & mg.emptySquares & (mg.emptySquares >> 8) & rank4
	for i := bits.TrailingZeros64(uint64(pm)); i < 64-bits.LeadingZeros64(uint64(pm)); i++ {
		if (pm>>i)&1 == 1 {
			mg.possibleMoves += strconv.Itoa(i/8+2) + strconv.Itoa(i%8) + strconv.Itoa(i/8) + strconv.Itoa(i%8)
		}
	}
	// promotions by capture to the right
	pm = (wp >> 7) & mg.blackPieces & rank8 &^ fileA
	for i := bits.TrailingZeros64(uint64(pm)); i < 64-bits.LeadingZeros64(uint64(pm)); i++ {
		if (pm>>i)&1 == 1 {
			mg.possibleMoves += strconv.Itoa(i%8-1) + strconv.Itoa(i%8) + "QP" + strconv.Itoa(i%8-1) + strconv.Itoa(i%8) + "RP" + strconv.Itoa(i%8-1) + strconv.Itoa(i%8) + "BP" + strconv.Itoa(i%8-1) + strconv.Itoa(i%8) + "NP"
		}
	}

	// promotions by capture to the left
	pm = (wp >> 9) & mg.blackPieces & rank8 &^ fileH
	for i := bits.TrailingZeros64(uint64(pm)); i < 64-bits.LeadingZeros64(uint64(pm)); i++ {
		if (pm>>i)&1 == 1 {
			mg.possibleMoves += strconv.Itoa(i%8+1) + strconv.Itoa(i%8) + "QP" + strconv.Itoa(i%8+1) + strconv.Itoa(i%8) + "RP" + strconv.Itoa(i%8+1) + strconv.Itoa(i%8) + "BP" + strconv.Itoa(i%8+1) + strconv.Itoa(i%8) + "NP"
		}
	}

	// promotions by 1 square forward
	pm = (wp >> 8) & mg.blackPieces & rank8 &^ fileH
	for i := bits.TrailingZeros64(uint64(pm)); i < 64-bits.LeadingZeros64(uint64(pm)); i++ {
		if (pm>>i)&1 == 1 {
			mg.possibleMoves += strconv.Itoa(i%8) + strconv.Itoa(i%8) + "QP" + strconv.Itoa(i%8+1) + strconv.Itoa(i%8) + "RP" + strconv.Itoa(i%8+1) + strconv.Itoa(i%8) + "BP" + strconv.Itoa(i%8+1) + strconv.Itoa(i%8) + "NP"
		}
	}
}
