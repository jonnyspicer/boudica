package state_test

import (
	"fmt"
	"testing"

	. "github.com/jonnyspicer/boudica/internal/state"

	"github.com/stretchr/testify/assert"
)

const standardStartWhiteRooks BitBoard = 0b_1000000100000000000000000000000000000000000000000000000000000000
const standardStartWhiteKnights BitBoard = 0b_0100001000000000000000000000000000000000000000000000000000000000
const standardStartWhiteBishops BitBoard = 0b_0010010000000000000000000000000000000000000000000000000000000000
const standardStartWhiteKing BitBoard = 0b_0001000000000000000000000000000000000000000000000000000000000000
const standardStartWhiteQueens BitBoard = 0b_0000100000000000000000000000000000000000000000000000000000000000
const standardStartWhitePawns BitBoard = 0b_0000000011111111000000000000000000000000000000000000000000000000
const standardStartBlackRooks BitBoard = 0b_0000000000000000000000000000000000000000000000000000000010000001
const standardStartBlackKnights BitBoard = 0b_0000000000000000000000000000000000000000000000000000000001000010
const standardStartBlackBishops BitBoard = 0b_0000000000000000000000000000000000000000000000000000000000100100
const standardStartBlackKing BitBoard = 0b_00000000000000000000000000000000000000000000000000000000000010000
const standardStartBlackQueens BitBoard = 0b_000000000000000000000000000000000000000000000000000000000001000
const standardStartBlackPawns BitBoard = 0b_0000000000000000000000000000000000000000000000001111111100000000

const standardStart Fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

// Func bitboardsToRunes is a testing/debugging method to
// allow for human readable output from bitboards
func bitboardsToRunes(board Board) ([8][8]rune, error) {
	b := [8][8]rune{}

	for i := 0; i < 64; i++ {
		if ((board.WhiteRooks >> i) & 1) == 1 {
			b[i/8][i%8] = 'R'
		} else if ((board.WhiteKnights >> i) & 1) == 1 {
			b[i/8][i%8] = 'N'
		} else if ((board.WhiteBishops >> i) & 1) == 1 {
			b[i/8][i%8] = 'B'
		} else if ((board.WhiteQueens >> i) & 1) == 1 {
			b[i/8][i%8] = 'Q'
		} else if ((board.WhiteKing >> i) & 1) == 1 {
			b[i/8][i%8] = 'K'
		} else if ((board.WhitePawns >> i) & 1) == 1 {
			b[i/8][i%8] = 'P'
		} else if ((board.BlackRooks >> i) & 1) == 1 {
			b[i/8][i%8] = 'r'
		} else if ((board.BlackKnights >> i) & 1) == 1 {
			b[i/8][i%8] = 'n'
		} else if ((board.BlackBishops >> i) & 1) == 1 {
			b[i/8][i%8] = 'b'
		} else if ((board.BlackQueens >> i) & 1) == 1 {
			b[i/8][i%8] = 'q'
		} else if ((board.BlackKing >> i) & 1) == 1 {
			b[i/8][i%8] = 'k'
		} else if ((board.BlackPawns >> i) & 1) == 1 {
			b[i/8][i%8] = 'p'
		} else {
			b[i/8][i%8] = ' '
		}
	}

	for _, j := range b {
		for _, k := range j {
			fmt.Printf(string(k) + ",")
		}
		fmt.Printf("\n")
	}

	return b, nil
}

func TestNewStandardGame(t *testing.T) {
	standard, err := NewStandardGame()

	actRunes, _ := bitboardsToRunes(standard.Board)

	expRunes, _ := bitboardsToRunes(Board{
		WhiteRooks:   standardStartWhiteRooks,
		WhiteKnights: standardStartWhiteKnights,
		WhiteBishops: standardStartWhiteBishops,
		WhiteQueens:  standardStartWhiteQueens,
		WhiteKing:    standardStartWhiteKing,
		WhitePawns:   standardStartWhitePawns,
		BlackRooks:   standardStartBlackRooks,
		BlackKnights: standardStartBlackKnights,
		BlackBishops: standardStartBlackBishops,
		BlackQueens:  standardStartBlackQueens,
		BlackKing:    standardStartBlackKing,
		BlackPawns:   standardStartBlackPawns,
	})

	assert.Equal(t, expRunes, actRunes)
	assert.Nil(t, err)
}

func TestNewCMLXGame(t *testing.T) {
	// TODO: test that black and white pieces are in the same arrangement, should be doable with exponents
	// TODO: test that bishops are on opposite colour squares (should be possible using logs, find position of each bishop and then check one is odd one is even)
	// TODO: test king is between rooks (again use logs, find position of all three pieces and check king is in the middle)
	_, err := NewCMLXGame()

	assert.Nil(t, err)
}

func TestNewCustomPosition(t *testing.T) {
	custom, err := NewCustomPosition(standardStart)

	actRunes, _ := bitboardsToRunes(custom.Board)

	expRunes, _ := bitboardsToRunes(Board{
		WhiteRooks:   standardStartWhiteRooks,
		WhiteKnights: standardStartWhiteKnights,
		WhiteBishops: standardStartWhiteBishops,
		WhiteQueens:  standardStartWhiteQueens,
		WhiteKing:    standardStartWhiteKing,
		WhitePawns:   standardStartWhitePawns,
		BlackRooks:   standardStartBlackRooks,
		BlackKnights: standardStartBlackKnights,
		BlackBishops: standardStartBlackBishops,
		BlackQueens:  standardStartBlackQueens,
		BlackKing:    standardStartBlackKing,
		BlackPawns:   standardStartBlackPawns,
	})

	assert.Equal(t, expRunes, actRunes)
	assert.Nil(t, err)
}
