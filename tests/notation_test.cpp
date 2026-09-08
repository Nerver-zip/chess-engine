#include "board/board.h"
#include "move/move.h"
#include "move/movegen.h"
#include <gtest/gtest.h>

TEST(NotationTest, FormatsStandardMovesToUCI) {
    Move m1;
    m1.from = 12; // e2
    m1.to = 28;   // e4
    m1.flags = DOUBLE_PAWN_PUSH;
    m1.promotion = EMPTY;
    EXPECT_EQ(moveToUCI(m1), "e2e4");

    Move m2;
    m2.from = 6; // g1
    m2.to = 21;  // f3
    m2.flags = QUIET;
    m2.promotion = EMPTY;
    EXPECT_EQ(moveToUCI(m2), "g1f3");
}

TEST(NotationTest, FormatsPromotionsToUCI) {
    Move qProm;
    qProm.from = 52; // e7
    qProm.to = 60;   // e8
    qProm.flags = PROMOTION;
    qProm.promotion = WQUEEN;
    EXPECT_EQ(moveToUCI(qProm), "e7e8q");

    Move nProm;
    nProm.from = 48; // a7
    nProm.to = 57;   // b8
    nProm.flags = PROMOTION | CAPTURE;
    nProm.promotion = WKNIGHT;
    EXPECT_EQ(moveToUCI(nProm), "a7b8n");
}

TEST(NotationTest, FormatsMovesToSAN) {
    Board board = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board.updateAttackBoards();

    // 1. e4
    Move e4;
    e4.from = 12;
    e4.to = 28;
    e4.flags = DOUBLE_PAWN_PUSH;
    e4.promotion = EMPTY;
    EXPECT_EQ(moveToSAN(e4, board), "e4");

    // 1. Nf3
    Move nf3;
    nf3.from = 6;
    nf3.to = 21;
    nf3.flags = QUIET;
    nf3.promotion = EMPTY;
    EXPECT_EQ(moveToSAN(nf3, board), "Nf3");
}

TEST(NotationTest, FormatsCastlingToSAN) {
    Board board = Board::fromFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    board.updateAttackBoards();

    Move o_o;
    o_o.from = 4;
    o_o.to = 6;
    o_o.flags = KING_CASTLE;
    o_o.promotion = EMPTY;
    EXPECT_EQ(moveToSAN(o_o, board), "O-O");

    Move o_o_o;
    o_o_o.from = 4;
    o_o_o.to = 2;
    o_o_o.flags = QUEEN_CASTLE;
    o_o_o.promotion = EMPTY;
    EXPECT_EQ(moveToSAN(o_o_o, board), "O-O-O");
}

TEST(NotationTest, FormatsCheckmateToSAN) {
    // White rook on a1 (0), White king on e1 (4). Black king on h8 (63), pawns on g7/h7.
    // 1. Ra8#
    Board board = Board::fromFEN("7k/6pp/8/8/8/8/8/R3K3 w Q - 0 1");
    board.updateAttackBoards();

    Move ra8;
    ra8.from = 0;
    ra8.to = 56;
    ra8.flags = QUIET;
    ra8.promotion = EMPTY;
    EXPECT_EQ(moveToSAN(ra8, board), "Ra8#");
}
