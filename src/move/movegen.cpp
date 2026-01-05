#include "movegen.h"

// Helper para evitar repetição de código na validação
// Retorna true se o movimento foi adicionado (era legal), false se era ilegal (rei em xeque)
static bool validator(const Board& board, std::vector<Move>& moves, int from, int to, uint8_t flags, uint8_t promotion = EMPTY) {
    Move m;
    m.from = from;
    m.to = to;
    m.flags = flags;
    m.promotion = promotion;

    // Aplica o movimento num tabuleiro temporário
    Board nextBoard = board.applyMove(m);

    // Atualiza o mapa de ataques do novo tabuleiro
    nextBoard.updateAttackBoards();

    // Verifica se o rei de quem acabou de mover (o "eu" do turno anterior) está em xeque
    // Nota: applyMove inverte o turno (whiteToMove). 
    // Se quem moveu foi BRANCO, agora é vez do PRETO. Checamos se o rei BRANCO está sob ataque do PRETO.
    
    bool moveWasByWhite = !nextBoard.whiteToMove; 

    if (moveWasByWhite) {
        // Se o rei branco colide com os ataques pretos -> Ilegal
        if (nextBoard.whiteKing & nextBoard.blackAttacks) return false;
    } else {
        // Se o rei preto colide com os ataques brancos -> Ilegal
        if (nextBoard.blackKing & nextBoard.whiteAttacks) return false;
    }

    // Se passou no teste, adiciona à lista
    moves.push_back(m);
    return true;
}

// Helper que filtra se o movimento além de legal, é "forçante"
static bool addForcingIfLegal(const Board& board, std::vector<Move>& moves,
                              int from, int to, uint8_t flags, uint8_t promotion = EMPTY)
{
    Move m;
    m.from = from;
    m.to = to;
    m.flags = flags;
    m.promotion = promotion;

    Board next = board.applyMove(m);
    next.updateAttackBoards();

    bool movedByWhite = !next.whiteToMove;

    if (movedByWhite) {
        if (next.whiteKing & next.blackAttacks) return false;
    } else {
        if (next.blackKing & next.whiteAttacks) return false;
    }

    bool givesCheck = false;
    if (movedByWhite) {
        givesCheck = (next.blackKing & next.whiteAttacks);
    } else {
        givesCheck = (next.whiteKing & next.blackAttacks);
    }

    bool isCapture = flags & CAPTURE;
    bool isPromotion = flags & PROMOTION;

    if (isCapture || isPromotion || givesCheck) {
        moves.push_back(m);
        return true;
    }

    return false;
}

struct NormalValidator {
    __attribute__((always_inline))
    inline bool operator()(const Board& b, std::vector<Move>& m,
                            int f,int t,uint8_t fl,uint8_t p = EMPTY) const {
        return validator(b,m,f,t,fl,p);
    }
};

struct QSearchValidator {
    __attribute__((always_inline))
    inline bool operator()(const Board& b, std::vector<Move>& m,
                            int f,int t,uint8_t fl,uint8_t p = EMPTY) const {
        return addForcingIfLegal(b,m,f,t,fl,p);
    }
};

std::vector<Move> MoveGen::generateMoves(const Board& board)
{
    std::vector<Move> moves;
    moves.reserve(256);
    generateAll(board, moves, NormalValidator{});
    return moves;
}

std::vector<Move> MoveGen::generateForcingMoves(const Board& board)
{
    std::vector<Move> moves;
    moves.reserve(64);
    generateAll(board, moves, QSearchValidator{});
    return moves;
}

// No momento usando mask ~ownPieces nas funções acima)
bool MoveGen::isOwnPiece(const Board& board, int sq, bool white) {
    uint64_t own = white ? board.whitePieces() : board.blackPieces();
    return (BB(sq) & own) != 0;
}
