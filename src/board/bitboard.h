#pragma once
#include <cstdint>

/**
 * @brief Constrói um bitboard com apenas um bit ativo na casa sq.
 * @param sq Casa (0..63)
 * @return Bitboard unitário
 */
constexpr inline uint64_t BB(int sq) {
    return 1ULL << sq;
}

/**
 * @brief Retorna o índice (0..63) de um bitboard com apenas um bit ativo.
 */
constexpr inline int sqFromBB(uint64_t b) {
    return __builtin_ctzll(b);
}

/**
 * @brief Verifica se um par rank/file está dentro do tabuleiro.
 */
constexpr inline bool onBoard(int r, int f) {
    return r >= 0 && r < 8 && f >= 0 && f < 8;
}
