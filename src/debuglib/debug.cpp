#include "debug.h"
#include <iostream>
#include <iomanip>

#ifdef DEBUG 

// Cores ANSI
#define RST  "\033[0m"
#define CYN  "\033[36m"   // Bordas
#define GRN  "\033[32m"   // Bits ativos ou White pieces
#define RED  "\033[31m"   // Black pieces
#define YEL  "\033[33m"   // Info
#define BLD  "\033[1m"    // Negrito

namespace Debug {

    void printBitboard(uint64_t bb, const std::string& title) {
        std::cout << "\n" << BLD << "=== " << title << " ===" << RST << "\n";
        
        for (int rank = 7; rank >= 0; --rank) {
            std::cout << CYN << rank + 1 << "  " << RST; // Número do rank
            for (int file = 0; file < 8; ++file) {
                int sq = rank * 8 + file;
                
                if ((bb >> sq) & 1) {
                    std::cout << GRN << "1 " << RST;
                } else {
                    std::cout << ". ";
                }
            }
            std::cout << "\n";
        }
        std::cout << CYN << "   a b c d e f g h" << RST << "\n\n";
        std::cout << "   Value: 0x" << std::hex << bb << std::dec << "\n";
    }

    char getPieceChar(Piece p) {
        switch (p) {
            case WPAWN:   return 'P';
            case WKNIGHT: return 'N';
            case WBISHOP: return 'B';
            case WROOK:   return 'R';
            case WQUEEN:  return 'Q';
            case WKING:   return 'K';
            case BPAWN:   return 'p';
            case BKNIGHT: return 'n';
            case BBISHOP: return 'b';
            case BROOK:   return 'r';
            case BQUEEN:  return 'q';
            case BKING:   return 'k';
            default:      return '.';
        }
    }

    void printBoard(const Board& board) {
        std::cout << "\n" << BLD << "=== Game State ===" << RST << "\n";

        for (int rank = 7; rank >= 0; --rank) {
            std::cout << CYN << rank + 1 << "  " << RST;
            for (int file = 0; file < 8; ++file) {
                int sq = rank * 8 + file;
                Piece p = board.pieceAt(sq);
                char c = getPieceChar(p);

                if (p == EMPTY) {
                    std::cout << ". ";
                } else if (p >= WPAWN && p <= WKING) {
                    // Peças brancas em Verde ou Negrito
                    std::cout << GRN << c << " " << RST; 
                } else {
                    // Peças pretas em Vermelho
                    std::cout << RED << c << " " << RST;
                }
            }
            std::cout << "\n";
        }
        std::cout << CYN << "   a b c d e f g h" << RST << "\n";

        // Informações adicionais
        std::cout << "   Side to move: " << (board.whiteToMove ? "White" : "Black") << "\n";
        
        std::cout << "   Castling: ";
        if (board.castlingRights & 1) std::cout << "K"; else std::cout << "-";
        if (board.castlingRights & 2) std::cout << "Q"; else std::cout << "-";
        if (board.castlingRights & 4) std::cout << "k"; else std::cout << "-";
        if (board.castlingRights & 8) std::cout << "q"; else std::cout << "-";
        std::cout << "\n";

        std::cout << "   En Passant: ";
        if (board.enPassantSquare != -1) {
            int f = board.enPassantSquare % 8;
            int r = board.enPassantSquare / 8;
            char file = 'a' + f;
            char rank = '1' + r;
            std::cout << file << rank;
        } else {
            std::cout << "None";
        }
        std::cout << "\n";
    }

    void printAttackMaps(const Board& board) {
        std::cout << "\n" << BLD << "=== Attack Maps ===" << RST << "\n";
        
        std::cout << YEL << ">> White Attacks:" << RST << "\n";
        for (int rank = 7; rank >= 0; --rank) {
            std::cout << CYN << rank + 1 << "  " << RST;
            for (int file = 0; file < 8; ++file) {
                int sq = rank * 8 + file;
                if ((board.whiteAttacks >> sq) & 1) std::cout << GRN << "x " << RST;
                else std::cout << ". ";
            }
            std::cout << "\n";
        }
        std::cout << CYN << "   a b c d e f g h" << RST << "\n";

        std::cout << "\n" << YEL << ">> Black Attacks:" << RST << "\n";
        for (int rank = 7; rank >= 0; --rank) {
            std::cout << CYN << rank + 1 << "  " << RST;
            for (int file = 0; file < 8; ++file) {
                int sq = rank * 8 + file;
                if ((board.blackAttacks >> sq) & 1) std::cout << RED << "x " << RST;
                else std::cout << ". ";
            }
            std::cout << "\n";
        }
        std::cout << CYN << "   a b c d e f g h" << RST << "\n";
    }

    void printMove(const Move& m) {
        std::string files = "abcdefgh";
        std::string ranks = "12345678";
        
        std::cout << files[m.from % 8] << ranks[m.from / 8]
                  << files[m.to % 8]   << ranks[m.to / 8];
                  
        if (m.flags & CAPTURE) std::cout << " (capture)";
        if (m.flags & KING_CASTLE) std::cout << " (O-O)";
        if (m.flags & QUEEN_CASTLE) std::cout << " (O-O-O)";
        std::cout << "\n";
    }

    // Helper interno para converter casa (0-63) em notação algébrica (e.g., e4)
    std::string sqToStr(int sq) {
        if (sq < 0 || sq > 63) return "--";
        char f = 'a' + (sq % 8);
        char r = '1' + (sq / 8);
        return {f, r};
    }

    std::string moveDebugString(const Move& m) {
        // Se o movimento for nulo/vazio
        if (m.from == 0 && m.to == 0) return "(null)";

        std::string s = sqToStr(m.from) + sqToStr(m.to);

        // Adiciona sufixo de promoção se houver
        if (m.flags & PROMOTION) {
            switch (m.promotion) {
                case WQUEEN: case BQUEEN: s += 'q'; break;
                case WROOK:  case BROOK:  s += 'r'; break;
                case WBISHOP:case BBISHOP:s += 'b'; break;
                case WKNIGHT:case BKNIGHT:s += 'n'; break;
            }
        }

        // Formatação rica para debug: "e2e4 [Score: 9000] (CAP)"
        std::string flags = "";
        if (m.flags & CAPTURE) flags += "C";
        if (m.flags & PROMOTION) flags += "P";
        if (m.flags & EN_PASSANT) flags += "E";
        if (m.flags & KING_CASTLE) flags += "K";
        if (m.flags & QUEEN_CASTLE) flags += "Q";
        
        std::string debugStr = s + " [" + std::to_string(m.score) + "]";
        if (!flags.empty()) debugStr += "(" + flags + ")";

        return debugStr;
    }

    void printMoveList(const std::vector<Move>& moves, const std::string& title) {
        Debug::cout << "=== " << title << " (" << moves.size() << ") ===\n";
        
        if (moves.empty()) {
            Debug::cout << "  (empty)\n";
            return;
        }

        // Imprime em colunas ou linha a linha
        int idx = 0;
        for (const auto& m : moves) {
            Debug::cout << std::setw(2) << idx++ << ": " 
                        << std::left << std::setw(20) << moveDebugString(m);
            
            // Quebra linha a cada 3 movimentos para não poluir
            if (idx % 3 == 0) Debug::cout << "\n";
        }
        Debug::cout << "\n==========================\n";
    }

    void printKillerTable(const Move killerTable[][2], int maxPly) {
        Debug::cout << "=== Killer Heuristic Table ===\n";
        bool empty = true;

        for (int ply = 0; ply < maxPly; ++ply) {
            const Move& k1 = killerTable[ply][0];
            const Move& k2 = killerTable[ply][1];

            // Só imprime se houver pelo menos um killer move salvo nesse ply
            bool hasK1 = !(k1.from == 0 && k1.to == 0);
            bool hasK2 = !(k2.from == 0 && k2.to == 0);

            if (hasK1 || hasK2) {
                empty = false;
                Debug::cout << "Ply " << std::setw(2) << ply << ": ";
                
                if (hasK1) Debug::cout << "1st: " << moveDebugString(k1) << "   ";
                else       Debug::cout << "1st: --              ";

                if (hasK2) Debug::cout << "2nd: " << moveDebugString(k2);
                else       Debug::cout << "2nd: --";

                Debug::cout << "\n";
            }
        }

        if (empty) Debug::cout << "  (Table is empty)\n";
        Debug::cout << "==============================\n";
    }
}

#endif  // DEBUG
