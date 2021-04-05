package state_test

import (
	"testing"

	. "github.com/jonnyspicer/boudica/internal/state"

	"github.com/stretchr/testify/assert"
)

func TestToBoard(t *testing.T) {
	var f Fen = "rnbqkb1r/pppppppp/5n2/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

	actBoard, err := f.ToBoard()

	expBoard := Board{
		WhiteRooks:   standardStartWhiteRooks,
		WhiteKnights: standardStartWhiteKnights,
		WhiteBishops: standardStartWhiteBishops,
		WhiteQueens:  standardStartWhiteQueens,
		WhiteKing:    standardStartWhiteKing,
		WhitePawns:   standardStartWhitePawns,
		BlackRooks:   standardStartBlackRooks,
		BlackKnights: BitBoard(2097154),
		BlackBishops: standardStartBlackBishops,
		BlackQueens:  standardStartBlackQueens,
		BlackKing:    standardStartBlackKing,
		BlackPawns:   standardStartBlackPawns,
	}

	assert.Nil(t, err)
	assert.Equal(t, expBoard, actBoard)
}

func TestToBoard_Error(t *testing.T) {
	var f Fen = "rnzqkb1r/pppppppp/5n2/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

	_, err := f.ToBoard()

	assert.NotNil(t, err)
}
