#include "gui.h"
#include "../move/movegen.h"
#include "../search/search.h"
#include "../debuglib/debug.h"
#include "../zobrist/zobrist.h"
#include "../tt/tt.h"
#include <algorithm>
#include <string>

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

    stateHistory.push_back(board);
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

    if (!isLive) return; 

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
    Piece p = board.pieceAt(m.from);
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

        // Área do Cabeçalho
        float headerH = 40;
        DrawText("Moves", rightPanelRect.x + 10, rightPanelRect.y + 10, 20, WHITE);
        
        // Área dos Botões de Controle (Rodapé)
        float controlsH = 50;
        float controlsY = rightPanelRect.y + rightPanelRect.height - controlsH;
        drawControlButtons(rightPanelRect.x, controlsY, rightPanelRect.width);

        // Área da Lista (Scrollable)
        Rectangle listRect = {
            rightPanelRect.x, 
            rightPanelRect.y + headerH, 
            rightPanelRect.width, 
            rightPanelRect.height - headerH - controlsH
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

    // Desenha Fundo
    DrawRectangleRec(rect, hovered ? BUTTON_HOVER : BUTTON_COLOR);
    DrawRectangleLinesEx(rect, 2, BLACK); // Borda

    // Desenha Texto Centralizado
    int fontSize = 30;
    int textW = MeasureText(text, fontSize);
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

    if (currentState != STATE_GAME) {
        drawMenu();
    }

    EndDrawing();
}
