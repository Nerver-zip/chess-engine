#include "eval.h"

inline int gamePhase(const Board& b) {
    int phase = 0;
    phase += __builtin_popcountll(b.whiteQueens  | b.blackQueens ) * 4;
    phase += __builtin_popcountll(b.whiteRooks   | b.blackRooks  ) * 2;
    phase += __builtin_popcountll(b.whiteBishops | b.blackBishops) * 1;
    phase += __builtin_popcountll(b.whiteKnights | b.blackKnights) * 1;
    if (phase > 24) phase = 24;
    return phase;   // 24 = MG puro, 0 = EG puro
}

inline int pstScore(uint64_t bb, const int* mg, const int* eg,
                    int mgW, int egW, bool isWhite)
{
    int sMG = 0, sEG = 0;
    while (bb) {
        int sq = __builtin_ctzll(bb);
        bb &= bb - 1;
        // Espelha verticalmente para as pretas (rank 0 vira rank 7)
        int idx = isWhite ? sq : sq ^ 56;
        sMG += mg[idx];
        sEG += eg[idx];
    }
    return sMG * mgW + sEG * egW;
}

int Eval::evaluate(const Board& board) {

    int mgPhase = gamePhase(board);
    int egPhase = 24 - mgPhase;

    int scoreMat = 0;

    scoreMat += __builtin_popcountll(board.whitePawns)   * P_VAL;
    scoreMat += __builtin_popcountll(board.whiteKnights) * N_VAL;
    scoreMat += __builtin_popcountll(board.whiteBishops) * B_VAL;
    scoreMat += __builtin_popcountll(board.whiteRooks)   * R_VAL;
    scoreMat += __builtin_popcountll(board.whiteQueens)  * Q_VAL;

    scoreMat -= __builtin_popcountll(board.blackPawns)   * P_VAL;
    scoreMat -= __builtin_popcountll(board.blackKnights) * N_VAL;
    scoreMat -= __builtin_popcountll(board.blackBishops) * B_VAL;
    scoreMat -= __builtin_popcountll(board.blackRooks)   * R_VAL;
    scoreMat -= __builtin_popcountll(board.blackQueens)  * Q_VAL;

    // 2. PST Interpolado
    // Este valor virá escalado por 24
    int scorePST = 0;

    scorePST += pstScore(board.whitePawns, PST_P_MG, PST_P_EG, mgPhase, egPhase, true);
    scorePST -= pstScore(board.blackPawns, PST_P_MG, PST_P_EG, mgPhase, egPhase, false);
    
    scorePST += pstScore(board.whiteKnights, PST_N_MG, PST_N_EG, mgPhase, egPhase, true);
    scorePST -= pstScore(board.blackKnights, PST_N_MG, PST_N_EG, mgPhase, egPhase, false);
    
    scorePST += pstScore(board.whiteBishops, PST_B_MG, PST_B_EG, mgPhase, egPhase, true);
    scorePST -= pstScore(board.blackBishops, PST_B_MG, PST_B_EG, mgPhase, egPhase, false);
    
    scorePST += pstScore(board.whiteRooks, PST_R_MG, PST_R_EG, mgPhase, egPhase, true);
    scorePST -= pstScore(board.blackRooks, PST_R_MG, PST_R_EG, mgPhase, egPhase, false);
    
    scorePST += pstScore(board.whiteQueens, PST_Q_MG, PST_Q_EG, mgPhase, egPhase, true);
    scorePST -= pstScore(board.blackQueens, PST_Q_MG, PST_Q_EG, mgPhase, egPhase, false);
    
    scorePST += pstScore(board.whiteKing, PST_K_MG, PST_K_EG, mgPhase, egPhase, true);
    scorePST -= pstScore(board.blackKing, PST_K_MG, PST_K_EG, mgPhase, egPhase, false);
    
    // Normalização
    int finalScore = scoreMat + (scorePST / 24);

    // Retorna do ponto de vista do lado a jogar
    return board.whiteToMove ? finalScore : -finalScore;
}
