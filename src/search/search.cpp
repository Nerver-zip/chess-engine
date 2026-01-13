#include "search.h"
#include "../eval/eval.h"
#include "../debuglib/debug.h"
#include "../tt/tt.h"
#include <algorithm>
#include <cstring>

/**
 * @brief Inicia a busca pelo melhor lance na raiz.
 * * Atualmente usa uma busca de profundidade fixa (Fixed Depth).
 * * TODO Futuro: Implementar Iterative Deepening (ID).
 * Em vez de chamar searchBestMove(depth 5) direto, chamar para 1, depois 2, até 5.
 * Isso ajuda muito no ordenamento de movimentos e gerenciamento de tempo.
 */
Move Search::searchBestMove(const Board& board, int maxDepth) {
    // ============= DEBUG ================
    Search::stats.nodes  = 0;
    Search::stats.qnodes = 0;
    Search::stats.evaluations = 0;

    Debug::RAII_Timer raii_timer("Search");
    Debug::Stopwatch stopwatch;

    // ====================================
    std::memset(killerMoves, 0, sizeof(killerMoves));
    std::memset(history, 0, sizeof(history));
    TT.newSearch();
     
    Move globalBestMove = {};
    int globalBestScore = -INF;

    // === ITERATIVE DEEPENING ===
    // Vai de 1 até a profundidade máxima pedida
    for (int currentDepth = 1; currentDepth <= maxDepth; ++currentDepth) {
        
        // Janela de Aspiração (Resetamos Alpha/Beta a cada nova profundidade)
        int alpha = -INF;
        int beta = INF;

        // Geramos os movimentos novamente a cada iteração
        // Isso é necessário porque a ordenação muda conforme a TT é preenchida
        std::vector<Move> moves = MoveGen::generateMoves(board);
        if (moves.empty()) return {};

        // Descobre o Hash Move dessa iteração (vindo da iteração anterior)
        Move ttMove = {};
        TTEntry entry;
        if (TT.probe(board.hashKey, entry, 0)) {
            ttMove = unpackMove(entry.move);
        }

        // Aplica bônus no Hash Move
        for (auto& m : moves) {
            if (m == ttMove) {
                m.score = 30000; // Prioridade máxima
                break;
            }
        }

        // Ordena (Agora o melhor lance da depth anterior será o primeiro)
        std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b){
            return a.score > b.score;
        });

        // Root Search (Busca na Raiz)
        Move iterationBestMove = {};
        int iterationBestScore = -INF;
        
        for (const auto& move : moves) {
            Board nextBoard = board.applyMove(move);
            nextBoard.updateAttackBoards();

            int score = -negamax(nextBoard, currentDepth - 1, -beta, -alpha, 1);

            if (score > iterationBestScore) {
                iterationBestScore = score;
                iterationBestMove = move;
            }

            if (score > alpha) {
                alpha = score;
                // Encontramos um novo melhor lance (PV), gravamos na TT imediatamente
                // para que, se pararmos o tempo agora, a informação esteja salva.
                // Quando implementar limite de tempo, parar depois desse passo
                TT.store(board.hashKey, currentDepth, score, TT_EXACT, move, 0);
            }
        }

        // Atualiza o melhor lance global
        globalBestMove = iterationBestMove;
        globalBestScore = iterationBestScore;

        // Stats da iteração 
        // Aqui printa o "pensamento" da engine
        uint64_t timeMs = stopwatch.elapsed_us();
        if (timeMs == 0) timeMs = 1; 
        
        uint64_t totalNodes = stats.nodes + stats.qnodes;
        uint64_t nps = (totalNodes * 1000) / timeMs;

        Debug::cout << "info depth " << currentDepth 
                    << " score " << globalBestScore 
                    << " nodes " << totalNodes 
                    << " nps " << nps 
                    << " pv " << (int)globalBestMove.from << "->" << (int)globalBestMove.to << "\n";
    }
    
    uint64_t us = stopwatch.elapsed_us();
    uint64_t total_nodes = stats.nodes + stats.qnodes;
    
    uint64_t nps = (total_nodes * 1000000) / us;

    Debug::cout << "\n=== Search Statistics ===\n";
    Debug::cout << "Depth:       " << maxDepth << "\n";
    Debug::cout << "Time:        " << (us / 1000.0) << " ms\n";
    Debug::cout << "Nodes:       " << stats.nodes << " (Interior)\n";
    Debug::cout << "QNodes:      " << stats.qnodes << " (Quiescence)\n";
    Debug::cout << "Total Nodes: " << total_nodes << "\n";
    Debug::cout << "Evaluations: " << stats.evaluations << "\n";
    Debug::cout << "NPS:         " << nps << " nodes/sec\n";
    Debug::cout << "Evaluation:  " << globalBestScore << "\n";
    Debug::cout << "TT Permill:  " << TT.hashfull() << "\n";
    Debug::cout << "=========================\n";

    return globalBestMove;
}

/**
 * @brief Algoritmo Negamax com poda Alpha-Beta.
 * * @param ply Distância da raiz (root). Usado para calcular "Mate em X".
 * Se encontrarmos um mate com ply 3 e outro com ply 5, o score do ply 3 será maior,
 * fazendo a engine preferir o mate mais rápido.
 */
int Search::negamax(const Board& board, int depth, int alpha, int beta, int ply) {
    ++Search::stats.nodes;
    int alphaOrig = alpha;
    
    bool inCheck = board.whiteToMove ? (board.whiteKing & board.blackAttacks) 
                                     : (board.blackKing & board.whiteAttacks);
    // Se em xeque, estende a busca
    if (inCheck) {
        depth++;
    }

    if (depth == 0) {
        ++Search::stats.evaluations;
        return quiescence(board, alpha, beta);
    }
    
    // Transposition Table Probe
    TTEntry ttEntry;
    Move ttMove = {};
    
    if (TT.probe(board.hashKey, ttEntry, ply)) {
        ttMove = unpackMove(ttEntry.move);

        // TT Cutoff (Só se depth for suficiente)
        if (ttEntry.depth >= depth) {
            if (ttEntry.flag == TT_EXACT) {
                return ttEntry.score;
            }
            if (ttEntry.flag == TT_ALPHA && ttEntry.score <= alpha){
                return ttEntry.score;
            }             
            if (ttEntry.flag == TT_BETA  && ttEntry.score >= beta){
                return ttEntry.score;
            } 
        }
    }

    std::vector<Move> moves = MoveGen::generateMoves(board);

    if (moves.empty()) {
        // Se não há lances legais, ou é Mate ou é Afogamento (Stalemate).
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
            // Bonus grande se o movimento veio da TT
            if(m == ttMove) {
                m.score = 30000;
                continue;
            }
            
            // Ignorar capturas, deixar para MVV-LVA
            if (m.flags & CAPTURE) continue;
            
            // Aplicar bônus se for killer move (quiet move que fez beta cutoff)
            else if (m.from == killerMoves[ply][0].from && m.to == killerMoves[ply][0].to) {
                m.score = KILLER_1_SCORE;
            }
            else if (m.from == killerMoves[ply][1].from && m.to == killerMoves[ply][1].to) {
                m.score = KILLER_2_SCORE;
            }
            else {
                // Se não for captura nem killer, recebe a pontuação do histórico
                int side = board.whiteToMove ? 0 : 1;
                m.score = history[side][m.from][m.to];
                if (m.score > 7000) 
                    m.score = 7000;
            }
        }
    }

    std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b){
        return a.score > b.score;
    });
        
    //if (ply > 0 && ply <= 2) { 
    //    Debug::printMoveList(moves, "Sorted Moves (Ply " + std::to_string(ply) + ")");
    //}

    // =============================================================
    // Recursão e Poda Alpha-Beta
    // =============================================================
    int bestVal = -INF;
    Move bestMove = {};
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
            bestMove = move;
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
                
                //Atualizar history heuristic
                int side = board.whiteToMove ? 0 : 1;
                
                // Lances que cortam perto da raiz (depth alto) ganham muitos pontos.
                // Lances nas folhas (depth 1) ganham pouco (1 ponto).
                int bonus = depth * depth;
                history[side][move.from][move.to] += bonus;
                
                // Capar por segurança e para não ultrapassar killers
                if (history[side][move.from][move.to] > MAX_HISTORY) {
                    history[side][move.from][move.to] = MAX_HISTORY;
                }
            }
            
            break; 
        }
    }

    TTFlag flag = TT_EXACT;
    // Não conseguimos melhorar o alpha original (Fail Low)
    if (bestVal <= alphaOrig) flag = TT_ALPHA;
    // Causamos um corte (Fail High)
    else if (bestVal >= beta) flag = TT_BETA;
    
    // Se não entrou nos ifs acima, é TT_EXACT (bestVal entre alphaOrig e beta)
    // Grava na tabela
    TT.store(board.hashKey, depth, bestVal, flag, bestMove, ply);

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
