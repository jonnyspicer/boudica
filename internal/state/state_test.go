package state_test

import (
	"math"
	"testing"

	. "github.com/jonnyspicer/boudica/internal/state"

	"github.com/stretchr/testify/assert"
)

const (
	standardStartWhiteRooks   BitBoard = 0b_1000000100000000000000000000000000000000000000000000000000000000
	standardStartWhiteKnights BitBoard = 0b_0100001000000000000000000000000000000000000000000000000000000000
	standardStartWhiteBishops BitBoard = 0b_0010010000000000000000000000000000000000000000000000000000000000
	standardStartWhiteKing    BitBoard = 0b_0001000000000000000000000000000000000000000000000000000000000000
	standardStartWhiteQueens  BitBoard = 0b_0000100000000000000000000000000000000000000000000000000000000000
	standardStartWhitePawns   BitBoard = 0b_0000000011111111000000000000000000000000000000000000000000000000
	standardStartBlackRooks   BitBoard = 0b_0000000000000000000000000000000000000000000000000000000010000001
	standardStartBlackKnights BitBoard = 0b_0000000000000000000000000000000000000000000000000000000001000010
	standardStartBlackBishops BitBoard = 0b_0000000000000000000000000000000000000000000000000000000000100100
	standardStartBlackKing    BitBoard = 0b_00000000000000000000000000000000000000000000000000000000000010000
	standardStartBlackQueens  BitBoard = 0b_000000000000000000000000000000000000000000000000000000000001000
	standardStartBlackPawns   BitBoard = 0b_0000000000000000000000000000000000000000000000001111111100000000
)

const standardStart Fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

func TestNewStandardGame(t *testing.T) {
	// TODO: this should test state not casting stuff to runes
	standard, err := NewStandardGame()

	actRunes, _ := standard.Board.ToRunes()

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

	expRunes, _ := expBoard.ToRunes()

	assert.Equal(t, expRunes, actRunes)
	assert.Nil(t, err)
}

func TestNewCMLXGame(t *testing.T) {
	CMLX, err := NewCMLXGame()

	t.Run("mirrored-pieces", func(t *testing.T) {
		// each side's pieces should be in the same positions, 56 squares apart
		assert.Equal(t, CMLX.Board.WhiteRooks>>56, CMLX.Board.BlackRooks)
		assert.Equal(t, CMLX.Board.WhiteKnights>>56, CMLX.Board.BlackKnights)
		assert.Equal(t, CMLX.Board.WhiteBishops>>56, CMLX.Board.BlackBishops)
		assert.Equal(t, CMLX.Board.WhiteQueens>>56, CMLX.Board.BlackQueens)
		assert.Equal(t, CMLX.Board.WhiteKing>>56, CMLX.Board.BlackKing)
	})

	t.Run("opposite-colour-bishops", func(t *testing.T) {
		bb := int(CMLX.Board.BlackBishops)

		// Find the value of the least significant 1, ie the right-most bishop
		b1 := bb - bb&(bb-1)
		// Find the value when the least significant 1 is eliminated, ie the left-most bishop
		b2 := bb - b1

		// Take the binary logarithm of each value to get the square number of each bishop;
		// one should be on a light square and one on a dark, hence one value should be odd
		// and the other even
		assert.True(t, int(math.Log2(float64(b1)))%2+int(math.Log2(float64(b2)))%2 == 1)
	})

	t.Run("king-between-rooks", func(t *testing.T) {
		br := int(CMLX.Board.BlackRooks)
		bk := int(CMLX.Board.BlackKing)

		// as above, find the positions of each rook
		r1 := br - br&(br-1)
		r2 := br - r1

		// and assert that the king is in between the two
		assert.True(t, r1 < bk && bk < r2)
	})

	assert.Nil(t, err)
}

func TestNewCustomPosition(t *testing.T) {
	// TODO: this should test state not casting stuff to runes
	custom, err := NewCustomPosition(standardStart)

	actRunes, _ := custom.Board.ToRunes()

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

	expRunes, _ := expBoard.ToRunes()

	assert.Equal(t, expRunes, actRunes)
	assert.Nil(t, err)
}
