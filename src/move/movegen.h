#pragma once
#include <vector>
#include "../board/board.h"
#include "move.h"

class MoveGen {
public:
    /**
     * @brief Gera todos os lances legais (ou pseudo-legais) para o lado que tem a vez
     * @param board Estado atual do tabuleiro
     * @return Vetor de Moves
     */
    static std::vector<Move> generateMoves(const Board& board);

private:
    // Funções auxiliares
    static void generatePawnMoves(const Board& board, std::vector<Move>& moves);
    static void generateKnightMoves(const Board& board, std::vector<Move>& moves);
    static void generateBishopMoves(const Board& board, std::vector<Move>& moves);
    static void generateRookMoves(const Board& board, std::vector<Move>& moves);
    static void generateQueenMoves(const Board& board, std::vector<Move>& moves);
    static void generateKingMoves(const Board& board, std::vector<Move>& moves);

    // Função helper para verificar se uma casa está ocupada pelo próprio lado
    static bool isOwnPiece(const Board& board, int sq, bool white);
};
