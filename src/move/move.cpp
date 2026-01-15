#include "move.h"
#include "../board/board.h"
#include "movegen.h"
#include <sstream>
#include <vector>

std::string moveToUCI(const Move& m) {
    std::stringstream ss;
    char files[] = "abcdefgh";
    char ranks[] = "12345678";

    ss << files[m.from % 8] << ranks[m.from / 8];
    ss << files[m.to % 8] << ranks[m.to / 8];

    if (m.flags & PROMOTION) {
        switch (m.promotion) {
            case WQUEEN: case BQUEEN: ss << "q"; break;
            case WROOK: case BROOK: ss << "r"; break;
            case WBISHOP: case BBISHOP: ss << "b"; break;
            case WKNIGHT: case BKNIGHT: ss << "n"; break;
        }
    }
    return ss.str();
}

std::string moveToSAN(const Move& move, const Board& boardState) {
    std::stringstream ss;
    
    if (move.flags & KING_CASTLE) return "O-O";
    if (move.flags & QUEEN_CASTLE) return "O-O-O";

    Piece p = boardState.pieceAt(move.from);
    
    char pieceChar = ' ';
    bool isPawn = (p == WPAWN || p == BPAWN);
    
    if (p == WKING || p == BKING) pieceChar = 'K';
    else if (p == WQUEEN || p == BQUEEN) pieceChar = 'Q';
    else if (p == WROOK || p == BROOK) pieceChar = 'R';
    else if (p == WBISHOP || p == BBISHOP) pieceChar = 'B';
    else if (p == WKNIGHT || p == BKNIGHT) pieceChar = 'N';

    // Desambiguação caso duas peças do mesmo tipo possam ir para a mesma casa
    if (!isPawn && pieceChar != 'K') {
        bool ambiguityFile = false;
        bool ambiguityRank = false;
        bool needsDisambiguation = false;

        std::vector<Move> moves = MoveGen::generatePieceMoves(boardState, p);

        for (const auto& otherM : moves) {
            if (otherM.from != move.from && otherM.to == move.to) {
                Piece otherP = boardState.pieceAt(otherM.from);
                if (otherP == p) {
                    needsDisambiguation = true;
                    if ((move.from % 8) == (otherM.from % 8)) ambiguityFile = true;
                    else ambiguityRank = true;
                }
            }
        }

        if (needsDisambiguation) {
            char files[] = "abcdefgh";
            char ranks[] = "12345678";
            
            if (ambiguityFile) {
                if (ambiguityRank) ss << files[move.from % 8];
                ss << ranks[move.from / 8];
            } else {
                ss << files[move.from % 8];
            }
        }
    }

    if (!isPawn) ss << pieceChar;

    // Na captura se for peão usar a file, adicionar 'x'
    bool isCapture = (boardState.pieceAt(move.to) != EMPTY) || (move.flags & EN_PASSANT);
    if (isCapture) {
        if (isPawn) {
            char files[] = "abcdefgh";
            ss << files[move.from % 8];
        }
        ss << "x";
    }

    // Destino
    char files[] = "abcdefgh";
    char ranks[] = "12345678";
    ss << files[move.to % 8] << ranks[move.to / 8];

    // Promoção, concatenar =PEÇA 
    if (move.flags & PROMOTION) {
        ss << "=";
        switch (move.promotion) {
            case WQUEEN: case BQUEEN: ss << "Q"; break;
            case WROOK: case BROOK: ss << "R"; break;
            case WBISHOP: case BBISHOP: ss << "B"; break;
            case WKNIGHT: case BKNIGHT: ss << "N"; break;
        }
    }

    // Xeque (+) ou Mate (#)
    Board nextBoard = boardState.applyMove(move);
    nextBoard.updateAttackBoards();
    
    // Detecta se o Rei oponente está em cheque
    bool enemyWhite = !boardState.whiteToMove;
    uint64_t kingBB = enemyWhite ? nextBoard.whiteKing : nextBoard.blackKing;
    bool inCheck = enemyWhite ? (nextBoard.blackAttacks & kingBB) : (nextBoard.whiteAttacks & kingBB);

    if (inCheck) {
        std::vector<Move> responses = MoveGen::generateCheckResponses(nextBoard);
        if (responses.empty()) ss << "#";
        else ss << "+";
    }

    return ss.str();
}
