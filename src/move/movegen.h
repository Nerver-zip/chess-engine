#pragma once
#include <vector>
#include "../board/board.h"
#include "../board/attack.h"
#include "../board/bitboard.h"
#include "move.h"

class MoveGen {
public:
    
    // Para busca genérica
    static std::vector<Move> generateMoves(const Board& board);

    // Para Q-search
    static std::vector<Move> generateForcingMoves(const Board& board);

private:

    template<typename Validator>
    static void generateAll(const Board& board, std::vector<Move>& moves, Validator&& validator)
    {
        generatePawnMoves(board, moves, validator);
        generateKnightMoves(board, moves, validator);
        generateBishopMoves(board, moves, validator);
        generateRookMoves(board, moves, validator);
        generateQueenMoves(board, moves, validator);
        generateKingMoves(board, moves, validator);
    }

    template<typename Validator>
    static inline void generatePawnMoves(const Board& board, std::vector<Move>& moves, Validator&& validator) {
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
                    validator(board, moves, from, to, PROMOTION, white ? WQUEEN : BQUEEN);
                    validator(board, moves, from, to, PROMOTION, white ? WROOK : BROOK);
                    validator(board, moves, from, to, PROMOTION, white ? WBISHOP : BBISHOP);
                    validator(board, moves, from, to, PROMOTION, white ? WKNIGHT : BKNIGHT);
                } else {
                    // Push normal
                    validator(board, moves, from, to, QUIET);

                    // --- 2. Movimento Duplo (Double Push) ---
                    // Só possível se o single push foi válido e está no rank inicial
                    if (r == startRank) {
                        int toDouble = from + (up * 2);
                        if (BB(toDouble) & empty) {
                            validator(board, moves, from, toDouble, DOUBLE_PAWN_PUSH);
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
                    validator(board, moves, from, captureTo, PROMOTION | CAPTURE, white ? WQUEEN : BQUEEN);
                    validator(board, moves, from, captureTo, PROMOTION | CAPTURE, white ? WROOK : BROOK);
                    validator(board, moves, from, captureTo, PROMOTION | CAPTURE, white ? WBISHOP : BBISHOP);
                    validator(board, moves, from, captureTo, PROMOTION | CAPTURE, white ? WKNIGHT : BKNIGHT);
                } else {
                    validator(board, moves, from, captureTo, CAPTURE);
                }
            }

            // --- 4. En Passant ---
            if (board.enPassantSquare != -1) {
                uint64_t epBB = BB(board.enPassantSquare);
                // Se o peão ataca a casa de en passant
                if (attacks & epBB) {
                    validator(board, moves, from, board.enPassantSquare, EN_PASSANT | CAPTURE);
                }
            }
        }
    }


    template<typename Validator>
    static inline void generateKnightMoves(const Board& board, std::vector<Move>& moves, Validator&& validator) {
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
                validator(board, moves, from, to, isCapture ? CAPTURE : QUIET);
            }
        }
    }

    template<typename Validator>
    static inline void generateBishopMoves(const Board& board, std::vector<Move>& moves, Validator&& validator) {
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
                validator(board, moves, from, to, isCapture ? CAPTURE : QUIET);
            }
        }
    }

    template<typename Validator>
    static inline void generateRookMoves(const Board& board, std::vector<Move>& moves, Validator&& validator) {
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
                validator(board, moves, from, to, isCapture ? CAPTURE : QUIET);
            }
        }
    }

    template<typename Validator>
    static inline void generateQueenMoves(const Board& board, std::vector<Move>& moves, Validator&& validator) {
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
                validator(board, moves, from, to, isCapture ? CAPTURE : QUIET);
            }
        }
    }

    template<typename Validator>
    static inline void generateKingMoves(const Board& board, std::vector<Move>& moves, Validator&& validator) {
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
        // Nota: validator fará a verificação estrita final, mas podemos filtrar o óbvio aqui
        uint64_t attacks = KING_ATTACKS[from] & ~own & ~enemyAttacks;

        while (attacks) {
            int to = __builtin_ctzll(attacks);
            attacks &= attacks - 1;

            bool isCapture = (BB(to) & enemies);
            validator(board, moves, from, to, isCapture ? CAPTURE : QUIET);
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
                validator(board, moves, 4, 6, KING_CASTLE);
            }

            // --- White Queen Side (Q) ---
            // Direitos bit 1 (2), casas b1(1), c1(2), d1(3) vazias
            // Rei passa por c1(2), d1(3), e1(4). c1 e d1 não podem estar atacadas. b1 pode estar atacada.
            if ((board.castlingRights & 2) && 
                !(all & (BB(1) | BB(2) | BB(3))) && 
                !(enemyAttacks & (BB(2) | BB(3)))) 
            {
                validator(board, moves, 4, 2, QUEEN_CASTLE);
            }

        } else {
            // --- Black King Side (k) ---
            // Direitos bit 2 (4), casas f8(61), g8(62)
            if ((board.castlingRights & 4) && 
                !(all & (BB(61) | BB(62))) && 
                !(enemyAttacks & (BB(61) | BB(62)))) 
            {
                validator(board, moves, 60, 62, KING_CASTLE);
            }

            // --- Black Queen Side (q) ---
            // Direitos bit 3 (8), casas b8(57), c8(58), d8(59)
            if ((board.castlingRights & 8) && 
                !(all & (BB(57) | BB(58) | BB(59))) && 
                !(enemyAttacks & (BB(58) | BB(59)))) 
            {
                validator(board, moves, 60, 58, QUEEN_CASTLE);
            }
        }
    }

    static bool isOwnPiece(const Board& board, int sq, bool white);
};
