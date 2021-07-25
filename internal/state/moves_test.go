package state

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestMoveGenerator_GeneratePossibleMoves(t *testing.T) {
	cases := []struct {
		name     string
		position string
		// TODO: add history
		expectedMoves string
	}{
		// TODO: add more cases
		{"standard-start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 ", "6050615162526353645465556656675760406141624263436444654566466747"},
		{"all-pawns-forward-two", "rnbqkbnr/8/8/pppppppp/PPPPPPPP/8/8/RNBQKBNR w KQkq a6 0 9", "40314132423343344435453646374130423143324433453446354736"},
	}
	// TODO: mock state?

	for _, c := range cases {
		t.Run(c.name, func(t *testing.T) {
			s, _ := NewCustomPosition(Fen(c.position))

			mg := NewMoveGenerator(s, "")
			mg.GeneratePossibleMoves()

			assert.Equal(t, c.expectedMoves, mg.possibleMoves)
		})
	}
}
