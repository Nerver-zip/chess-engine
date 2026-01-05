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
    int score;          // Pontuação MVV-LVA
};

static constexpr int MVV_LVA_VALUES[13] = {
    0,      // EMPTY
    100,    // WPAWN
    320,    // WKNIGHT
    330,    // WBISHOP
    500,    // WROOK
    900,    // WQUEEN
    20000,  // WKING
    100,    // BPAWN
    320,    // BKNIGHT
    330,    // BBISHOP
    500,    // BROOK
    900,    // BQUEEN
    20000   // BKING
};
