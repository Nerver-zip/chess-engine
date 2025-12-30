#pragma once
#include <cstdint>
#include "piece.h"
#include "../move/move.h"

struct Board {

    // ===== Bitboards =====
    uint64_t whitePawns;
    uint64_t whiteKnights;
    uint64_t whiteBishops;
    uint64_t whiteRooks;
    uint64_t whiteQueens;
    uint64_t whiteKing;

    uint64_t blackPawns;
    uint64_t blackKnights;
    uint64_t blackBishops;
    uint64_t blackRooks;
    uint64_t blackQueens;
    uint64_t blackKing;

    // ===== Estado =====
    bool whiteToMove;
    uint8_t castlingRights;   // bits: 0001 WK, 0010 WQ, 0100 BK, 1000 BQ
    int8_t enPassantSquare;   // -1 se não houver
    
    // Mapa de ataque, casas controladas por cada peça
    int64_t whiteAttacks;
    uint64_t blackAttacks;

    /**
     * @brief 
     * Retorna uma bit board representando as peças brancas
     * @return 
     */
    inline uint64_t whitePieces() const {
        return whitePawns | whiteKnights | whiteBishops | whiteRooks | whiteQueens | whiteKing;
    }

    /**
     * @brief 
     * Retorna uma bit board representando as peças pretas
     * @return Bit board
     */
    inline uint64_t blackPieces() const {
        return blackPawns | blackKnights | blackBishops | blackRooks | blackQueens | blackKing;
    }

    /**
     * @brief 
     * Retorna uma bit board unindo todas as peças, sejam elas brancas ou pretas
     * @return Bit board
     */
    inline uint64_t allPieces() const {
        return whitePieces() | blackPieces();
    }

    /**
     * @brief 
     * A partir de uma casa, verifica que peça está nela
     * @param sq, a posição encodada num inteiro 64 bits, em bit board
     * @return Enum referente a peça ou vazio 
     */
    inline Piece pieceAt(int sq) const;   // retorna enum Piece ou 0 se vazio

    /**
     * @brief 
     * Cuida da lógica da aplicação do movimento. No momento, sem configurar roque,
     * en passant e promoções
     * @param m -> Movimento a ser feito
     * @return -> Retorna o tabuleiro com o movimento aplicado
     */
    Board applyMove(const Move& m) const;

    /**
     * @brief 
     * Inicializa uma posição de xadrez 'Board' a partir de um FEN
     * @param fen 
     * @return A posição de xadrez, objeto 'Board'
     */
    static Board fromFEN(const char* fen);

    static Board fromPGN(const char* pgn);

    
    /**
     * @brief Essa função constrói do zero um mapa de ataque que auxilia na geração
     * de movimentos válidos para as peças. Num futuro update, podemos pensar em 
     * uma implementação incremental, que não calcula tudo do zero.
     */
    void updateAttackBoards();
};
