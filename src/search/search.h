#pragma once
#include "../board/board.h"
#include "../move/movegen.h"
#include "../move/move.h"

// Valores para infinito e Mate. 
// Mate não é infinito real para podermos calcular "Mate em X lances"
constexpr int INF = 1000000;
constexpr int MATE_SCORE = 100000; 

class Search {
public:
    /**
     * @brief Inicia a busca pelo melhor movimento a partir do estado atual.
     * @param board O tabuleiro raiz.
     * @param depth Profundidade da busca (ex: 4, 5, 6 plies).
     * @return O melhor lance encontrado.
     */
    static Move searchBestMove(const Board& board, int depth);

private:
    /**
     * @brief O algoritmo Negamax com Alpha-Beta Pruning.
     * * @param board Estado atual.
     * @param depth Profundidade restante.
     * @param alpha O melhor score que o lado atual já garantiu (limite inferior).
     * @param beta O melhor score que o oponente já garantiu (limite superior).
     * @param ply Distância da raiz (usado para preferir mates mais rápidos).
     * @return A pontuação da posição (do ponto de vista de quem joga).
     */
    static int negamax(const Board& board, int depth, int alpha, int beta, int ply);
};
