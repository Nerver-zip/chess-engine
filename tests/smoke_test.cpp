#include "board/bitboard.h"
#include "board/board.h"
#include "capy_version.h"
#include "move/move.h"
#include "move/movegen.h"
#include <algorithm>
#include <gtest/gtest.h>

TEST(VersionTest, ReportsProjectVersion) {
    EXPECT_EQ(Capy::version(), "0.1.0");
    EXPECT_EQ(Capy::versionMajor(), 0);
    EXPECT_EQ(Capy::versionMinor(), 1);
    EXPECT_EQ(Capy::versionPatch(), 0);
}

TEST(EngineSmokeTest, GeneratesAndAppliesE2e4FromStartingPosition) {
    Board board = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board.updateAttackBoards();
    const auto moves = MoveGen::generateMoves(board);
    ASSERT_EQ(moves.size(), 20U);

    const auto move = std::find_if(moves.begin(), moves.end(), [](const Move& candidate) {
        return candidate.from == 12U && candidate.to == 28U && (candidate.flags & DOUBLE_PAWN_PUSH);
    });
    ASSERT_NE(move, moves.end());
    EXPECT_EQ(moveToUCI(*move), "e2e4");

    const Board after = board.applyMove(*move);
    EXPECT_FALSE(after.whiteToMove);
    EXPECT_EQ(after.enPassantSquare, 20);
    EXPECT_NE(after.whitePawns & (1ULL << 28), 0ULL);
    EXPECT_EQ(after.whitePawns & (1ULL << 12), 0ULL);
}
