#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <algorithm>

#include "../../move/movegen.h"
#include "../../search/search.h"

// ==========================================
//  Helpers de Visualização e Parsing
// ==========================================

// Converte índice (0..63) para string ("e4")
std::string sqStr(int sq) {
    std::string s;
    s += (char)('a' + (sq % 8));
    s += (char)('1' + (sq / 8));
    return s;
}

// Converte string ("e4") para índice
int parseSq(std::string s) {
    if (s.size() < 2) return -1;
    int f = s[0] - 'a';
    int r = s[1] - '1';
    if (f < 0 || f > 7 || r < 0 || r > 7) return -1;
    return r * 8 + f;
}

// Converte Move para String ("e2e4")
std::string moveStr(const Move& m) {
    std::string s = sqStr(m.from) + sqStr(m.to);
    if (m.flags & PROMOTION) {
        switch (m.promotion) {
            case WQUEEN: case BQUEEN: s += 'q'; break;
            case WROOK:  case BROOK:  s += 'r'; break;
            case WBISHOP: case BBISHOP: s += 'b'; break;
            case WKNIGHT: case BKNIGHT: s += 'n'; break;
        }
    }
    return s;
}

// Imprime o tabuleiro com caracteres ASCII simples
void printBoard(const Board& b) {
    std::cout << "\n   +-----------------+\n";
    for (int r = 7; r >= 0; --r) {
        std::cout << " " << r + 1 << " | ";
        for (int f = 0; f < 8; ++f) {
            int sq = r * 8 + f;
            Piece p = b.pieceAt(sq);
            char c = '.';
            switch (p) {
                case WPAWN: c = 'P'; break; case BPAWN: c = 'p'; break;
                case WKNIGHT: c = 'N'; break; case BKNIGHT: c = 'n'; break;
                case WBISHOP: c = 'B'; break; case BBISHOP: c = 'b'; break;
                case WROOK: c = 'R'; break; case BROOK: c = 'r'; break;
                case WQUEEN: c = 'Q'; break; case BQUEEN: c = 'q'; break;
                case WKING: c = 'K'; break; case BKING: c = 'k'; break;
                default: break;
            }
            std::cout << c << " ";
        }
        std::cout << "|\n";
    }
    std::cout << "   +-----------------+\n";
    std::cout << "     a b c d e f g h\n\n";
    std::cout << "Vez de: " << (b.whiteToMove ? "Brancas" : "Pretas") << "\n";
}

int main() {
    
    // Posições de mate
    constexpr std::array<std::string_view, 10> positions = {{
        "7k/6pp/8/8/8/8/8/R3K3 w Q - 0 1",       // M1 - back rank mate
        "7k/6pp/8/8/8/8/R7/R3K2b w HQha - 0 1",  // M2 - back rank mate
        "r6k/6pp/8/4b3/8/1Q6/1R6/1R2K3 w q - 0 1", // M3 - back rank mate
        "5rk1/5Npp/r7/8/8/1Q2b3/8/4K3 w - - 0 1", // M3 - smothered
        "r5rk/5p1p/5R2/4B3/8/8/7P/7K w", // M3
        "Q7/p1p1q1pk/3p2rp/4n3/3bP3/7b/PP3PPK/R1B2R2 b - - 0 1", //M4
        "r1bqr3/ppp1B1kp/1b4p1/n2B4/3PQ1P1/2P5/P4P2/RN4K1 w - - 1 0", //M4 Morphy
        "4rb1k/2pqn2p/6pn/ppp3N1/P1QP2b1/1P2p3/2B3PP/B3RRK1 w - - 0 24", //M5
        "4rr2/1p4bk/2p3pn/B3n2b/P4N1q/1P5P/6PK/1BQ1RR2 b - - 1 31", //M6
        "5r2/1pB2rbk/6pn/4n2q/P3B3/1P5P/4R1P1/2Q2R1K b - - 3 33" // M7
    }};
    
    for (const auto& position : positions) {
        Board board = Board::fromFEN(position.data());
        int depth = 6;

        while (true) {
            board.updateAttackBoards();

            printBoard(board);

            std::vector<Move> legalMoves = MoveGen::generateMoves(board);

            if (legalMoves.empty()) {
                bool inCheck = board.whiteToMove 
                               ? (board.whiteKing & board.blackAttacks)
                               : (board.blackKing & board.whiteAttacks);
                std::cout << "FIM DE JOGO: " << (inCheck ? "Xeque-Mate!" : "Afogamento (Empate)") << "\n";
                break;
            }

            std::cout << "Calculando melhor lance (depth " << depth << ")... ";
            std::cout.flush();

            auto start = std::chrono::high_resolution_clock::now();
            Move bestMove = Search::searchBestMove(board, depth);
            auto end = std::chrono::high_resolution_clock::now();
            
            long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            std::string bestStr = moveStr(bestMove);
            std::cout << "Feito em " << ms << "ms.\n";
            std::cout << ">> Sugestao da Engine: " << bestStr << "\n";

            // ==========================================
            //  Input do Usuário
            // ==========================================
            std::cout << "> Digite seu lance: ";
            std::string input;
            std::getline(std::cin, input);

            if (input == "quit" || input == "exit") break;
            
            if (input == "go" || input == "") {
                // Joga o que a engine sugeriu
                board = board.applyMove(bestMove);
                std::cout << "Engine jogou: " << bestStr << "\n";
                continue;
            }

            if (input.rfind("depth ", 0) == 0) {
                depth = std::stoi(input.substr(6));
                std::cout << "Profundidade ajustada para " << depth << "\n";
                continue;
            }
            
            if (input.rfind("fen ", 0) == 0) {
                std::string fen = input.substr(4);
                board = Board::fromFEN(fen.c_str());
                std::cout << "Nova posicao carregada.\n";
                continue;
            }

            // Tenta jogar o lance do usuário (Coordinate Notation: e2e4)
            bool found = false;
            for (const auto& m : legalMoves) {
                if (moveStr(m) == input) {
                    board = board.applyMove(m);
                    found = true;
                    break;
                }
            }

            if (!found) {
                std::cout << "Lance invalido ou ilegal! Use notacao coordenada (ex: e2e4, a7a8q).\n";
            }
        }
 
    }

    
    return 0;
}
