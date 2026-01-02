#include "movegen.h"
#include "../board/attack.h"
#include "../board/bitboard.h"

// Helper para evitar repetição de código na validação
// Retorna true se o movimento foi adicionado (era legal), false se era ilegal (rei em xeque)
static bool isValidMove(const Board& board, std::vector<Move>& moves, int from, int to, uint8_t flags, uint8_t promotion = EMPTY) {
    Move m;
    m.from = from;
    m.to = to;
    m.flags = flags;
    m.promotion = promotion;

    // 1. Aplica o movimento num tabuleiro temporário
    Board nextBoard = board.applyMove(m);

    // 2. Atualiza o mapa de ataques do novo tabuleiro
    nextBoard.updateAttackBoards();

    // 3. Verifica se o rei de quem acabou de mover (o "eu" do turno anterior) está em xeque
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

std::vector<Move> MoveGen::generateMoves(const Board& board) {
    std::vector<Move> moves;
    moves.reserve(256);

    generatePawnMoves(board, moves);
    generateKnightMoves(board, moves);
    generateBishopMoves(board, moves);
    generateRookMoves(board, moves);
    generateQueenMoves(board, moves);
    generateKingMoves(board, moves);

    return moves;
}

void MoveGen::generatePawnMoves(const Board& board, std::vector<Move>& moves) {
    bool white = board.whiteToMove;
    uint64_t pawns = white ? board.whitePawns : board.blackPawns;
    uint64_t enemies = white ? board.blackPieces() : board.whitePieces();
    uint64_t empty = ~board.allPieces();

    int up = white ? 8 : -8;
    int promRank = white ? 7 : 0;
    int startRank = white ? 1 : 6;

    // Iteração padrão de bitboard
    uint64_t copy = pawns;
    while (copy) {
        int from = __builtin_ctzll(copy);
        copy &= copy - 1; // Remove o bit processado

        int r = from / 8;

        // --- 1. Movimento Simples (Push) ---
        int to = from + up;
        if (BB(to) & empty) {
            // Verifica Promoção
            if ((to / 8) == promRank) {
                isValidMove(board, moves, from, to, PROMOTION, white ? WQUEEN : BQUEEN);
                isValidMove(board, moves, from, to, PROMOTION, white ? WROOK : BROOK);
                isValidMove(board, moves, from, to, PROMOTION, white ? WBISHOP : BBISHOP);
                isValidMove(board, moves, from, to, PROMOTION, white ? WKNIGHT : BKNIGHT);
            } else {
                // Push normal
                isValidMove(board, moves, from, to, QUIET);

                // --- 2. Movimento Duplo (Double Push) ---
                // Só possível se o single push foi válido e está no rank inicial
                if (r == startRank) {
                    int toDouble = from + (up * 2);
                    if (BB(toDouble) & empty) {
                        isValidMove(board, moves, from, toDouble, DOUBLE_PAWN_PUSH);
                    }
                }
            }
        }

        // --- 3. Capturas ---
        // Usamos a tabela pré-calculada em attack.h para saber onde esse peão ataca
        // whiteIndex=0, blackIndex=1 na array PAWN_ATTACKS
        uint64_t attacks = white ? PAWN_ATTACKS[0][from] : PAWN_ATTACKS[1][from];

        // Filtra apenas ataques que caem em peças inimigas
        uint64_t validCaptures = attacks & enemies;
        
        // Loop pelas capturas normais
        while (validCaptures) {
            int captureTo = __builtin_ctzll(validCaptures);
            validCaptures &= validCaptures - 1;

            if ((captureTo / 8) == promRank) {
                isValidMove(board, moves, from, captureTo, PROMOTION | CAPTURE, white ? WQUEEN : BQUEEN);
                isValidMove(board, moves, from, captureTo, PROMOTION | CAPTURE, white ? WROOK : BROOK);
                isValidMove(board, moves, from, captureTo, PROMOTION | CAPTURE, white ? WBISHOP : BBISHOP);
                isValidMove(board, moves, from, captureTo, PROMOTION | CAPTURE, white ? WKNIGHT : BKNIGHT);
            } else {
                isValidMove(board, moves, from, captureTo, CAPTURE);
            }
        }

        // --- 4. En Passant ---
        if (board.enPassantSquare != -1) {
            uint64_t epBB = BB(board.enPassantSquare);
            // Se o peão ataca a casa de en passant
            if (attacks & epBB) {
                isValidMove(board, moves, from, board.enPassantSquare, EN_PASSANT | CAPTURE);
            }
        }
    }
}

void MoveGen::generateKnightMoves(const Board& board, std::vector<Move>& moves) {
    bool white = board.whiteToMove;
    uint64_t knights = white ? board.whiteKnights : board.blackKnights;
    uint64_t own = white ? board.whitePieces() : board.blackPieces();
    uint64_t enemies = white ? board.blackPieces() : board.whitePieces();

    while (knights) {
        int from = __builtin_ctzll(knights);
        knights &= knights - 1;

        // Pega ataques da lookup table e remove peças próprias
        uint64_t attacks = KNIGHT_ATTACKS[from] & ~own;

        while (attacks) {
            int to = __builtin_ctzll(attacks);
            attacks &= attacks - 1;

            bool isCapture = (BB(to) & enemies);
            isValidMove(board, moves, from, to, isCapture ? CAPTURE : QUIET);
        }
    }
}

void MoveGen::generateBishopMoves(const Board& board, std::vector<Move>& moves) {
    bool white = board.whiteToMove;
    uint64_t bishops = white ? board.whiteBishops : board.blackBishops;
    uint64_t own = white ? board.whitePieces() : board.blackPieces();
    uint64_t enemies = white ? board.blackPieces() : board.whitePieces();
    uint64_t all = board.allPieces();

    while (bishops) {
        int from = __builtin_ctzll(bishops);
        bishops &= bishops - 1;

        // Magic Bitboards para gerar ataques deslizantes
        uint64_t attacks = bishopAttacks(from, all) & ~own;

        while (attacks) {
            int to = __builtin_ctzll(attacks);
            attacks &= attacks - 1;

            bool isCapture = (BB(to) & enemies);
            isValidMove(board, moves, from, to, isCapture ? CAPTURE : QUIET);
        }
    }
}

void MoveGen::generateRookMoves(const Board& board, std::vector<Move>& moves) {
    bool white = board.whiteToMove;
    uint64_t rooks = white ? board.whiteRooks : board.blackRooks;
    uint64_t own = white ? board.whitePieces() : board.blackPieces();
    uint64_t enemies = white ? board.blackPieces() : board.whitePieces();
    uint64_t all = board.allPieces();

    while (rooks) {
        int from = __builtin_ctzll(rooks);
        rooks &= rooks - 1;

        uint64_t attacks = rookAttacks(from, all) & ~own;

        while (attacks) {
            int to = __builtin_ctzll(attacks);
            attacks &= attacks - 1;

            bool isCapture = (BB(to) & enemies);
            isValidMove(board, moves, from, to, isCapture ? CAPTURE : QUIET);
        }
    }
}

void MoveGen::generateQueenMoves(const Board& board, std::vector<Move>& moves) {
    bool white = board.whiteToMove;
    uint64_t queens = white ? board.whiteQueens : board.blackQueens;
    uint64_t own = white ? board.whitePieces() : board.blackPieces();
    uint64_t enemies = white ? board.blackPieces() : board.whitePieces();
    uint64_t all = board.allPieces();

    while (queens) {
        int from = __builtin_ctzll(queens);
        queens &= queens - 1;

        // Dama = Bispo | Torre
        uint64_t attacks = (bishopAttacks(from, all) | rookAttacks(from, all)) & ~own;

        while (attacks) {
            int to = __builtin_ctzll(attacks);
            attacks &= attacks - 1;

            bool isCapture = (BB(to) & enemies);
            isValidMove(board, moves, from, to, isCapture ? CAPTURE : QUIET);
        }
    }
}

void MoveGen::generateKingMoves(const Board& board, std::vector<Move>& moves) {
    bool white = board.whiteToMove;
    uint64_t king = white ? board.whiteKing : board.blackKing;
    uint64_t own = white ? board.whitePieces() : board.blackPieces();
    uint64_t enemies = white ? board.blackPieces() : board.whitePieces();
    uint64_t enemyAttacks = white ? board.blackAttacks : board.whiteAttacks;
    uint64_t all = board.allPieces();

    if (king == 0) return; // Segurança
    int from = __builtin_ctzll(king);

    // 1. Movimentos normais do Rei
    // O rei não pode ir para casas controladas pelo inimigo (Pseudo-Legal filtering)
    // Nota: isValidMove fará a verificação estrita final, mas podemos filtrar o óbvio aqui
    uint64_t attacks = KING_ATTACKS[from] & ~own & ~enemyAttacks;

    while (attacks) {
        int to = __builtin_ctzll(attacks);
        attacks &= attacks - 1;

        bool isCapture = (BB(to) & enemies);
        isValidMove(board, moves, from, to, isCapture ? CAPTURE : QUIET);
    }

    // 2. Castling
    // Requisitos: Rei não está em xeque, caminho livre, caminho não atacado.

    // Se o rei já está em xeque, não pode rocar
    if (king & enemyAttacks) return;

    if (white) {
        // --- White King Side (K) ---
        // Direitos bit 0 (1), casas f1(5), g1(6) vazias e não atacadas
        if ((board.castlingRights & 1) && 
            !(all & (BB(5) | BB(6))) && 
            !(enemyAttacks & (BB(5) | BB(6)))) 
        {
            isValidMove(board, moves, 4, 6, KING_CASTLE);
        }

        // --- White Queen Side (Q) ---
        // Direitos bit 1 (2), casas b1(1), c1(2), d1(3) vazias
        // Rei passa por c1(2), d1(3), e1(4). c1 e d1 não podem estar atacadas. b1 pode estar atacada.
        if ((board.castlingRights & 2) && 
            !(all & (BB(1) | BB(2) | BB(3))) && 
            !(enemyAttacks & (BB(2) | BB(3)))) 
        {
            isValidMove(board, moves, 4, 2, QUEEN_CASTLE);
        }

    } else {
        // --- Black King Side (k) ---
        // Direitos bit 2 (4), casas f8(61), g8(62)
        if ((board.castlingRights & 4) && 
            !(all & (BB(61) | BB(62))) && 
            !(enemyAttacks & (BB(61) | BB(62)))) 
        {
            isValidMove(board, moves, 60, 62, KING_CASTLE);
        }

        // --- Black Queen Side (q) ---
        // Direitos bit 3 (8), casas b8(57), c8(58), d8(59)
        if ((board.castlingRights & 8) && 
            !(all & (BB(57) | BB(58) | BB(59))) && 
            !(enemyAttacks & (BB(58) | BB(59)))) 
        {
            isValidMove(board, moves, 60, 58, QUEEN_CASTLE);
        }
    }
}

// No momento usando mask ~ownPieces nas funções acima)
bool MoveGen::isOwnPiece(const Board& board, int sq, bool white) {
    uint64_t own = white ? board.whitePieces() : board.blackPieces();
    return (BB(sq) & own) != 0;
}
