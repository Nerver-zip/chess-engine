#include "board/bitboard.h"
#include "board/board.h"
#include "board/piece.h"
#include <gtest/gtest.h>

TEST(BoardTest, ParsesStartingPositionFEN) {
    Board board = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_TRUE(board.whiteToMove);
    EXPECT_EQ(board.castlingRights, 15); // 1 | 2 | 4 | 8 = 15 (KQkq)
    EXPECT_EQ(board.enPassantSquare, -1);

    // Check piece counts using popcount
    EXPECT_EQ(__builtin_popcountll(board.whitePawns), 8);
    EXPECT_EQ(__builtin_popcountll(board.whiteKnights), 2);
    EXPECT_EQ(__builtin_popcountll(board.whiteBishops), 2);
    EXPECT_EQ(__builtin_popcountll(board.whiteRooks), 2);
    EXPECT_EQ(__builtin_popcountll(board.whiteQueens), 1);
    EXPECT_EQ(__builtin_popcountll(board.whiteKing), 1);

    EXPECT_EQ(__builtin_popcountll(board.blackPawns), 8);
    EXPECT_EQ(__builtin_popcountll(board.blackKnights), 2);
    EXPECT_EQ(__builtin_popcountll(board.blackBishops), 2);
    EXPECT_EQ(__builtin_popcountll(board.blackRooks), 2);
    EXPECT_EQ(__builtin_popcountll(board.blackQueens), 1);
    EXPECT_EQ(__builtin_popcountll(board.blackKing), 1);

    EXPECT_EQ(__builtin_popcountll(board.allPieces()), 32);
}

TEST(BoardTest, PieceAtIdentifiesPiecesCorrectly) {
    Board board = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(board.pieceAt(0), WROOK);   // a1
    EXPECT_EQ(board.pieceAt(1), WKNIGHT); // b1
    EXPECT_EQ(board.pieceAt(2), WBISHOP); // c1
    EXPECT_EQ(board.pieceAt(3), WQUEEN);  // d1
    EXPECT_EQ(board.pieceAt(4), WKING);   // e1
    EXPECT_EQ(board.pieceAt(12), WPAWN);  // e2

    EXPECT_EQ(board.pieceAt(56), BROOK); // a8
    EXPECT_EQ(board.pieceAt(60), BKING); // e8
    EXPECT_EQ(board.pieceAt(52), BPAWN); // e7

    EXPECT_EQ(board.pieceAt(28), EMPTY); // e4 empty
}

TEST(BoardTest, ParsesCustomFENWithEnPassantAndPartialCastling) {
    Board board = Board::fromFEN("r3k2r/8/8/4Pp2/8/8/8/R3K2R w Kq f6 0 1");
    EXPECT_TRUE(board.whiteToMove);
    EXPECT_EQ(board.castlingRights, 1 | 8); // K (1) and q (8)
    // f6 is file 5 (f), rank 5 (6th rank) -> 5 * 8 + 5 = 45
    EXPECT_EQ(board.enPassantSquare, 45);
}

TEST(BoardTest, AttackMapsCalculation) {
    Board board = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board.updateAttackBoards();

    // From starting position, e3 and d3 are attacked by pawns
    EXPECT_NE(board.whiteAttacks & (1ULL << 20), 0ULL); // e3
    EXPECT_NE(board.whiteAttacks & (1ULL << 19), 0ULL); // d3

    // c3 and f3 are attacked by knights (b1 and g1)
    EXPECT_NE(board.whiteAttacks & (1ULL << 18), 0ULL); // c3
    EXPECT_NE(board.whiteAttacks & (1ULL << 21), 0ULL); // f3

    // Black attacks e6 and d6
    EXPECT_NE(board.blackAttacks & (1ULL << 44), 0ULL); // e6
    EXPECT_NE(board.blackAttacks & (1ULL << 43), 0ULL); // d6
}

TEST(BoardTest, AttackersToFunction) {
    // White knight on c3 (18), White bishop on b1 (1) attacking e4 (28)
    // Diagonal from b1 (1): c2 (10), d3 (19), e4 (28)
    Board board = Board::fromFEN("8/8/8/8/4p3/2N5/8/1B2K2k w - - 0 1");
    uint64_t occ = board.allPieces();
    uint64_t attackers = board.attackersTo(28, occ);

    // Both c3 knight and b1 bishop attack e4
    EXPECT_NE(attackers & (1ULL << 18), 0ULL);
    EXPECT_NE(attackers & (1ULL << 1), 0ULL);
    EXPECT_EQ(__builtin_popcountll(attackers), 2);
}
