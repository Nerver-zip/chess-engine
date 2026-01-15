#include "movegen.h"
#include <iostream>

// Calcula o score do movimento seguindo MVV-LVA
static int scoreMove(const Board& board, int from, int to, uint8_t flags, uint8_t promotion) {
    int score = 0;

    if (flags & CAPTURE) {
        int victim = board.pieceAt(to);
        int attacker = board.pieceAt(from);
        
        // Ajuste para En Passant (a vítima não está em 'to')
        if (flags & EN_PASSANT) {
            victim = (attacker == WPAWN) ? BPAWN : WPAWN;
        }

        // MVV-LVA: Vítima valiosa - Atacante barato
        // Somamos um offset grande (10000) para garantir que capturas sejam
        // analisadas antes de lances quietos na busca principal.
        score = MVV_LVA_VALUES[victim] - MVV_LVA_VALUES[attacker] + OFFSET;
    }

    if (flags & PROMOTION) {
        // Promoção vale muito (geralmente vira Dama)
        score += MVV_LVA_VALUES[promotion] + 1000;
    }

    return score;
}

// ------------------------------------------
// Validador Normal (MoveGen::generateMoves)
// ------------------------------------------
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
    m.score = scoreMove(board, from, to, flags, promotion);
    moves.push_back(m);
    return true;
}

/*
// --------------------------------------------------
// Filtro de lances de promoção, captura e xeque 
// --------------------------------------------------
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
        m.score = scoreMove(board, from, to, flags, promotion);
        moves.push_back(m);
        return true;
    }

    return false;
}
*/

// Helper para encontrar o bit da peça de menor valor num bitboard de atacantes
static int getLeastValuableAttacker(const Board& board, uint64_t attackers, bool whiteToMove) {
    // Ordem de verificação: Peão -> Cavalo -> Bispo -> Torre -> Dama -> Rei
    
    uint64_t pawns = whiteToMove ? board.whitePawns : board.blackPawns;
    if (attackers & pawns) return __builtin_ctzll(attackers & pawns);

    uint64_t knights = whiteToMove ? board.whiteKnights : board.blackKnights;
    if (attackers & knights) return __builtin_ctzll(attackers & knights);

    uint64_t bishops = whiteToMove ? board.whiteBishops : board.blackBishops;
    if (attackers & bishops) return __builtin_ctzll(attackers & bishops);

    uint64_t rooks = whiteToMove ? board.whiteRooks : board.blackRooks;
    if (attackers & rooks) return __builtin_ctzll(attackers & rooks);

    uint64_t queens = whiteToMove ? board.whiteQueens : board.blackQueens;
    if (attackers & queens) return __builtin_ctzll(attackers & queens);

    uint64_t king = whiteToMove ? board.whiteKing : board.blackKing;
    if (attackers & king) return __builtin_ctzll(attackers & king);

    return -1;
}

// ============================================================================
// SEE (Static Exchange Evaluation)
// Simula a sequência de trocas na casa 'to' para determinar se o lance é vantajoso.
// Considera raios-X (baterias) e a opção de parar a troca
static bool see(const Board& board, int from, int to, int capturedPieceType) {
    int gain[32];
    int d = 0;

    gain[d] = MVV_LVA_VALUES[capturedPieceType];

    uint64_t fromBB = (1ULL << from);
    uint64_t occ = board.allPieces();
    occ &= ~fromBB; 
    occ |= (1ULL << to); 

    uint64_t attackers = board.attackersTo(to, occ);
    bool sideToMove = !board.whiteToMove; 
    int attackerType = board.pieceAt(from);
    
    while (true) {
        d++;
        
        gain[d] = MVV_LVA_VALUES[attackerType] - gain[d-1];

        // Pruning: Se o ganho acumulado é ruim e o oponente pode parar, ele para.
        if (std::max(-gain[d-1], gain[d]) < 0) break; 
        
        // Filtra atacantes usando & occ
        uint64_t myAttackers = attackers & (sideToMove ? board.whitePieces() : board.blackPieces()) & occ;
        
        if (myAttackers == 0) break;

        int lvaSq = getLeastValuableAttacker(board, myAttackers, sideToMove);
        if (lvaSq == -1) break;

        int lvaPiece = board.pieceAt(lvaSq); 
        uint64_t lvaBB = (1ULL << lvaSq);
        
        // Remove a peça da ocupação (ela foi "comida" ou "moveu")
        occ &= ~lvaBB; 
        
        // Atualização de Raio-X
        // (Isso vai re-adicionar lvaSq em 'attackers' porque o raio passa por lá e a peça estática existe,
        //  mas o filtro '& occ' na próxima iteração vai ignorá-la.
        if (lvaPiece == WPAWN || lvaPiece == BPAWN || 
            lvaPiece == WBISHOP || lvaPiece == BBISHOP || 
            lvaPiece == WQUEEN || lvaPiece == BQUEEN) {
            attackers |= (bishopAttacks(to, occ) & (board.whiteBishops | board.blackBishops | board.whiteQueens | board.blackQueens));
        }
        if (lvaPiece == WROOK || lvaPiece == BROOK || 
            lvaPiece == WQUEEN || lvaPiece == BQUEEN) {
            attackers |= (rookAttacks(to, occ) & (board.whiteRooks | board.blackRooks | board.whiteQueens | board.blackQueens));
        }

        attackerType = lvaPiece;
        sideToMove = !sideToMove;
    }

    // Minimax reverso
    while (--d) {
        gain[d-1] = -std::max(-gain[d-1], gain[d]);
    }

    return gain[0] >= 0;
}

// =============================================================================================
// Validador QSearch = Filtro por promoções e capturas que ganham material (avaliação estática)
// =============================================================================================
static bool addWinningCaptureIfLegal(const Board& board, std::vector<Move>& moves,
                                     int from, int to, uint8_t flags, uint8_t promotion = EMPTY)
{
    // Filtro Básico: Apenas Capturas e Promoções
    bool isCapture = flags & CAPTURE;
    bool isPromotion = flags & PROMOTION;
    if (!isCapture && !isPromotion) return false;

    // Calcula Score MVV-LVA e prepara o objeto Move
    Move m; m.from = from; m.to = to; m.flags = flags; m.promotion = promotion;
    m.score = scoreMove(board, from, to, flags, promotion);

    // Static Exchange Evaluation (SEE)
    // Se a captura parece ruim (vítima <= atacante), verificamos se a troca compensa.
    if (isCapture) {
        int victim = board.pieceAt(to);
        
        if (flags & EN_PASSANT) {
            victim = board.whiteToMove ? BPAWN : WPAWN;
        }

        int victimVal = MVV_LVA_VALUES[victim];
        int attackerVal = MVV_LVA_VALUES[board.pieceAt(from)];

        // Regra simples para SEE:
        // 1. Se capturamos peça mais valiosa (PxQ), aceitamos direto (Good Capture), assumindo que demais trocas podem ser interrompidas
        // por quem já obteve ganho.
        // 2. Se capturamos igual ou menor (QxP ou PxP), rodamos SEE para ver se não perdemos na troca.
        if (victimVal <= attackerVal) {
             if (!see(board, from, to, victim)) {
                 return false; // SEE diz que perdemos material -> Corta o lance (Pruning)
             }
        }
    }

    Board next = board.applyMove(m);
    next.updateAttackBoards();

    bool movedByWhite = !next.whiteToMove;
    if (movedByWhite) {
        if (next.whiteKing & next.blackAttacks) return false;
    } else {
        if (next.blackKing & next.whiteAttacks) return false;
    }

    moves.push_back(m);
    return true;
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
        return addWinningCaptureIfLegal(b,m,f,t,fl,p);
    }
};

std::vector<Move> MoveGen::generateMoves(const Board& board)
{
    std::vector<Move> moves;
    moves.reserve(256);
    generateAll(board, moves, NormalValidator{});
    return moves;
}

std::vector<Move> MoveGen::generatePieceMoves(const Board &board, Piece piece){
    std::vector<Move> moves;
    moves.reserve(16);

    if(piece == WPAWN || piece == BPAWN)
        generatePawnMoves(board, moves, NormalValidator{});
    else if(piece == WKNIGHT || piece == BKNIGHT)
        generateKnightMoves(board, moves, NormalValidator{});
    else if(piece == WBISHOP || piece == BBISHOP)
        generateBishopMoves(board, moves, NormalValidator{});
    else if(piece == WROOK || piece == BROOK)
        generateRookMoves(board, moves, NormalValidator{});
    else if(piece == WQUEEN || piece == BQUEEN)
        generateQueenMoves(board, moves, NormalValidator{});
    else
        generateKingMoves(board, moves, NormalValidator{});
    
    return moves;
}

std::vector<Move> MoveGen::generateWinningMoves(const Board& board)
{
    std::vector<Move> moves;
    moves.reserve(64);
    generateAll(board, moves, QSearchValidator{});
    return moves;
}


std::vector<Move> MoveGen::generateCheckResponses(const Board& board)
{
    std::vector<Move> moves;
    moves.reserve(32);

    const bool white = board.whiteToMove;
    const uint64_t own  = ownPieces(white, board);
    const uint64_t all  = board.allPieces();

    const int kingSq = __builtin_ctzll(white ? board.whiteKing : board.blackKing);

    uint64_t checkers = board.attackersTo(kingSq, all) & enemyPieces(white, board);

    if (!checkers)
        return generateMoves(board);

    // ============================================================
    // DOUBLE CHECK → só rei
    // ============================================================
    if (__builtin_popcountll(checkers) > 1) {
        std::vector<Move> kingOnly;
        generateKingMoves(board, kingOnly, NormalValidator{});
        return kingOnly;
    }

    // ============================================================
    // SINGLE CHECK
    // ============================================================
    const int checkerSq = __builtin_ctzll(checkers);
    const int checker   = board.pieceAt(checkerSq);

    uint64_t blockMask = 0;

    if (checker >= WBISHOP) // bishop, rook, queen
        blockMask = squaresBetween(kingSq, checkerSq);

    // ----------------- KING -----------------
    generateKingMoves(board, moves, NormalValidator{});

    // ----------------- CAPTURE CHECKER -----------------
    uint64_t attackers = board.attackersTo(checkerSq, all) & own;

    while (attackers) {
        int from = __builtin_ctzll(attackers);
        attackers &= attackers - 1;

        bool promo = (board.pieceAt(from) == (white ? WPAWN : BPAWN)) &&
                     (checkerSq / 8 == (white ? 7 : 0));

        if (promo) {
            NormalValidator{}(board, moves, from, checkerSq, CAPTURE|PROMOTION, white ? WQUEEN : BQUEEN);
            NormalValidator{}(board, moves, from, checkerSq, CAPTURE|PROMOTION, white ? WROOK  : BROOK);
            NormalValidator{}(board, moves, from, checkerSq, CAPTURE|PROMOTION, white ? WBISHOP: BBISHOP);
            NormalValidator{}(board, moves, from, checkerSq, CAPTURE|PROMOTION, white ? WKNIGHT: BKNIGHT);
        } else {
            NormalValidator{}(board, moves, from, checkerSq, CAPTURE);
        }
    }

    // ----------------- BLOCK RAY -----------------
    if (blockMask) {
        uint64_t blockers = blockMask & ~all;

        while (blockers) {
            int to = __builtin_ctzll(blockers);
            blockers &= blockers - 1;

            uint64_t cand = board.attackersTo(to, all) & own;

            while (cand) {
                int from = __builtin_ctzll(cand);
                cand &= cand - 1;
                NormalValidator{}(board, moves, from, to, QUIET);
            }
        }
    }

    return moves;
}

