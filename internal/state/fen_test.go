package state_test

import (
	"fmt"
	"testing"

	. "github.com/jonnyspicer/boudica/internal/state"

	"github.com/stretchr/testify/assert"
)

func TestToBoard(t *testing.T) {
	actBoard, err := standardStart.ToBoard()

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

	fmt.Println("Actual bitboard:")
	bitboardsToRunes(actBoard)
	fmt.Println("Expected bitboard:")
	bitboardsToRunes(expBoard)

	assert.Nil(t, err)
	assert.Equal(t, expBoard, actBoard)
}
