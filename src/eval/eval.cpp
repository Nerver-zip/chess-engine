#include "eval.h"

int Eval::evaluate(const Board& board) {
    // Por hora, simples contagem de material

    int whiteMat = 0;
    int blackMat = 0;

    whiteMat += __builtin_popcountll(board.whitePawns)   * P_VAL;
    whiteMat += __builtin_popcountll(board.whiteKnights) * N_VAL;
    whiteMat += __builtin_popcountll(board.whiteBishops) * B_VAL;
    whiteMat += __builtin_popcountll(board.whiteRooks)   * R_VAL;
    whiteMat += __builtin_popcountll(board.whiteQueens)  * Q_VAL;

    blackMat += __builtin_popcountll(board.blackPawns)   * P_VAL;
    blackMat += __builtin_popcountll(board.blackKnights) * N_VAL;
    blackMat += __builtin_popcountll(board.blackBishops) * B_VAL;
    blackMat += __builtin_popcountll(board.blackRooks)   * R_VAL;
    blackMat += __builtin_popcountll(board.blackQueens)  * Q_VAL;

    // Score Final
    // O valor deve ser relativo ao lado que joga (Conceito do Negamax)
    // Se for branco: (White - Black)
    // Se for preto:  (Black - White)
    
    int score = whiteMat - blackMat;

    if (!board.whiteToMove) {
        score = -score;
    }

    return score;
}
