#include "board/board.h"
#include "move/move.h"
#include "search/search.h"
#include "tt/tt.h"
#include <gtest/gtest.h>

TEST(SearchTest, FindsBackRankMateInOne) {
    // White rook on a1 (0), White king on e1 (4). Black king on h8 (63), pawns on g7/h7.
    // Winning move is 1. Ra8# (a1a8: 0 -> 56)
    Board board = Board::fromFEN("7k/6pp/8/8/8/8/8/R3K3 w Q - 0 1");
    board.updateAttackBoards();

    Move best = Search::searchBestMove(board, 2);
    EXPECT_EQ(best.from, 0); // a1
    EXPECT_EQ(best.to, 56);  // a8
    EXPECT_EQ(moveToUCI(best), "a1a8");
}

TEST(SearchTest, FindsLegalMoveFromStartingPosition) {
    Board board = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board.updateAttackBoards();

    Move best = Search::searchBestMove(board, 2);
    EXPECT_NE(best.from, best.to);

    // Verify best move is in legal moves
    auto moves = MoveGen::generateMoves(board);
    bool found = false;
    for (const auto& m : moves) {
        if (m.from == best.from && m.to == best.to) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(TranspositionTableTest, StoreAndProbeEntry) {
    TT.resize(16); // 16 MB
    TT.clear();

    uint64_t key = 0x123456789ABCDEULL;
    Move move;
    move.from = 12;
    move.to = 28;
    move.flags = DOUBLE_PAWN_PUSH;
    move.promotion = EMPTY;

    TT.store(key, 4, 150, TT_EXACT, move, 0);

    TTEntry entry;
    bool hit = TT.probe(key, entry, 0);
    EXPECT_TRUE(hit);
    EXPECT_EQ(entry.score, 150);
    EXPECT_EQ(entry.depth, 4);
    EXPECT_EQ(entry.flag, TT_EXACT);

    Move unpacked = unpackMove(entry.move);
    EXPECT_EQ(unpacked.from, 12);
    EXPECT_EQ(unpacked.to, 28);
}
