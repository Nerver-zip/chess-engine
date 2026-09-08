#include "board/board.h"
#include "move/move.h"
#include "move/movegen.h"
#include <algorithm>
#include <gtest/gtest.h>

TEST(MoveGenTest, StartingPositionHasTwentyLegalMoves) {
    Board board = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board.updateAttackBoards();
    auto moves = MoveGen::generateMoves(board);
    EXPECT_EQ(moves.size(), 20U);
}

TEST(MoveGenTest, GeneratesKingsideAndQueensideCastlingWhenLegal) {
    Board board = Board::fromFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    board.updateAttackBoards();
    auto moves = MoveGen::generateMoves(board);

    bool hasKingsideCastle = false;
    bool hasQueensideCastle = false;

    for (const auto& m : moves) {
        if (m.from == 4 && m.to == 6 && (m.flags & KING_CASTLE))
            hasKingsideCastle = true;
        if (m.from == 4 && m.to == 2 && (m.flags & QUEEN_CASTLE))
            hasQueensideCastle = true;
    }

    EXPECT_TRUE(hasKingsideCastle);
    EXPECT_TRUE(hasQueensideCastle);
}

TEST(MoveGenTest, CannotCastleWhenInCheck) {
    // White king on e1 is checked by black rook on e8
    Board board = Board::fromFEN("r3k2r/8/8/8/8/8/4r3/R3K2R w KQkq - 0 1");
    board.updateAttackBoards();
    auto moves = MoveGen::generateMoves(board);

    for (const auto& m : moves) {
        EXPECT_FALSE(m.flags & KING_CASTLE);
        EXPECT_FALSE(m.flags & QUEEN_CASTLE);
    }
}

TEST(MoveGenTest, CannotCastleThroughAttackedSquare) {
    // White wants to castle kingside (e1->g1), but f1 is attacked by black rook on f8
    Board board = Board::fromFEN("r3kr2/8/8/8/8/8/8/R3K2R w KQ - 0 1");
    board.updateAttackBoards();
    auto moves = MoveGen::generateMoves(board);

    bool hasKingsideCastle = false;
    for (const auto& m : moves) {
        if (m.from == 4 && m.to == 6 && (m.flags & KING_CASTLE))
            hasKingsideCastle = true;
    }
    EXPECT_FALSE(hasKingsideCastle);
}

TEST(MoveGenTest, CastlingRightsLostWhenKingMoves) {
    Board board = Board::fromFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    // Move King e1 -> e2
    Move kingMove;
    kingMove.from = 4; // e1
    kingMove.to = 12;  // e2
    kingMove.flags = QUIET;
    kingMove.promotion = EMPTY;

    Board after = board.applyMove(kingMove);
    // White lost both WK (1) and WQ (2), black keeps bk (4) and bq (8) = 12
    EXPECT_EQ(after.castlingRights, 12);
}

TEST(MoveGenTest, CastlingRightsLostWhenRookMoves) {
    Board board = Board::fromFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    // Move Rook h1 -> h2
    Move rookMove;
    rookMove.from = 7; // h1
    rookMove.to = 15;  // h2
    rookMove.flags = QUIET;
    rookMove.promotion = EMPTY;

    Board after = board.applyMove(rookMove);
    // White lost WK (1), keeps WQ (2), black keeps bk (4) and bq (8) -> 2 | 12 = 14
    EXPECT_EQ(after.castlingRights, 14);
}

TEST(MoveGenTest, GeneratesAndExecutesEnPassantCapture) {
    // White pawn on e5 (36), Black pawn just pushed f7 -> f5, enPassantSquare is f6 (45)
    Board board = Board::fromFEN("8/8/8/4Pp2/8/8/8/4K2k w - f6 0 1");
    board.updateAttackBoards();
    auto moves = MoveGen::generateMoves(board);

    auto epMove = std::find_if(moves.begin(), moves.end(), [](const Move& m) {
        return m.from == 36 && m.to == 45 && (m.flags & EN_PASSANT);
    });

    ASSERT_NE(epMove, moves.end());
    EXPECT_TRUE(epMove->flags & CAPTURE);

    // Apply en passant move
    Board after = board.applyMove(*epMove);
    // Black pawn on f5 (37) should be captured and removed
    EXPECT_EQ(after.blackPawns & (1ULL << 37), 0ULL);
    // White pawn is now on f6 (45)
    EXPECT_NE(after.whitePawns & (1ULL << 45), 0ULL);
    // e5 is empty
    EXPECT_EQ(after.pieceAt(36), EMPTY);
}

TEST(MoveGenTest, GeneratesAllFourPromotions) {
    // White pawn on a7 (48), empty board
    Board board = Board::fromFEN("8/P7/8/8/8/8/8/4K2k w - - 0 1");
    board.updateAttackBoards();
    auto moves = MoveGen::generateMoves(board);

    int promotionCount = 0;
    bool hasQueen = false, hasRook = false, hasBishop = false, hasKnight = false;

    for (const auto& m : moves) {
        if (m.from == 48 && m.to == 56 && (m.flags & PROMOTION)) {
            promotionCount++;
            if (m.promotion == WQUEEN)
                hasQueen = true;
            if (m.promotion == WROOK)
                hasRook = true;
            if (m.promotion == WBISHOP)
                hasBishop = true;
            if (m.promotion == WKNIGHT)
                hasKnight = true;
        }
    }

    EXPECT_EQ(promotionCount, 4);
    EXPECT_TRUE(hasQueen);
    EXPECT_TRUE(hasRook);
    EXPECT_TRUE(hasBishop);
    EXPECT_TRUE(hasKnight);
}

TEST(MoveGenTest, DoubleCheckAllowsOnlyKingMoves) {
    // White king on e1 is in double check from black rook on e8 and bishop on a5
    Board board = Board::fromFEN("4r3/8/8/b7/8/8/8/4K2k w - - 0 1");
    board.updateAttackBoards();
    auto moves = MoveGen::generateMoves(board);

    ASSERT_GT(moves.size(), 0U);
    for (const auto& m : moves) {
        EXPECT_EQ(m.from, 4); // Only king (e1) moves can be legal
    }
}
