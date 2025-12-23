#pragma once

#include <cstdint>

enum Piece : uint8_t {
    EMPTY = 0,

    WPAWN, WKNIGHT, WBISHOP, WROOK, WQUEEN, WKING,
    BPAWN, BKNIGHT, BBISHOP, BROOK, BQUEEN, BKING
};
