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

// ==========================================
//  Main Loop
// ==========================================

int main(int argc, char* argv[]) {
    // 1. Configuração Inicial
    // Se passar argumento, usa como FEN, senão usa padrão
    std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    if (argc > 1) {
        startFen = argv[1];
    }
    
    Board board = Board::fromFEN(startFen.c_str());
    int depth = 5;

    std::cout << "=== CHESS ENGINE PROTOTYPE ===\n";
    std::cout << "Comandos: \n";
    std::cout << " - 'e2e4': joga o lance\n";
    std::cout << " - 'go': joga o lance sugerido pela engine\n";
    std::cout << " - 'fen [string]': carrega nova posicao\n";
    std::cout << " - 'depth [n]': altera profundidade (atual: " << depth << ")\n";
    std::cout << " - 'quit': sair\n\n";

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

    return 0;
}
