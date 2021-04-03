package state_test

import (
	"fmt"
	"testing"

	. "github.com/jonnyspicer/boudica/internal/state"

	"github.com/stretchr/testify/assert"
)

const standardStartWhiteRooks BitBoard = 9295429630892703744
const standardStartWhiteKnights BitBoard = 4755801206503243776
const standardStartWhiteBishops BitBoard = 2594073385365405696
const standardStartWhiteQueens BitBoard = 1152921504606846976
const standardStartWhiteKing BitBoard = 576460752303423488
const standardStartWhitePawns BitBoard = 71776119061217280
const standardStartBlackRooks BitBoard = 129
const standardStartBlackKnights BitBoard = 66
const standardStartBlackBishops BitBoard = 36
const standardStartBlackQueens BitBoard = 16
const standardStartBlackKing BitBoard = 8
const standardStartBlackPawns BitBoard = 65280

const standardStart Fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

// draw array helper method (converts bitboards back into a printable string)

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
			b[i/8][i%8] = 'k'
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

// TODO: make this function private, then mock it
func TestFenToBoard(t *testing.T) {
	actBoard, err := FenToBoard(standardStart)

	expBoard := Board{
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
	}

	assert.Nil(t, err)
	assert.Equal(t, expBoard, actBoard)
}
