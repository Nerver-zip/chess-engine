#pragma once
#include <string>
#include "../board/board.h"

#ifdef DEBUG

namespace Debug {

    /**
     * @brief Imprime um único bitboard (0s e 1s) formatado como tabuleiro 8x8.
     * Útil para ver máscaras de ataque, pathfinding, etc.
     */
    void printBitboard(uint64_t bb, const std::string& title = "Bitboard");

    /**
     * @brief Imprime o estado completo do jogo (Peças, Lado a jogar, Roque, EnPassant).
     * Usa caracteres ASCII (P, n, k...) e cores ANSI se o terminal suportar.
     */
    void printBoard(const Board& board);

    /**
     * @brief Atalho para imprimir os dois mapas de ataque (White Attacks vs Black Attacks).
     */
    void printAttackMaps(const Board& board);

    void printMove(const Move& m);  
}

#else

namespace Debug {
    inline void printBitboard(uint64_t, const std::string& = "") {}
    inline void printBoard(const Board&) {}
    inline void printAttackMaps(const Board&) {}
    inline void printMove(const Move&) {}
}

#endif 
