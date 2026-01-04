#include <iostream>
#include <vector>
#include <string>

#include "../../board/board.h"   
#include "../../move/movegen.h"
#include "../../debuglib/debug.h"

// Helper: Converte coordenadas de string ("e2") para índice (12)
int parseSquare(const std::string& s) {
    if (s.length() < 2) return -1;
    int f = s[0] - 'a';
    int r = s[1] - '1';
    if (f < 0 || f > 7 || r < 0 || r > 7) return -1;
    return r * 8 + f;
}

// Helper: Converte índice para string (para exibir a lista)
std::string squareToString(int sq) {
    std::string s = "";
    s += (char)('a' + (sq % 8));
    s += (char)('1' + (sq / 8));
    return s;
}

// Helper: Formata o movimento para string (ex: "e2e4" ou "a7a8q")
std::string moveToString(const Move& m) {
    std::string res = squareToString(m.from) + squareToString(m.to);
    
    // Adiciona sufixo de promoção se houver
    if (m.flags & PROMOTION) {
        switch (m.promotion) {
            case WQUEEN:  case BQUEEN:  res += 'q'; break;
            case WROOK:   case BROOK:   res += 'r'; break;
            case WBISHOP: case BBISHOP: res += 'b'; break;
            case WKNIGHT: case BKNIGHT: res += 'n'; break;
        }
    }
    return res;
}

int main() {
    // 1. Configuração Inicial
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; 
    
    // Ou posição inicial padrão:
    // std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    Board board = Board::fromFEN(fen.c_str());

    std::cout << "=== CHESS ENGINE INTERACTIVE TEST ===\n";
    std::cout << "Digite os lances em notação algébrica longa (ex: e2e4, a7a8q).\n";
    std::cout << "Digite 'quit' ou 'exit' para sair.\n\n";

    while (true) {
        // 2. Atualiza estado crítico
        board.updateAttackBoards();

        // 3. Gera movimentos válidos
        std::vector<Move> legalMoves = MoveGen::generateMoves(board);

        // 4. Imprime Interface
        Debug::printBoard(board);
        Debug::printAttackMaps(board);

        // Verifica fim de jogo
        if (legalMoves.empty()) {
            // Se não tem lances e o rei está em ataque = Xeque-mate
            bool inCheck = (board.whiteToMove && (board.whiteKing & board.blackAttacks)) ||
                           (!board.whiteToMove && (board.blackKing & board.whiteAttacks));
            
            std::cout << "\n=== FIM DE JOGO ===\n";
            if (inCheck) {
                std::cout << "Xeque-mate! Vitoria das " << (board.whiteToMove ? "Pretas" : "Brancas") << ".\n";
            } else {
                std::cout << "Afogamento (Stalemate)!\n";
            }
            break;
        }

        // 5. Lista movimentos possíveis
        std::cout << "\nMovimentos Legais (" << legalMoves.size() << "): ";
        for (const auto& m : legalMoves) {
            std::cout << moveToString(m) << " ";
        }
        std::cout << "\n";

        // 6. Input do Usuário
        std::cout << "\n(" << (board.whiteToMove ? "White" : "Black") << ") Enter move: ";
        std::string input;
        std::cin >> input;

        if (input == "quit" || input == "exit") break;

        // 7. Validação
        // Procura se o texto digitado bate com algum movimento gerado
        bool found = false;
        Move selectedMove;

        for (const auto& m : legalMoves) {
            if (moveToString(m) == input) {
                selectedMove = m;
                found = true;
                break;
            }
        }

        if (found) {
            // 8. Aplica o movimento
            board = board.applyMove(selectedMove);
            std::cout << "Move aplicado: " << input << "\n";
        } else {
            std::cout << ">> ERRO: Movimento invalido ou ilegal! Tente novamente.\n";
        }
    }

    return 0;
}
