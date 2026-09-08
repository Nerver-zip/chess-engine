#include "board/board.h"
#include "eval/eval.h"
#include <gtest/gtest.h>

TEST(EvalTest, StartingPositionIsSymmetricAndZero) {
    Board board = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    int score = Eval::evaluate(board);
    // In starting position, White and Black pieces and PSTs are mirrored
    EXPECT_EQ(score, 0);
}

TEST(EvalTest, WhiteMaterialAdvantageIsPositiveForWhite) {
    // White has a queen, Black only has king and pawns
    Board board = Board::fromFEN("4k3/pppppppp/8/8/8/8/PPPPPPPP/3QK3 w - - 0 1");
    int score = Eval::evaluate(board);
    EXPECT_GT(score, 800);
}

TEST(EvalTest, MaterialDeficitIsNegativeForCurrentSide) {
    // White has only king, Black has queen and king; White to move
    Board board = Board::fromFEN("4k3/8/8/8/8/8/8/3qK3 w - - 0 1");
    int score = Eval::evaluate(board);
    EXPECT_LT(score, -800);
}

TEST(EvalTest, PerspectiveInvertsForBlackToMove) {
    // Black is up a queen and it's Black to move -> score should be positive from Black's
    // perspective
    Board board = Board::fromFEN("4k3/8/8/8/8/8/8/3qK3 b - - 0 1");
    int score = Eval::evaluate(board);
    EXPECT_GT(score, 800);
}
