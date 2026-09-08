#include "board/board.h"
#include "move/move.h"
#include "move/movegen.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

uint64_t perft(const Board& board, int depth) {
    if (depth == 0)
        return 1ULL;

    Board b = board;
    b.updateAttackBoards();
    std::vector<Move> moves = MoveGen::generateMoves(b);

    if (depth == 1)
        return moves.size();

    uint64_t nodes = 0;
    for (const auto& m : moves) {
        Board next = b.applyMove(m);
        nodes += perft(next, depth - 1);
    }
    return nodes;
}

} // namespace

TEST(PerftTest, InitialPositionDepth1) {
    Board board = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(perft(board, 1), 20ULL);
}

TEST(PerftTest, InitialPositionDepth2) {
    Board board = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(perft(board, 2), 400ULL);
}

TEST(PerftTest, InitialPositionDepth3) {
    Board board = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(perft(board, 3), 8902ULL);
}

TEST(PerftTest, KiwipetePositionDepth1And2) {
    // Kiwipete position by Peter McKenzie
    Board board =
        Board::fromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    EXPECT_EQ(perft(board, 1), 48ULL);
    EXPECT_EQ(perft(board, 2), 2039ULL);
}

TEST(PerftTest, Position3Depth1And2) {
    Board board = Board::fromFEN("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    EXPECT_EQ(perft(board, 1), 14ULL);
    EXPECT_EQ(perft(board, 2), 191ULL);
}

TEST(PerftTest, Position4Depth1And2) {
    Board board =
        Board::fromFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    EXPECT_EQ(perft(board, 1), 6ULL);
    EXPECT_EQ(perft(board, 2), 264ULL);
}

TEST(PerftTest, Position5Depth1And2) {
    Board board = Board::fromFEN("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    EXPECT_EQ(perft(board, 1), 44ULL);
    EXPECT_EQ(perft(board, 2), 1486ULL);
}
