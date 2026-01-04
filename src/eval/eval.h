#pragma once
#include "../board/board.h"

class Eval {
public:
    /**
     * @brief Avalia a posição estática do tabuleiro.
     * @param board Estado atual.
     * @return Score em centipawns (100 = 1 peão). 
     * Positivo significa vantagem para o lado que tem a vez (White ou Black).
     */
    static int evaluate(const Board& board);
};
