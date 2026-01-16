#include "gui.h"
#include "../move/movegen.h"
#include "../search/search.h"
#include "../debuglib/debug.h"
#include "../zobrist/zobrist.h"
#include "../tt/tt.h"
#include <algorithm>
#include <string>
#include <sstream>

const Color LIGHT_SQUARE = {235, 236, 208, 255};
const Color DARK_SQUARE  = {119, 149, 86, 255};
const Color HIGHLIGHT_MOVE = {255, 255, 0, 100};
const Color HIGHLIGHT_DRAG = {20, 85, 30, 128};
const Color BG_COLOR = {48, 46, 43, 255};
const Color BUTTON_COLOR = {65, 63, 60, 255};
const Color BUTTON_HOVER = {85, 83, 80, 255};

ChessGUI::ChessGUI() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT); 
    InitWindow(1500, 800, "Chess Engine");
    SetTargetFPS(60);
    
    Zobrist::init();
    
    // TT de 64MB
    TT.resize(64);
    board = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board.updateAttackBoards();
    legalMoves = MoveGen::generateMoves(board);
    
    loadAssets();

    // Inicia direto no reset (mas o estado inicial é MENU no header)
    resetGame();
    Debug::printBoard(board);
}

ChessGUI::~ChessGUI() {
    UnloadTexture(pieceTextures);
    UnloadTexture(enginePfp);
    UnloadTexture(userPfp);
    CloseWindow();
}

void ChessGUI::loadAssets() {
    pieceTextures = LoadTexture("assets/pieces.png"); 
    SetTextureFilter(pieceTextures, TEXTURE_FILTER_BILINEAR);

    if (FileExists("assets/capy.png")) {
        enginePfp = LoadTexture("assets/capy.png");
    } else {
        Image img = GenImageColor(128, 128, PURPLE);
        enginePfp = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    loadPlayerConfig();
}

void ChessGUI::loadPlayerConfig() {
    userNameStr = "Player";
    userRatingStr = "????";
    
    Image defaultImg = GenImageColor(128, 128, DARKGRAY);
    userPfp = LoadTextureFromImage(defaultImg);
    UnloadImage(defaultImg);

    std::ifstream file("player/player_info.txt");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();

            if (line.find("DISPLAY_NAME=") == 0) {
                userNameStr = line.substr(13);
            }
            else if (line.find("RATING=") == 0) {
                userRatingStr = line.substr(7);
            }
        }
        file.close();
    }

    if (DirectoryExists("player")) {
        FilePathList files = LoadDirectoryFiles("player");
        
        for (unsigned int i = 0; i < files.count; i++) {
            if (IsFileExtension(files.paths[i], ".png")) {
                UnloadTexture(userPfp);
                userPfp = LoadTexture(files.paths[i]);
                break;
            }
        }
        UnloadDirectoryFiles(files);
    }
}

void ChessGUI::resetGame() {
    board = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board.updateAttackBoards();
    legalMoves = MoveGen::generateMoves(board);
    
    animations.clear();
    history.clear();
    stateHistory.clear();
    flatMoveHistory.clear();
    isGameOver = false;
    showGameOverPopup = false;
    gameResult = RESULT_NONE;
    gameReason = REASON_NONE;
    timerActive = false;
    gameOverTimer = 0.0f;
    fiftyMoveCounter = 0;
    positionHistory.clear();
    

    stateHistory.push_back(board);
    positionHistory[generateFEN()]++;
    viewPly = 0;
    scrollOffset = 0;
    lastMove = {};
    isDragging = false;
    sourceSquare = -1;
    
    // Reseta flags da thread
    isEngineThinking = false;
    engineMoveReady = false;

    // Configura os Players usando os dados carregados em loadAssets
    if (userIsWhite) {
        whitePlayer = {userNameStr, userRatingStr, userPfp};
        blackPlayer = {"Capy Engine", "????", enginePfp};
        
        isFlipped = false;
    } else {
        whitePlayer = {"Capy Engine", "????", enginePfp};
        blackPlayer = {userNameStr, userRatingStr, userPfp};
        
        isFlipped = true;
    }
}

// =========================================================
//  Helpers de Dimensão e Posição
// =========================================================

void ChessGUI::calculateLayout() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // Define larguras relativas
    float boardDim = std::min(sw, sh) * 0.90f; // Tabuleiro ocupa 90% da menor dimensão
    float sidePanelWidth = (sw - boardDim) / 2.0f;

    // Centraliza o tabuleiro
    boardRect = {
        (sw - boardDim) / 2.0f,
        (sh - boardDim) / 2.0f,
        boardDim,
        boardDim
    };

    // Painel Esquerdo (Info Jogadores)
    if (sidePanelWidth > 100) {
        leftPanelRect = { 10, boardRect.y, sidePanelWidth - 20, boardRect.height };
        rightPanelRect = { boardRect.x + boardRect.width + 10, boardRect.y, sidePanelWidth - 20, boardRect.height };
    } else {
        // Fallback se a janela for muito estreita
        leftPanelRect = {0,0,0,0};
        rightPanelRect = {0,0,0,0};
    }
}

float ChessGUI::getSquareSize() {
    return boardRect.width / 8.0f;
}

// Converte Índice (0..63) -> Pixels (considerando Board Flip e Offset)
Vector2 ChessGUI::getSquarePos(int sq) {
    float sz = getSquareSize();
    int f = sq % 8;
    int r = 7 - (sq / 8); // Padrão: Rank 0 embaixo

    // Se estiver flipado (Pretas embaixo):
    if (isFlipped) {
        f = 7 - f;
        r = 7 - r;
    }

    return Vector2 { 
        boardRect.x + f * sz, 
        boardRect.y + r * sz 
    };
}

int ChessGUI::getSquareFromMouse() {
    Vector2 m = GetMousePosition();
    
    // Verifica colisão com o retangulo do tabuleiro
    if (!CheckCollisionPointRec(m, boardRect)) return -1;

    float sz = getSquareSize();
    // Coordenada relativa ao inicio do tabuleiro
    float rx = m.x - boardRect.x;
    float ry = m.y - boardRect.y;

    int file = (int)(rx / sz);
    int rank = 7 - (int)(ry / sz); // Inverte Y padrão

    // Se estiver flipado, inverte o resultado
    if (isFlipped) {
        file = 7 - file;
        rank = 7 - rank;
    }

    if (file < 0 || file > 7 || rank < 0 || rank > 7) return -1;
    return rank * 8 + file;
}

Rectangle ChessGUI::getPieceRect(Piece p) {
    int index = 0; int row = 0;
    
    // Define linha (0=Brancas, 1=Pretas)
    if (p >= WPAWN && p <= WKING) row = 0; else row = 1;

    // Define coluna baseado na ordem do Sprite (K, Q, B, N, R, P)
    switch (p) {
        case WKING:   case BKING:   index = 0; break;
        case WQUEEN:  case BQUEEN:  index = 1; break;
        case WBISHOP: case BBISHOP: index = 2; break;
        case WKNIGHT: case BKNIGHT: index = 3; break;
        case WROOK:   case BROOK:   index = 4; break;
        case WPAWN:   case BPAWN:   index = 5; break;
        default: break;
    }

    float w = pieceTextures.width / 6.0f;
    float h = pieceTextures.height / 2.0f;
    
    return Rectangle{index * w, row * h, w, h};
}

// =========================================================
//  Lógica Principal (Loop e Updates)
// =========================================================

void ChessGUI::run() {
    while (!WindowShouldClose() && !shouldClose) {
        if (currentState == STATE_GAME) {
            updateLogic();
        } else {
            updateMenuLogic();
        }
        draw();
    }
}

void ChessGUI::startEngineThink() {
    if (isEngineThinking) return;

    isEngineThinking = true;

    std::thread searchThread([this]() {
        
        Move best = Search::searchBestMove(board, 6);
        
        this->computedMove = best;
        this->engineMoveReady = true;     // Avisa a main thread
        this->isEngineThinking = false;   // Libera a flag
    });

    searchThread.detach();
}

void ChessGUI::updateLogic() {
    calculateLayout(); 
    
    if (timerActive) {
        gameOverTimer += GetFrameTime();
        if (gameOverTimer > 1.0f) {
            showGameOverPopup = true;
            timerActive = false;
        }
    }

    // Navegação (Sempre disponível)
    if (IsKeyPressed(KEY_ESCAPE)) currentState = STATE_MENU_MAIN;
    if (IsKeyPressed(KEY_F)) isFlipped = !isFlipped;
    if (IsKeyPressed(KEY_LEFT))  goToPreviousMove();
    if (IsKeyPressed(KEY_RIGHT)) goToNextMove();
    if (IsKeyPressed(KEY_UP))    goToFirstMove();
    if (IsKeyPressed(KEY_DOWN))  goToLastMove();

    // Atualiza Animações
    for (auto& anim : animations) {
        anim.currentPos = Vector2Lerp(anim.currentPos, anim.targetPos, smoothness);
        if (Vector2Distance(anim.currentPos, anim.targetPos) < 0.5f) {
            anim.currentPos = anim.targetPos;
            anim.finished = true;
        }
    }
    animations.remove_if([](const MovingPiece& m) { return m.finished; });

    // Verifica se estamos no "Presente"
    bool isLive = (viewPly == (int)stateHistory.size() - 1);
    
    // Engine Thinking
    bool isEngineTurn = (board.whiteToMove != userIsWhite);

    if (isEngineTurn) {
        if (!isEngineThinking && !engineMoveReady && animations.empty()) {
             startEngineThink();
        }
    }

    if (!isLive || isGameOver) return; 

    // =========================================================
    // Daqui pra baixo, só executa se estiver jogando (Live)
    // =========================================================

    // Aplica movimento da engine se estiver pronto
    if (engineMoveReady) {
        engineMoveReady = false;
        if (computedMove.from != computedMove.to) {
            performMove(computedMove);
        }
    }

    // Input Humano (Drag & Drop)
    int mouseSq = getSquareFromMouse();
    targetSquare = mouseSq;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (mouseSq != -1) {
            Piece p = board.pieceAt(mouseSq);
            
            // Só é permitido arrastar peça da cor do jogador
            bool isWhitePiece = (p >= WPAWN && p <= WKING);
            bool belongsToUser = (userIsWhite && isWhitePiece) || (!userIsWhite && !isWhitePiece);

            if (p != EMPTY && belongsToUser) {
                isDragging = true;
                sourceSquare = mouseSq;
            }
        }
    }

    if (isDragging) dragPos = GetMousePosition();

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && isDragging) {
        isDragging = false;

        // Validação: Só move se não for a vez da engine e ela não estiver pensando
        bool canMove = !isEngineTurn && !isEngineThinking && !engineMoveReady;

        if (canMove && mouseSq != -1 && mouseSq != sourceSquare) {
            Move move = {};
            bool found = false;

            for (const auto& m : legalMoves) {
                if (m.from == sourceSquare && m.to == mouseSq) {
                    move = m;
                    found = true;
                    break;
                }
            }

            if (found) {
                if (move.flags & PROMOTION && move.promotion == EMPTY) move.promotion = WQUEEN;
                performMove(move);
            }
        }
        sourceSquare = -1;
    }
}
void ChessGUI::setVisualPly(int targetPly) {
    if (targetPly < 0) targetPly = 0;
    if (targetPly >= (int)stateHistory.size()) targetPly = stateHistory.size() - 1;

    if (targetPly == viewPly) return;
    
    // Limpa animações anteriores imediatamente
    animations.clear();

    // Lógica de Animação (Apenas para passos simples de 1 em 1)
    if (abs(targetPly - viewPly) == 1) {
        
        bool isUndo = (targetPly < viewPly);
        Move move = {};
        
        int moveIdx = isUndo ? (viewPly - 1) : (targetPly - 1);
        
        if (moveIdx >= 0 && moveIdx < (int)flatMoveHistory.size()) {
            move = flatMoveHistory[moveIdx];
            
            MovingPiece anim;
            
            // Qual tabuleiro olhar para pegar o sprite da peça?
            // Sempre olhamos para o tabuleiro onde a peça ESTÁ antes de começar a mover.
            
            if (isUndo) {
                // VOLTA (Undo): Estamos no futuro (Ply K), voltando para passado (Ply K-1).
                // A peça está no destino (move.to).
                // O estado visual atual (viewPly) contém a peça em 'to'.
                const Board& currentBoard = stateHistory[viewPly];
                
                anim.piece = currentBoard.pieceAt(move.to); 
                anim.currentPos = getSquarePos(move.to);
                anim.targetPos  = getSquarePos(move.from);
                anim.toSq = move.from;
            } 
            else {
                // IDA (Redo): Estamos no passado (Ply K), indo para futuro (Ply K+1).
                // A peça está na origem (move.from).
                // O estado visual atual (viewPly) contém a peça em 'from'.
                const Board& currentBoard = stateHistory[viewPly];

                anim.piece = currentBoard.pieceAt(move.from);
                anim.currentPos = getSquarePos(move.from);
                anim.targetPos  = getSquarePos(move.to);
                anim.toSq = move.to;
            }
            
            anim.finished = false;
            animations.push_back(anim);
        }
    }

    // Aplica a mudança de estado FINALMENTE
    viewPly = targetPly;
}

// =========================================================
//  Execução de Movimentos e Animação
// =========================================================

void ChessGUI::performMove(Move m) {
    
    // Atualiza Regra 50 lances  
    
    Piece p = board.pieceAt(m.from);
    
    bool isPawn = (p == WPAWN || p == BPAWN);
    bool isCapture = m.flags & CAPTURE || m.flags & EN_PASSANT;

    if (isPawn || isCapture) {
        fiftyMoveCounter = 0;
    } else {
        fiftyMoveCounter++;
    }

    // Gera notação e Atualiza Histórico de Texto
    std::string san = moveToSAN(m, board);

    if (board.whiteToMove) {
        GameHistory entry;
        entry.moveNumber = history.size() + 1;
        entry.whiteMove = san;
        history.push_back(entry);
    } else {
        if (!history.empty()) history.back().blackMove = san;
    }
    
    // Salva movimento bruto para animação reversa
    flatMoveHistory.push_back(m);

    // Configura Animação Visual
    if (m.flags & PROMOTION) p = (Piece)m.promotion;

    Vector2 startPos = getSquarePos(m.from);
    Vector2 endPos   = getSquarePos(m.to);

    MovingPiece anim;
    anim.piece = p;
    anim.toSq = m.to; 
    anim.currentPos = startPos;
    anim.targetPos = endPos;
    anim.finished = false;
    animations.push_back(anim);
    
    // Animação da Torre (Roque)
    if (m.flags & KING_CASTLE || m.flags & QUEEN_CASTLE) {
        bool kingside = (m.flags & KING_CASTLE);
        // Lógica para achar a torre correta
        int rFrom = kingside ? (board.whiteToMove ? 7 : 63) : (board.whiteToMove ? 0 : 56);
        int rTo   = kingside ? (board.whiteToMove ? 5 : 61) : (board.whiteToMove ? 3 : 59);
        Piece rook = board.whiteToMove ? WROOK : BROOK;

        MovingPiece rookAnim;
        rookAnim.piece = rook;
        rookAnim.toSq = rTo;
        rookAnim.currentPos = getSquarePos(rFrom);
        rookAnim.targetPos = getSquarePos(rTo);
        rookAnim.finished = false;
        animations.push_back(rookAnim);
    }

    // Aplica Lógica no Tabuleiro
    board = board.applyMove(m);
    board.updateAttackBoards();
    legalMoves = MoveGen::generateMoves(board);

    // Salva Snapshot no Histórico
    stateHistory.push_back(board);
    
    // Atualiza visualização para o novo estado
    setVisualPly(stateHistory.size() - 1);

    lastMove = m;
     
    Debug::printBoard(board); 
    
    if (!isGameOver) {
        checkGameOver();
    }
}

// =========================================================
//  Renderização (Draw)
// =========================================================

void ChessGUI::drawPanels() {
    // Se a janela for muito estreita, não desenha
    if (leftPanelRect.width <= 0) return;

    // --- Definição Dinâmica de Quem é Quem ---
    PlayerInfo* topPlayer = nullptr;
    PlayerInfo* bottomPlayer = nullptr;

    if (!isFlipped) {
        bottomPlayer = &whitePlayer;
        topPlayer    = &blackPlayer;
    } else {
        bottomPlayer = &blackPlayer;
        topPlayer    = &whitePlayer;
    }

    // Configuração de Tamanho
    float padding = 20.0f;
    // O avatar será 128px ou o tamanho do painel menos padding (o que for menor)
    float avatarSize = std::min(leftPanelRect.width - (padding * 2), 128.0f);
    
    // Onde começa o texto (Logo depois da foto + um espacinho)
    float textX = leftPanelRect.x + padding + avatarSize + 15.0f;

    // ---------------------------------------------------------
    // Painel Esquerdo
    // ---------------------------------------------------------
    DrawRectangleRec(leftPanelRect, Fade(BLACK, 0.2f));

    // --- Jogador do topo (Oponente) ---
    if (topPlayer) {
        float topY = leftPanelRect.y + padding;

        // Avatar
        DrawTexturePro(topPlayer->avatar, 
            {0, 0, (float)topPlayer->avatar.width, (float)topPlayer->avatar.height}, 
            {leftPanelRect.x + padding, topY, avatarSize, avatarSize}, 
            {0,0}, 0, WHITE);
            
        // Texto (Alinhado com o topo do avatar ou levemente descido)
        DrawText(topPlayer->name.c_str(), textX, topY + 10, 24, WHITE); // Fonte maior (24)
        DrawText(topPlayer->rating.c_str(), textX, topY + 40, 20, LIGHTGRAY); // Fonte maior (20)
    }

    // --- Jogador da base (Player) ---
    if (bottomPlayer) {
        // Calcula Y de baixo para cima
        float bottomY = leftPanelRect.y + leftPanelRect.height - avatarSize - padding;
        
        // Avatar
        DrawTexturePro(bottomPlayer->avatar, 
            {0, 0, (float)bottomPlayer->avatar.width, (float)bottomPlayer->avatar.height}, 
            {leftPanelRect.x + padding, bottomY, avatarSize, avatarSize}, 
            {0,0}, 0, WHITE);
            
        // Texto
        DrawText(bottomPlayer->name.c_str(), textX, bottomY + 10, 24, WHITE);
        DrawText(bottomPlayer->rating.c_str(), textX, bottomY + 40, 20, LIGHTGRAY);
    }

    // ---------------------------------------------------------
    // Painel Direito: Histórico Interativo
    // ---------------------------------------------------------
    if (rightPanelRect.width > 0) {
        DrawRectangleRec(rightPanelRect, Fade(BLACK, 0.4f));

        // Cabeçalho
        float headerH = 40;
        DrawText("Moves", rightPanelRect.x + 10, rightPanelRect.y + 10, 20, WHITE);
        
        // Área do Rodapé (Controles Navegação)
        float navH = 50;
        float navY = rightPanelRect.y + rightPanelRect.height - navH;
        
        // Área da Toolbar (Novos Botões: Resign, Copy, Flip)
        float toolbarH = 40; 
        float toolbarY = navY - toolbarH - 5; // 5px de padding acima da navegação

        // Desenha a Toolbar (4 botões em linha)
        // Resign | Copy FEN | Copy PGN | Flip
        const char* toolLabels[] = { "Resign", "Copy FEN", "Copy PGN", "Flip" };
        float btnMargin = 5;
        float btnW = (rightPanelRect.width - (btnMargin * 5)) / 4.0f; 
        
        for(int i=0; i<4; i++) {
            Rectangle btnRect = { 
                rightPanelRect.x + btnMargin + (i * (btnW + btnMargin)), 
                toolbarY, 
                btnW, 
                30
            };
            
            bool clicked = drawButton(btnRect, toolLabels[i]);
            
            // Lógica dos Botões
            if (clicked) {
                if (i == 0) { // RESIGN
                    if (!isGameOver) {
                        isGameOver = true;
                        gameReason = REASON_RESIGNATION;
                        if (userIsWhite) gameResult = RESULT_BLACK_WINS;
                        else gameResult = RESULT_WHITE_WINS;
                        
                        showGameOverPopup = true;
                    }
                }
                else if (i == 1) { // COPY FEN
                    SetClipboardText(generateFEN(true).c_str());
                }
                else if (i == 2) { // COPY PGN
                    SetClipboardText(generatePGN().c_str());
                }
                else if (i == 3) { // FLIP
                    isFlipped = !isFlipped;
                }
            }
        }

        // Desenha Controles de Navegação (Original)
        drawControlButtons(rightPanelRect.x, navY, rightPanelRect.width);

        // Ajusta a Área da Lista (Scrollable) para terminar ANTES da toolbar
        Rectangle listRect = {
            rightPanelRect.x, 
            rightPanelRect.y + headerH, 
            rightPanelRect.width, 
            rightPanelRect.height - headerH - navH - toolbarH - 10
        };
        
        // Scissor Mode: Garante que o texto não vaze para fora da lista
        BeginScissorMode((int)listRect.x, (int)listRect.y, (int)listRect.width, (int)listRect.height);
        
        float startY = listRect.y + 5 - scrollOffset;
        float lineHeight = 28;
        float colWidth = (listRect.width - 50) / 2; 

        // Desenha a lista
        int currentPlyCounter = 0; // 0=Start, 1=White1, 2=Black1...

        for (int i = 0; i < (int)history.size(); i++) {
            float y = startY + i * lineHeight;
            
            // Só desenha se estiver visível na tela (Otimização)
            if (y + lineHeight < listRect.y || y > listRect.y + listRect.height) {
                currentPlyCounter += (history[i].blackMove.empty() ? 1 : 2);
                continue;
            }

            // Número do Lance
            char numStr[8];
            sprintf(numStr, "%d.", history[i].moveNumber);
            DrawText(numStr, listRect.x + 10, y + 4, 20, LIGHTGRAY);

            // Lance das Brancas (Ply impar: 1, 3, 5...)
            currentPlyCounter++; // Agora é o ply do lance branco
            Rectangle whiteRect = { listRect.x + 45, y, colWidth, lineHeight };
            
            // Highlight e Clique
            bool isHoverW = CheckCollisionPointRec(GetMousePosition(), whiteRect);
            bool isSelectedW = (viewPly == currentPlyCounter);
            
            if (isSelectedW) DrawRectangleRec(whiteRect, {54, 154, 235, 200}); // Azul Ativo
            else if (isHoverW) DrawRectangleRec(whiteRect, {255, 255, 255, 30}); // Hover

            if (isHoverW && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                viewPly = currentPlyCounter;
            }
            
            DrawText(history[i].whiteMove.c_str(), whiteRect.x + 5, y + 4, 20, isSelectedW ? WHITE : LIGHTGRAY);

            // 3. Lance das Pretas (Ply par: 2, 4, 6...)
            if (!history[i].blackMove.empty()) {
                currentPlyCounter++;
                Rectangle blackRect = { listRect.x + 45 + colWidth, y, colWidth, lineHeight };

                bool isHoverB = CheckCollisionPointRec(GetMousePosition(), blackRect);
                bool isSelectedB = (viewPly == currentPlyCounter);

                if (isSelectedB) DrawRectangleRec(blackRect, {54, 154, 235, 200});
                else if (isHoverB) DrawRectangleRec(blackRect, {255, 255, 255, 30});

                if (isHoverB && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    viewPly = currentPlyCounter;
                }

                DrawText(history[i].blackMove.c_str(), blackRect.x + 5, y + 4, 20, isSelectedB ? WHITE : LIGHTGRAY);
            }
        }
        
        EndScissorMode();

        // Controle do Scroll com roda do mouse
        if (CheckCollisionPointRec(GetMousePosition(), listRect)) {
            float wheel = GetMouseWheelMove();
            scrollOffset -= wheel * 30.0f;
            
            // Limites do Scroll
            float maxScroll = (history.size() * lineHeight) - (listRect.height - 50);
            if (maxScroll < 0) maxScroll = 0;
            if (scrollOffset < 0) scrollOffset = 0;
            if (scrollOffset > maxScroll) scrollOffset = maxScroll;
        }
    }
}

void ChessGUI::drawControlButtons(float x, float y, float w) {
    float btnW = w / 4.0f;
    float btnH = 50;
    
    // Cores
    Color normal = {40, 40, 40, 255};
    Color hover = {60, 60, 60, 255};
    
    // Botões: <<  <  >  >>
    const char* labels[4] = { "<<", "<", ">", ">>" };
    
    for (int i = 0; i < 4; i++) {
        Rectangle btnRect = { x + (i * btnW), y, btnW, btnH };
        bool isMouseOver = CheckCollisionPointRec(GetMousePosition(), btnRect);
        
        DrawRectangleRec(btnRect, isMouseOver ? hover : normal);
        DrawRectangleLinesEx(btnRect, 1, BLACK);
        
        int txtW = MeasureText(labels[i], 20);
        DrawText(labels[i], btnRect.x + (btnW - txtW)/2, btnRect.y + 15, 20, WHITE);
        
        if (isMouseOver && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            if (i == 0) goToFirstMove();
            if (i == 1) goToPreviousMove();
            if (i == 2) goToNextMove();
            if (i == 3) goToLastMove();
        }
    }
}

void ChessGUI::drawHighlights() {
    float sz = getSquareSize();
    
    // 1. Lógica de Seleção de Highlight
    // Só destacamos um movimento se NÃO estivermos no estado inicial (0)
    if (viewPly > 0 && viewPly <= (int)flatMoveHistory.size()) {
        
        // O lance que resultou neste estado visual é o (viewPly - 1)
        Move m = flatMoveHistory[viewPly - 1];
        
        Vector2 fromPos = getSquarePos(m.from);
        Vector2 toPos   = getSquarePos(m.to);
        
        DrawRectangle(fromPos.x, fromPos.y, sz, sz, HIGHLIGHT_MOVE);
        DrawRectangle(toPos.x, toPos.y, sz, sz, HIGHLIGHT_MOVE);
    }

    // 2. Highlight de Dragging (Interação Live)
    // Só desenha se estivermos olhando o último estado (Live)
    bool isLive = (viewPly == (int)stateHistory.size() - 1);
    
    if (isLive && isDragging && sourceSquare != -1) {
        Vector2 srcPos = getSquarePos(sourceSquare);
        DrawRectangle(srcPos.x, srcPos.y, sz, sz, HIGHLIGHT_DRAG);
        
        // Destinos
        for (const auto& m : legalMoves) {
            if (m.from == sourceSquare) {
                Vector2 destPos = getSquarePos(m.to);
                bool capture = board.pieceAt(m.to) != EMPTY; // Usa board "real" aqui pois é live
                
                if (capture) {
                    DrawRing({destPos.x + sz/2, destPos.y + sz/2}, sz*0.4f, sz*0.45f, 0, 360, 0, Fade(BLACK, 0.3f));
                } else {
                    DrawCircle(destPos.x + sz/2, destPos.y + sz/2, sz*0.15f, Fade(BLACK, 0.3f));
                }
            }
        }
    }
}

void ChessGUI::drawBoard() {
    float sz = getSquareSize();
    
    for (int i = 0; i < 64; i++) {
        int f = i % 8;
        int r = i / 8;
        
        // Cores alternadas
        Color c = ((r + f) % 2 == 0) ? DARK_SQUARE : LIGHT_SQUARE;
        
        Vector2 pos = getSquarePos(i);
        
        DrawRectangle(pos.x, pos.y, sz, sz, c);
    }
}

const Board& ChessGUI::getDisplayBoard() {
    if (stateHistory.empty()) return board;
    int idx = std::clamp(viewPly, 0, (int)stateHistory.size() - 1);
    return stateHistory[idx];
}

void ChessGUI::goToFirstMove() { setVisualPly(0); }
void ChessGUI::goToPreviousMove() { if (viewPly > 0) setVisualPly(viewPly - 1); }
void ChessGUI::goToNextMove() { if (viewPly < (int)stateHistory.size() - 1) setVisualPly(viewPly + 1); }
void ChessGUI::goToLastMove() { setVisualPly(stateHistory.size() - 1); }

void ChessGUI::drawPieces() {
    float sz = getSquareSize();
    const Board& displayBoard = getDisplayBoard(); 

    for (int sq = 0; sq < 64; sq++) {
        // Se estiver arrastando com mouse, não desenha a peça na origem
        if (isDragging && sq == sourceSquare) continue;
        
        bool animatingToHere = false;
        for(const auto& a : animations) { 
            if(a.toSq == sq) {
                animatingToHere = true; 
                break;
            }
        }
        if (animatingToHere) continue;

        Piece p = displayBoard.pieceAt(sq);
        if (p != EMPTY) {
            Vector2 pos = getSquarePos(sq);
            Rectangle dest = {pos.x, pos.y, sz, sz};
            DrawTexturePro(pieceTextures, getPieceRect(p), dest, {0,0}, 0, WHITE);
        }
    }
}

// =========================================================
//  UI Helpers (Botões e Menus)
// =========================================================

bool ChessGUI::drawButton(Rectangle rect, const char* text) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);
    bool clicked = false;

    if (hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        clicked = true;
    }

    // Cores Consistentes com o Tema Dark
    Color normalColor = BUTTON_COLOR; 
    Color hoverColor  = BUTTON_HOVER; 

    // Desenha Fundo
    DrawRectangleRec(rect, hovered ? hoverColor : normalColor);
    DrawRectangleLinesEx(rect, 1, BLACK); // Borda fina preta

    // Tamanho da fonte dinâmico (Max 30, mas diminui se o botão for pequeno)
    int fontSize = std::min(30, (int)(rect.height * 0.5f));
    int textW = MeasureText(text, fontSize);
    
    // Centraliza
    DrawText(text, 
             rect.x + (rect.width - textW) / 2, 
             rect.y + (rect.height - fontSize) / 2, 
             fontSize, WHITE);

    return clicked;
}

void ChessGUI::updateMenuLogic() {
    calculateLayout();
}

void ChessGUI::drawMenu() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    
    // Fundo escurecido glossy sobre o tabuleiro
    DrawRectangle(0, 0, sw, sh, Fade(BG_COLOR, 0.9f));

    float btnW = 300;
    float btnH = 60;
    float centerX = (sw - btnW) / 2;
    float startY = sh * 0.35f;

    // Título
    const char* title = "CAPY CHESS ENGINE";
    int titleW = MeasureText(title, 50);
    DrawText(title, (sw - titleW)/2, startY - 100, 50, WHITE);

    if (currentState == STATE_MENU_MAIN) {
        if (drawButton({centerX, startY, btnW, btnH}, "PLAY")) {
            currentState = STATE_MENU_MODE;
        }
        if (drawButton({centerX, startY + 80, btnW, btnH}, "EXIT")) {
            shouldClose = true;
        }
    }
    else if (currentState == STATE_MENU_MODE) {
        DrawText("Game mode:", centerX, startY - 40, 20, LIGHTGRAY);

        if (drawButton({centerX, startY, btnW, btnH}, "CLASSIC")) {
            currentState = STATE_MENU_SIDE;
        }
        
        DrawRectangleRec({centerX, startY + 80, btnW, btnH}, Fade(BUTTON_COLOR, 0.3f));
        DrawText("CHESS 960 (Soon)", centerX + 20, startY + 80 + 20, 20, Fade(WHITE, 0.3f));

        DrawRectangleRec({centerX, startY + 160, btnW, btnH}, Fade(BUTTON_COLOR, 0.3f));
        DrawText("SETUP POSITION (Soon)", centerX + 20, startY + 160 + 20, 20, Fade(WHITE, 0.3f));

        if (drawButton({centerX, startY + 260, btnW, btnH}, "BACK")) {
            currentState = STATE_MENU_MAIN;
        }
    }
    else if (currentState == STATE_MENU_SIDE) {
        DrawText("Choose your side:", centerX, startY - 40, 20, LIGHTGRAY);

        if (drawButton({centerX, startY, btnW, btnH}, "WHITE")) {
            userIsWhite = true;
            resetGame();
            currentState = STATE_GAME;
        }
        
        if (drawButton({centerX, startY + 80, btnW, btnH}, "BLACK")) {
            userIsWhite = false;
            resetGame();
            currentState = STATE_GAME;
        }

        if (drawButton({centerX, startY + 200, btnW, btnH}, "BACK")) {
            currentState = STATE_MENU_MODE;
        }
    }
}

void ChessGUI::drawGameOverPopup() {
    if (!showGameOverPopup) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // 1. Overlay Escuro (Fundo do jogo fica mais escuro)
    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.7f));

    // 2. Configuração da Janela
    float w = 420; // Um pouco mais largo para caber os botões
    float h = 220;
    Rectangle rect = { (sw - w)/2, (sh - h)/2, w, h };

    // Fundo da Janela (Cinza Escuro estilo painéis)
    // Usando uma cor sólida levemente mais clara que o BG_COLOR
    Color popupBg = { 55, 53, 50, 255 }; 
    DrawRectangleRec(rect, popupBg);
    
    // Borda Sutil
    DrawRectangleLinesEx(rect, 1, { 80, 80, 80, 255 }); 

    // 3. Título (Game Over / Resultado)
    const char* titleText = "GAME OVER";
    Color titleColor = WHITE;

    if (gameResult == RESULT_WHITE_WINS) { 
        titleText = "WHITE WINS"; 
        titleColor = { 155, 235, 155, 255 }; // Verde pastel
    }
    else if (gameResult == RESULT_BLACK_WINS) { 
        titleText = "BLACK WINS"; 
        titleColor = { 155, 155, 235, 255 }; // Azul pastel
    }
    else if (gameResult == RESULT_DRAW) { 
        titleText = "DRAW"; 
        titleColor = { 200, 200, 200, 255 }; // Cinza claro
    }

    int titleW = MeasureText(titleText, 40);
    DrawText(titleText, rect.x + (w - titleW)/2, rect.y + 30, 40, titleColor);

    // 4. Subtítulo (Motivo)
    const char* reasonText = "";
    switch(gameReason) {
        case REASON_CHECKMATE: reasonText = "by Checkmate"; break;
        case REASON_STALEMATE: reasonText = "by Stalemate"; break;
        case REASON_REPETITION: reasonText = "by Repetition"; break;
        case REASON_RESIGNATION: reasonText = "by Resignation"; break;
        case REASON_INSUFFICIENT_MATERIAL: reasonText = "Insufficient Material"; break;
        case REASON_50_MOVE_RULE: reasonText = "50-move rule"; break;
        default: break;
    }

    int reasonW = MeasureText(reasonText, 20);
    DrawText(reasonText, rect.x + (w - reasonW)/2, rect.y + 80, 20, LIGHTGRAY);

    // 5. Botões
    // Layout: [Rematch] [ Save ] [ Menu ]
    // Espaçamento calculado
    float padding = 20;
    float btnSpacing = 15;
    float totalBtnWidth = w - (padding * 2); 
    float btnW = (totalBtnWidth - (btnSpacing * 2)) / 3; // Divide igualmente por 3
    float btnH = 45;
    float btnY = rect.y + 140;

    // Botão 1: Rematch
    if (drawButton({rect.x + padding, btnY, btnW, btnH}, "Rematch")) {
        resetGame();
    }

    // Botão 2: Save
    if (drawButton({rect.x + padding + btnW + btnSpacing, btnY, btnW, btnH}, "Save")) {
        // Copiar FEN para clipboard como "Save" temporário
        SetClipboardText(generateFEN().c_str());
    }
    
    // Botão 3: Menu
    if (drawButton({rect.x + padding + (btnW + btnSpacing)*2, btnY, btnW, btnH}, "Menu")) {
        currentState = STATE_MENU_MAIN;
        resetGame();
    }

    // 6. Botão de Fechar (X)
    // Um X pequeno e discreto no canto
    Rectangle closeBtn = { rect.x + w - 30, rect.y + 5, 25, 25 };
    bool hoverClose = CheckCollisionPointRec(GetMousePosition(), closeBtn);
    
    DrawText("x", closeBtn.x + 8, closeBtn.y - 2, 24, hoverClose ? WHITE : GRAY);
    
    if (hoverClose && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        showGameOverPopup = false; 
    }
}

void ChessGUI::draw() {
    BeginDrawing();
    ClearBackground(BG_COLOR);

    drawPanels();      // Painéis laterais
    drawBoard();       // Tabuleiro Base
    drawHighlights();  // Cores (Last Move, Seleção)
    drawPieces();      // Peças Estáticas

    float sz = getSquareSize();
    for (const auto& anim : animations) {
        Rectangle dest = { anim.currentPos.x, anim.currentPos.y, sz, sz };
        DrawTexturePro(pieceTextures, getPieceRect(anim.piece), dest, {0,0}, 0.0f, WHITE);
    }

    if (isDragging && sourceSquare != -1) {
        Piece p = board.pieceAt(sourceSquare);
        Rectangle dest = { dragPos.x - sz/2, dragPos.y - sz/2, sz, sz };
        DrawTexturePro(pieceTextures, getPieceRect(p), dest, {0,0}, 0.0f, WHITE);
    }

    if (showGameOverPopup) {
        drawGameOverPopup();
    }

    if (currentState != STATE_GAME && !showGameOverPopup) {
        drawMenu();
    }

    EndDrawing();
}

std::string squareToAlgebraic(int sq) {
    if (sq < 0 || sq > 63) return "-";
    char f = 'a' + (sq % 8);
    return std::string{f, (char)('1' + (sq/8))}; 
}

std::string ChessGUI::generateFEN(bool full) {
    std::string fen = "";
    int empty = 0;

    // 1. Tabuleiro (Rank 8 até 1)
    for (int r = 7; r >= 0; r--) {
        for (int f = 0; f < 8; f++) {
            int sq = r * 8 + f; 
            Piece p = board.pieceAt(sq);

            if (p == EMPTY) {
                empty++;
            } else {
                if (empty > 0) {
                    fen += std::to_string(empty);
                    empty = 0;
                }
                char c = '?';
                switch(p) {
                    case WPAWN:   c='P'; break; case WKNIGHT: c='N'; break;
                    case WBISHOP: c='B'; break; case WROOK:   c='R'; break;
                    case WQUEEN:  c='Q'; break; case WKING:   c='K'; break;
                    case BPAWN:   c='p'; break; case BKNIGHT: c='n'; break;
                    case BBISHOP: c='b'; break; case BROOK:   c='r'; break;
                    case BQUEEN:  c='q'; break; case BKING:   c='k'; break;
                    case EMPTY: break;
                }
                fen += c;
            }
        }
        if (empty > 0) { fen += std::to_string(empty); empty = 0; }
        if (r > 0) fen += "/";
    }

    // 2. Vez
    fen += (board.whiteToMove ? " w " : " b ");

    // 3. Roque
    std::string castling = "";
    if (board.castlingRights & 1) castling += "K";
    if (board.castlingRights & 2) castling += "Q";
    if (board.castlingRights & 4) castling += "k";
    if (board.castlingRights & 8) castling += "q";
    if (castling == "") castling = "-";
    fen += castling;

    // 4. En Passant
    fen += " ";
    if (board.enPassantSquare != -1) {
        fen += squareToAlgebraic(board.enPassantSquare);
    } else {
        fen += "-";
    }

    // --- PONTO DE CORTE ---
    // Se full=false, paramos aqui. Isso gera a chave perfeita para repetição.
    if (!full) return fen;

    // 5. Contadores (Só para Save/Copy)
    // Halfmove (50 regras)
    fen += " " + std::to_string(fiftyMoveCounter);
    
    // Fullmove (Número da jogada)
    int fullMove = 1 + (flatMoveHistory.size() / 2);
    fen += " " + std::to_string(fullMove);

    return fen;
}


std::string ChessGUI::generatePGN() {
    std::stringstream ss;

    // ======================
    // 1. Cabeçalhos (Tags)
    // ======================
    time_t now = time(0);
    struct tm tstruct = *localtime(&now);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y.%m.%d", &tstruct);

    ss << "[Event \"Capy Chess Game\"]\n";
    ss << "[Site \"Local\"]\n";
    ss << "[Date \"" << buf << "\"]\n";
    ss << "[Round \"1\"]\n";
    ss << "[White \"" << whitePlayer.name << "\"]\n";
    ss << "[Black \"" << blackPlayer.name << "\"]\n";

    std::string fen = generateFEN(true);
    
    if(isFischerRandom){
        ss << "[SetUp \"1\"]\n";
        ss << "[FEN \"" << fen << "\"]\n";
    }

    // ======================
    // Resultado
    // ======================
    std::string result = "*";
    if (gameResult == RESULT_WHITE_WINS) result = "1-0";
    else if (gameResult == RESULT_BLACK_WINS) result = "0-1";
    else if (gameResult == RESULT_DRAW) result = "1/2-1/2";

    ss << "[Result \"" << result << "\"]\n\n";

    // ======================
    // 2. Movimentos
    // ======================
    for (const auto& entry : history) {
        ss << entry.moveNumber << ". " << entry.whiteMove << " ";
        if (!entry.blackMove.empty()) {
            ss << entry.blackMove << " ";
        }
    }

    // Resultado no final do PGN
    ss << result;

    return ss.str();
}

bool ChessGUI::checkInsufficientMaterial() {
    // Contagem de peças
    int wPawn=0, wKnight=0, wBishop=0, wRook=0, wQueen=0;
    int bPawn=0, bKnight=0, bBishop=0, bRook=0, bQueen=0;

    // Varre o tabuleiro (ou use os bitboards popcount se tiver acesso)
    // Usando loop simples para compatibilidade com o código atual
    for(int i=0; i<64; i++) {
        Piece p = board.pieceAt(i);
        if (p == WPAWN) wPawn++; else if (p == WKNIGHT) wKnight++;
        else if (p == WBISHOP) wBishop++; else if (p == WROOK) wRook++;
        else if (p == WQUEEN) wQueen++;
        else if (p == BPAWN) bPawn++; else if (p == BKNIGHT) bKnight++;
        else if (p == BBISHOP) bBishop++; else if (p == BROOK) bRook++;
        else if (p == BQUEEN) bQueen++;
    }

    // 1. Se tem Peões, Torres ou Damas -> Tem material suficiente
    if (wPawn+wRook+wQueen+bPawn+bRook+bQueen > 0) return false;

    // O que sobrou: Reis + Cavalos + Bispos
    int wMinor = wKnight + wBishop;
    int bMinor = bKnight + bBishop;

    // Rei vs Rei
    if (wMinor == 0 && bMinor == 0) return true;

    // Rei + Menor vs Rei (Impossível mate forçado)
    if ((wMinor == 1 && bMinor == 0) || (wMinor == 0 && bMinor == 1)) return true;

    // Rei + 2 Cavalos vs Rei (Impossível mate forçado sem ajuda do oponente)
    // Nota: K+NN vs K+P NÃO é empate. Mas aqui já filtramos peões.
    if ((wKnight == 2 && wBishop == 0 && bMinor == 0) || 
        (bKnight == 2 && bBishop == 0 && wMinor == 0)) return true;
        
    // Bispos de cores opostas? (K+B vs K+B) -> Geralmente empate, mas engines jogam.
    // Vamos ficar no básico pedido.

    return false;
}

void ChessGUI::checkGameOver() {
    if (checkInsufficientMaterial()) {
        isGameOver = true;
        gameResult = RESULT_DRAW;
        gameReason = REASON_INSUFFICIENT_MATERIAL;
        timerActive = true;
        return;
    }

    std::string currentFen = generateFEN();
    positionHistory[currentFen]++;
    
    if (positionHistory[currentFen] >= 3) {
        isGameOver = true;
        gameResult = RESULT_DRAW;
        gameReason = REASON_REPETITION;
        timerActive = true;
        return;
    }

    // Checa Mate ou Stalemate
    // Se não há movimentos legais disponíveis
    if (legalMoves.empty()) {
        isGameOver = true;
        timerActive = true;

        bool inCheck = board.whiteToMove ? (board.whiteKing & board.blackAttacks) 
                                     : (board.blackKing & board.whiteAttacks);

        if (inCheck) {
            gameReason = REASON_CHECKMATE;
            gameResult = board.whiteToMove ? RESULT_BLACK_WINS : RESULT_WHITE_WINS;
        } else {
            gameReason = REASON_STALEMATE;
            gameResult = RESULT_DRAW;
        }
    }

    if (fiftyMoveCounter >= 100) {
        isGameOver = true;
        gameResult = RESULT_DRAW;
        gameReason = REASON_50_MOVE_RULE;
        timerActive = true;
        return;
    }
}
