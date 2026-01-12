#pragma once
#include <cstdint>
#include "../board/piece.h"

namespace Zobrist {
    // Usando enum definido em piece.h
    extern uint64_t pieces[13][64];

    // [Direitos de Roque: 0 a 15]
    // Bits: 1=WK, 2=WQ, 4=BK, 8=BQ
    extern uint64_t castling[16];

    // En Passant: 0-7 (A-H), 8 = Nenhum]
    extern uint64_t enPassant[9];

    // Lado a jogar (fazemos XOR com isso na vez das pretas)
    extern uint64_t sideToMove;

    // Inicializa todos os arrays com números aleatórios
    void init();
}
