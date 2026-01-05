#include "search.h"
#include "../eval/eval.h"
#include <algorithm>
#include <iostream>

/**
 * @brief Inicia a busca pelo melhor lance na raiz.
 * * Atualmente usa uma busca de profundidade fixa (Fixed Depth).
 * * TODO Futuro: Implementar Iterative Deepening (ID).
 * Em vez de chamar searchBestMove(depth 5) direto, chamar para 1, depois 2, até 5.
 * Isso ajuda muito no ordenamento de movimentos e gerenciamento de tempo.
 */
Move Search::searchBestMove(const Board& board, int depth) {
    // 1. Gera lances legais da raiz
    std::vector<Move> moves = MoveGen::generateMoves(board);
    
    // Se não há lances legais, o jogo acabou.
    if (moves.empty()) return {}; 

    // TODO: Move Ordering na Raiz
    // É crucial ordenar os lances aqui também para garantir que o melhor lance
    // seja analisado cedo, melhorando o Alpha-Beta.
    
    Move bestMove;
    int bestScore = -INF;
    
    int alpha = -INF;
    int beta = INF;

    // Itera sobre todos os movimentos possíveis da raiz
    for (const auto& move : moves) {
        Board nextBoard = board.applyMove(move);
        
        // CRÍTICO: Atualiza os mapas de ataque.
        // O MoveGen da PRÓXIMA recursão precisa disso para saber se o rei está em xeque
        // e para filtrar lances ilegais do oponente.
        nextBoard.updateAttackBoards();

        // Chama o Negamax recursivo
        // ply = 1 (estamos a 1 passo da raiz)
        // Invertemos o sinal e trocamos alpha/beta (conceito Negamax)
        int score = -negamax(nextBoard, depth - 1, -beta, -alpha, 1);

        // Debug: Mostra pontuação de cada lance principal
        // std::cout << "Info: Move " << (int)move.from << "->" << (int)move.to 
        //           << " Score: " << score << "\n";

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }

        // Atualiza Alpha (limite inferior da nossa pontuação)
        if (score > alpha) {
            alpha = score;
        }
    }

    return bestMove;
}

/**
 * @brief Algoritmo Negamax com poda Alpha-Beta.
 * * @param ply Distância da raiz (root). Usado para calcular "Mate em X".
 * Se encontrarmos um mate com ply 3 e outro com ply 5, o score do ply 3 será maior,
 * fazendo a engine preferir o mate mais rápido.
 */
int Search::negamax(const Board& board, int depth, int alpha, int beta, int ply) {
    
    if (depth == 0) {
        return quiescence(board, alpha, beta);
    }

    std::vector<Move> moves = MoveGen::generateMoves(board);

    if (moves.empty()) {
        // Se não há lances legais, ou é Mate ou é Afogamento (Stalemate).
        bool inCheck = board.whiteToMove 
                       ? (board.whiteKing & board.blackAttacks)
                       : (board.blackKing & board.whiteAttacks);
        
        if (inCheck) {
            // Xeque-mate!
            // Retornamos -MATE + ply. 
            // Quanto menor o ply (mais perto da raiz), maior o score (menos negativo).
            // Isso incentiva a engine a dar mate rápido e adiar levar mate.
            return -MATE_SCORE + ply; 
        } else {
            // Afogamento (Empate)
            return 0;
        }
    }

    // =============================================================
    // Move Ordering (Ordenação de Movimentos)
    // =============================================================
    // TODO: Implementar heurísticas de ordenação.
    // O Alpha-Beta funciona muito melhor se analisarmos os melhores lances primeiro.
    //  1. Hash Move (da Transposition Table - Futuro)
    //  2. Capturas (MVV-LVA: Most Valuable Victim - Least Valuable Attacker)
    //  3. Killer Moves (lances que causaram beta cutoff em irmãos)
    //  4. History Heuristic
    //  5. Demais lances

    // =============================================================
    // Recursão e Poda Alpha-Beta
    // =============================================================
    int bestVal = -INF;

    for (const auto& move : moves) {
        Board nextBoard = board.applyMove(move);
        nextBoard.updateAttackBoards(); // Prepara para o próximo nível

        // Recursão Negamax:
        // - diminuímos profundidade (depth - 1)
        // - aumentamos a distância da raiz (ply + 1)
        // - invertemos a janela alpha-beta: alpha vira -beta, beta vira -alpha
        // - invertemos o sinal do resultado (-)
        int score = -negamax(nextBoard, depth - 1, -beta, -alpha, ply + 1);

        if (score > bestVal) {
            bestVal = score;
        }

        // Atualiza o limite inferior (Alpha)
        alpha = std::max(alpha, bestVal);
        
        // Poda Beta (Beta Cutoff)
        // Se o score for maior que beta, o oponente evitará essa variante.
        // Podemos parar de buscar (Fail High).
        if (alpha >= beta) {
            // TODO: Salvar este lance como "Killer Move" para ordenação futura
            break; 
        }
    }

    return bestVal;
}

int Search::quiescence(const Board& board, int alpha, int beta) {
    // Avaliamos a posição atual. Se já for boa o suficiente (>= beta),
    // assumimos que não precisamos capturar nada e cortamos (Beta Cutoff).
    // Isso evita que sejamos forçados a fazer capturas ruins.
    int stand_pat = Eval::evaluate(board);

    if (stand_pat >= beta) {
        return beta;
    }

    // Se a avaliação estática melhora nosso alpha, atualizamos.
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    std::vector<Move> moves = MoveGen::generateForcingMoves(board);
    
    //-------------------------------------------------------------------
    // TODO: Ordenação MVV-LVA aqui é CRÍTICA para performance do QSearch
    // (Capturar peça valiosa com peça barata primeiro)
    // ------------------------------------------------------------------

    for (const auto& move : moves) {

        Board nextBoard = board.applyMove(move);
        nextBoard.updateAttackBoards();

        int score = -quiescence(nextBoard, -beta, -alpha);

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}
