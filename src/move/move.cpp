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
    std::string san;
    san.reserve(8);

    // Castling
    if (move.flags & KING_CASTLE)  return "O-O";
    if (move.flags & QUEEN_CASTLE) return "O-O-O";

    Piece p = boardState.pieceAt(move.from);
    bool isPawn = (p == WPAWN || p == BPAWN);

    // Letra da peça
    char pieceChar = 0;
    if (p == WKING || p == BKING)       pieceChar = 'K';
    else if (p == WQUEEN || p == BQUEEN)pieceChar = 'Q';
    else if (p == WROOK || p == BROOK)  pieceChar = 'R';
    else if (p == WBISHOP || p == BBISHOP) pieceChar = 'B';
    else if (p == WKNIGHT || p == BKNIGHT) pieceChar = 'N';

    if (!isPawn)
        san.push_back(pieceChar);

    // -----------------------------
    // Desambiguação (exceto rei)
    // -----------------------------
    if (!isPawn && pieceChar != 'K') {
        bool ambiguityFile = false;
        bool ambiguityRank = false;
        bool needsDisambiguation = false;

        std::vector<Move> moves = MoveGen::generatePieceMoves(boardState, p);

        for (const auto& otherM : moves) {
            if (otherM.from != move.from && otherM.to == move.to) {
                if (boardState.pieceAt(otherM.from) == p) {
                    needsDisambiguation = true;

                    if ((move.from % 8) == (otherM.from % 8))
                        ambiguityFile = true;
                    else
                        ambiguityRank = true;
                }
            }
        }

        if (needsDisambiguation) {
            static constexpr char files[] = "abcdefgh";
            static constexpr char ranks[] = "12345678";

            if (!ambiguityFile)
                san.push_back(files[move.from % 8]);
            else if (!ambiguityRank)
                san.push_back(ranks[move.from / 8]);
            else {
                san.push_back(files[move.from % 8]);
                san.push_back(ranks[move.from / 8]);
            }
        }
    }

    // -----------------------------
    // Captura
    // -----------------------------
    bool isCapture =
        (boardState.pieceAt(move.to) != EMPTY) ||
        (move.flags & EN_PASSANT);

    if (isCapture) {
        if (isPawn)
            san.push_back("abcdefgh"[move.from % 8]);
        san.push_back('x');
    }

    // -----------------------------
    // Casa de destino
    // -----------------------------
    san.push_back("abcdefgh"[move.to % 8]);
    san.push_back("12345678"[move.to / 8]);

    // -----------------------------
    // Promoção
    // -----------------------------
    if (move.flags & PROMOTION) {
        san.push_back('=');
        switch (move.promotion) {
            case WQUEEN:
            case BQUEEN:  san.push_back('Q'); break;
            case WROOK:
            case BROOK:   san.push_back('R'); break;
            case WBISHOP:
            case BBISHOP: san.push_back('B'); break;
            case WKNIGHT:
            case BKNIGHT: san.push_back('N'); break;
        }
    }

    // -----------------------------
    // Xeque ou mate
    // -----------------------------
    Board nextBoard = boardState.applyMove(move);
    nextBoard.updateAttackBoards();

    bool enemyWhite = !boardState.whiteToMove;
    uint64_t kingBB = enemyWhite ? nextBoard.whiteKing : nextBoard.blackKing;

    bool inCheck = enemyWhite
        ? (nextBoard.blackAttacks & kingBB)
        : (nextBoard.whiteAttacks & kingBB);

    if (inCheck) {
        std::vector<Move> responses = MoveGen::generateCheckResponses(nextBoard);
        san.push_back(responses.empty() ? '#' : '+');
    }

    return san;
}
