#include "search.h"
#include "../eval/eval.h"
#include "../debuglib/debug.h"
#include <algorithm>
#include <cstring>
#include <iostream>

/**
 * @brief Inicia a busca pelo melhor lance na raiz.
 * * Atualmente usa uma busca de profundidade fixa (Fixed Depth).
 * * TODO Futuro: Implementar Iterative Deepening (ID).
 * Em vez de chamar searchBestMove(depth 5) direto, chamar para 1, depois 2, até 5.
 * Isso ajuda muito no ordenamento de movimentos e gerenciamento de tempo.
 */
Move Search::searchBestMove(const Board& board, int depth) {
    // ============= DEBUG ================
    Search::stats.nodes  = 0;
    Search::stats.qnodes = 0;
    Search::stats.evaluations = 0;

    Debug::RAII_Timer raii_timer("Search");
    Debug::Stopwatch stopwatch;

    // ====================================
    std::memset(killerMoves, 0, sizeof(killerMoves));
   
    // Gera lances legais da raiz
    std::vector<Move> moves = MoveGen::generateMoves(board);
    
    // Se não há lances legais, o jogo acabou.
    if (moves.empty()) return {}; 
    
    // Move Ordering MVV-LVA
    std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b){
        return a.score > b.score;
    });
    
    Move bestMove;
    int bestScore = -INF;
    
    int alpha = -INF;
    int beta = INF;

    // Itera sobre todos os movimentos possíveis da raiz
    for (const auto& move : moves) {
        Board nextBoard = board.applyMove(move);
        
        // O MoveGen da próxima recursão precisa disso para saber se o rei está em xeque
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
    
    uint64_t us = stopwatch.elapsed_us();
    uint64_t total_nodes = stats.nodes + stats.qnodes;
    
    uint64_t nps = (total_nodes * 1000000) / us;

    Debug::cout << "\n=== Search Statistics ===\n";
    Debug::cout << "Depth:       " << depth << "\n";
    Debug::cout << "Time:        " << (us / 1000.0) << " ms\n";
    Debug::cout << "Nodes:       " << stats.nodes << " (Interior)\n";
    Debug::cout << "QNodes:      " << stats.qnodes << " (Quiescence)\n";
    Debug::cout << "Total Nodes: " << total_nodes << "\n";
    Debug::cout << "Evaluations: " << stats.evaluations << "\n";
    Debug::cout << "NPS:         " << nps << " nodes/sec\n";
    Debug::cout << "=========================\n";

    return bestMove;
}

/**
 * @brief Algoritmo Negamax com poda Alpha-Beta.
 * * @param ply Distância da raiz (root). Usado para calcular "Mate em X".
 * Se encontrarmos um mate com ply 3 e outro com ply 5, o score do ply 3 será maior,
 * fazendo a engine preferir o mate mais rápido.
 */
int Search::negamax(const Board& board, int depth, int alpha, int beta, int ply) {
    ++Search::stats.nodes;
    if (depth == 0) {
        ++Search::stats.evaluations;
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

    // Atualmente atualmente usando MVL-LVA + Killer Moves
    
    // =============================================================
    // APLICANDO BÔNUS PARA KILLER MOVES
    // =============================================================
    // Antes de ordenar, verificamos se algum lance gerado é um Killer Move desse ply.
    // Se for, damos um score alto para ele ser testado logo após as capturas.
    
    if (ply < MAX_PLY) {
        for (auto& m : moves) {
            if (m.flags & CAPTURE) continue;
            
            // Aplicar bônus se for killer move (quiet move que fez beta cutoff)
            if (m.from == killerMoves[ply][0].from && m.to == killerMoves[ply][0].to) {
                m.score = KILLER_1_SCORE;
            }
            else if (m.from == killerMoves[ply][1].from && m.to == killerMoves[ply][1].to) {
                m.score = KILLER_2_SCORE;
            }
        }
    }

    std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b){
        return a.score > b.score;
    });

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
            
            // Salvar killer move 
            if (!(move.flags & CAPTURE) && ply < MAX_PLY) {
                bool isFirst = (move.from == killerMoves[ply][0].from && move.to == killerMoves[ply][0].to);
                
                if (!isFirst) {
                    // Empurra o Killer 1 para a posição 2
                    killerMoves[ply][1] = killerMoves[ply][0];
                    // Salva o novo lance como Killer 1 (Prioridade máxima)
                    killerMoves[ply][0] = move;
                }
            }
            
            break; 
        }
    }

    return bestVal;
}

int Search::quiescence(const Board& board, int alpha, int beta) {
    ++Search::stats.qnodes;
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

    std::vector<Move> moves = MoveGen::generateWinningMoves(board);
    
    // Ordenação MVV-LVA
    std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b){
        return a.score > b.score;
    });

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
