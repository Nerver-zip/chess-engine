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
}

#endif  // DEBUG
