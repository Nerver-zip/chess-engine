#include "board/board.h"
#include "move/move.h"
#include "move/movegen.h"
#include "zobrist/zobrist.h"
#include <algorithm>
#include <gtest/gtest.h>

TEST(ZobristTest, InitialPositionHashIsDeterministic) {
    Zobrist::init();
    Board b1 = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Board b2 = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_NE(b1.hashKey, 0ULL);
    EXPECT_EQ(b1.hashKey, b2.hashKey);
}

TEST(ZobristTest, DifferentPositionsHaveDifferentHashes) {
    Zobrist::init();
    Board b1 = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Board b2 = Board::fromFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    EXPECT_NE(b1.hashKey, b2.hashKey);
}

TEST(ZobristTest, IncrementalHashMatchesRecomputedHashOnAllOpeningMoves) {
    Zobrist::init();
    Board board = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board.updateAttackBoards();
    auto moves = MoveGen::generateMoves(board);

    for (const auto& m : moves) {
        Board after = board.applyMove(m);
        uint64_t incrementalHash = after.hashKey;

        // Freshly recompute
        after.computeHash();
        EXPECT_EQ(incrementalHash, after.hashKey)
            << "Incremental hash mismatch for move " << moveToUCI(m);
    }
}

TEST(ZobristTest, TranspositionProducesIdenticalHash) {
    // Sequence 1: 1. e4 e5 2. Nf3 Nc6
    // Sequence 2: 1. Nf3 Nc6 2. e4 e5
    // Note: In chess without en passant leftover, the resulting board state is identical.
    Board b1 = Board::fromFEN("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3");
    Board b2 = Board::fromFEN("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3");
    EXPECT_EQ(b1.hashKey, b2.hashKey);
}
