#pragma once
#include "raylib.h"
#include "raymath.h"
#include "../board/board.h"
#include "../move/move.h"
#include <vector>
#include <list> 

// Estrutura para controlar a animação
struct MovingPiece {
    Piece piece;
    int toSq;
    Vector2 currentPos;
    Vector2 targetPos;
    bool finished;
};

class ChessGUI {
public:
    ChessGUI();
    ~ChessGUI();

    void run();

private:
    Board board;
    std::vector<Move> legalMoves;
    Texture2D pieceTextures;
    
    // Controle de Drag & Drop
    bool isDragging = false;
    int sourceSquare = -1;
    int targetSquare = -1;
    Vector2 dragPos = {0, 0};

    // Controle de Animação
    std::list<MovingPiece> animations;
    float smoothness = 0.5f; // 0.01 (lento) a 1.0 (instantâneo). 0.2 é bom.

    // Helpers Dinâmicos
    float getSquareSize();
    float getBoardSize();
    int getSquareFromMouse();
    Rectangle getPieceRect(Piece p);
    Vector2 getSquarePos(int sq); //índice -> pixels (x,y)

    // Lógica
    void loadAssets();
    void updateLogic(); 
    void draw();        

    void makeEngineMove();
    
    // Função unificada para aplicar move com animação
    void performMove(Move m);
};
