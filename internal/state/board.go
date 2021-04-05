package state

type BitBoard uint64

type Board struct {
	WhiteRooks   BitBoard
	WhiteKnights BitBoard
	WhiteBishops BitBoard
	WhiteQueens  BitBoard
	WhiteKing    BitBoard
	WhitePawns   BitBoard
	BlackRooks   BitBoard
	BlackKnights BitBoard
	BlackBishops BitBoard
	BlackQueens  BitBoard
	BlackKing    BitBoard
	BlackPawns   BitBoard
}
