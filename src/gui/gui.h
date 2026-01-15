#pragma once
#include "raylib.h"
#include "raymath.h"
#include "../board/board.h"
#include "../move/move.h"
#include <fstream>
#include <vector>
#include <list>
#include <string>
#include <thread>
#include <atomic>

enum AppState {
    STATE_MENU_MAIN,
    STATE_MENU_MODE,
    STATE_MENU_SIDE,
    STATE_GAME
};

// Estrutura para controlar a animação
struct MovingPiece {
    Piece piece;
    int toSq;
    Vector2 currentPos;
    Vector2 targetPos;
    bool finished;
};

// Estrutura para dados do jogador
struct PlayerInfo {
    std::string name;
    std::string rating;
    Texture2D avatar; // PFP
};

struct GameHistory {
    int moveNumber;
    std::string whiteMove;
    std::string blackMove;
};

class ChessGUI {
public:
    ChessGUI();
    ~ChessGUI();
    void run();

private:

    Texture2D enginePfp;       // A capivara
    Texture2D userPfp;         // A foto da pasta player/
    std::string userNameStr;   // Nome lido do txt
    std::string userRatingStr; // Rating lido do txt

    // Helper para ler o arquivo de config
    void loadPlayerConfig();

    // Estado Atual
    AppState currentState = STATE_MENU_MAIN;
    bool shouldClose = false;

    // Core
    Board board;
    std::vector<Move> legalMoves;
    Texture2D pieceTextures;
    Texture2D defaultAvatar;
    
    // UI Helpers
    Move lastMove = {};
    bool isFlipped = false;
    bool userIsWhite = true;

    // Layout
    Rectangle boardRect;
    Rectangle leftPanelRect;
    Rectangle rightPanelRect;

    PlayerInfo whitePlayer;
    PlayerInfo blackPlayer;

    // Drag & Drop
    bool isDragging = false;
    int sourceSquare = -1;
    int targetSquare = -1;
    Vector2 dragPos = {0, 0};

    // Animação
    std::list<MovingPiece> animations;
    float smoothness = 0.3f;

    // Métodos Internos
    void calculateLayout();
    float getSquareSize();
    int getSquareFromMouse();
    Rectangle getPieceRect(Piece p);
    Vector2 getSquarePos(int sq);

    void loadAssets();
    void resetGame();
    
    // Controle de thread da engine
    std::atomic<bool> isEngineThinking = false;
    bool engineMoveReady = false; 
    Move computedMove = {};
    void startEngineThink();

    // Logic Steps
    void updateLogic(); 
    void updateMenuLogic();

    // Draw Steps
    void draw();        
    void drawBoard();
    void drawPieces();
    void drawPanels();
    void drawHighlights();
    
    // Menu Drawers
    void drawMenu();
    bool drawButton(Rectangle rect, const char* text);

    // Engine
    void performMove(Move m);

    std::vector<GameHistory> history;

    // Controle de histórico e navegação
    std::vector<Board> stateHistory; // Snapshot do tabuleiro a cada lance
    std::vector<Move> flatMoveHistory; 
    void setVisualPly(int targetPly);
    int viewPly;
    float scrollOffset = 0.0f;

    // Helpers de Navegação
    void goToFirstMove();
    void goToPreviousMove();
    void goToNextMove();
    void goToLastMove();
    
    // Retorna o tabuleiro que deve ser desenhado (pode ser passado ou presente)
    const Board& getDisplayBoard(); 

    // Helper de UI para desenhar ícones de controle
    void drawControlButtons(float x, float y, float w);
};
