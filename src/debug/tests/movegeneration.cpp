#include <iostream>
#include <vector>
#include <chrono>

#include "../../board/board.h"   
#include "../../move/movegen.h"
#include "../../debuglib/debug.h"


int main() {
    // 1. Inicializa o tabuleiro com a posição inicial
    // FEN padrão: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    Board board = Board::fromFEN("8/8/4k3/8/1b6/2N5/3K4/8 w HAha - 0 1");
    
    std::cout << "--- Tabuleiro Inicializado ---\n";
    
    board.updateAttackBoards(); 
    
    Debug::printBoard(board);
    Debug::printAttackMaps(board);

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<Move> moves = MoveGen::generateMoves(board);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::cout << "Movimentos gerados: " << moves.size() << "\n";
    std::cout << "Tempo: " 
              << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() 
              << "us\n";

    for (const auto& m : moves) {
        Debug::printMove(m);
    }

    return 0;
}
