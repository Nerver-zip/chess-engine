#pragma once
#include <cstdint>

enum MoveFlags : uint8_t {
    QUIET      = 0,
    CAPTURE    = 1 << 0,
    DOUBLE_PAWN_PUSH = 1 << 1,
    KING_CASTLE = 1 << 2,
    QUEEN_CASTLE = 1 << 3,
    EN_PASSANT = 1 << 4,
    PROMOTION  = 1 << 5
};

struct Move {
    uint8_t from;        // 0..63
    uint8_t to;          // 0..63
    uint8_t promotion;  // Piece (só usado se PROMOTION)
    uint8_t flags;      // MoveFlags
};
