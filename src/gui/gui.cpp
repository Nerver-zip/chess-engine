#include "gui.h"
#include "../move/movegen.h"
#include "../search/search.h"
#include "../debuglib/debug.h"
#include <iostream>
#include <algorithm>

// =========================================================
//  Construtor e Destrutor
// =========================================================

ChessGUI::ChessGUI() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT); 
    InitWindow(800, 800, "Chess Engine - Raylib");
    SetTargetFPS(60);

    // Carrega FEN inicial
    board = Board::fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    // board = Board::fromFEN("6k1/1P6/8/p3p2p/1p6/6b1/5p2/R4K2 w - - 0 1"); // Teste promoção

    board.updateAttackBoards();
    legalMoves = MoveGen::generateMoves(board);
    
    loadAssets();
    Debug::printBoard(board);
}

ChessGUI::~ChessGUI() {
    UnloadTexture(pieceTextures);
    CloseWindow();
}

void ChessGUI::loadAssets() {
    pieceTextures = LoadTexture("assets/pieces.png"); 
    SetTextureFilter(pieceTextures, TEXTURE_FILTER_BILINEAR);
}

// =========================================================
//  Helpers de Dimensão e Posição
// =========================================================

// Garante que o tabuleiro seja sempre quadrado e caiba na tela
float ChessGUI::getBoardSize() {
    return (float)std::min(GetScreenWidth(), GetScreenHeight());
}

float ChessGUI::getSquareSize() {
    return getBoardSize() / 8.0f;
}

// Converte Coordenada Lógica (0..63) -> Pixels (Vector2 centro ou canto)
Vector2 ChessGUI::getSquarePos(int sq) {
    float sz = getSquareSize();
    int f = sq % 8;
    int r = 7 - (sq / 8); // Inverte Y (Rank 0 é embaixo)
    return Vector2 { f * sz, r * sz };
}

int ChessGUI::getSquareFromMouse() {
    Vector2 m = GetMousePosition();
    float sz = getSquareSize();
    
    // Se mouse estiver fora da área do tabuleiro
    if (m.x < 0 || m.x > getBoardSize() || m.y < 0 || m.y > getBoardSize()) {
        return -1;
    }

    int file = (int)(m.x / sz);
    int rank = 7 - (int)(m.y / sz);

    // Clamp para segurança
    if (file < 0) file = 0; 
    if (file > 7) file = 7;
    if (rank < 0) rank = 0; 
    if (rank > 7) rank = 7;

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
    while (!WindowShouldClose()) {
        updateLogic();
        draw();
    }
}

void ChessGUI::updateLogic() {
    // -----------------------------------------------------
    // 1. Atualiza Animações (Interpolação Suave)
    // -----------------------------------------------------
    for (auto& anim : animations) {
        // Lerp: Move currentPos em direção a targetPos
        // Smoothness 0.2f dá um efeito agradável (quanto menor, mais lento/pesado)
        anim.currentPos = Vector2Lerp(anim.currentPos, anim.targetPos, smoothness);
        
        // Snap: Se chegou muito perto (< 1 pixel), finaliza
        if (Vector2Distance(anim.currentPos, anim.targetPos) < 0.5f) {
            anim.currentPos = anim.targetPos;
            anim.finished = true;
        }
    }
    // Remove animações concluídas da lista
    animations.remove_if([](const MovingPiece& m) { return m.finished; });

    // -----------------------------------------------------
    // 2. Vez da Engine
    // -----------------------------------------------------
    // Só deixa a engine jogar se não tiver animação rodando (opcional, para visual limpo)
    if (!board.whiteToMove && animations.empty()) {
        makeEngineMove();
        return;
    }

    // -----------------------------------------------------
    // 3. Input Humano (Drag and Drop)
    // -----------------------------------------------------
    int mouseSq = getSquareFromMouse();
    targetSquare = mouseSq; // Para highlight visual

    // Iniciar Arraste (Mouse Down)
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (mouseSq != -1) {
            Piece p = board.pieceAt(mouseSq);
            // Só pega se for peça branca (humano)
            if (p >= WPAWN && p <= WKING) {
                isDragging = true;
                sourceSquare = mouseSq;
            }
        }
    }

    // Atualizar Posição (Mouse Held)
    if (isDragging) {
        dragPos = GetMousePosition();
    }

    // Soltar Peça (Mouse Up)
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && isDragging) {
        isDragging = false;

        // Verifica se soltou numa casa válida diferente da origem
        if (mouseSq != -1 && mouseSq != sourceSquare) {
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
                // Auto-promotion para Queen por enquanto
                if (move.flags & PROMOTION && move.promotion == EMPTY) {
                    move.promotion = WQUEEN;
                }
                
                // Aplica o movimento usando a função de animação
                performMove(move); 
            }
        }
        sourceSquare = -1;
    }
}

// =========================================================
//  Execução de Movimentos e Animação
// =========================================================

void ChessGUI::performMove(Move m) {
    // 1. Configura Animação da Peça Principal
    Vector2 startPos = getSquarePos(m.from);
    Vector2 endPos   = getSquarePos(m.to);
    
    // Se o movimento veio de um Drag Humano, seria legal iniciar startPos 
    // onde o mouse soltou para fazer um "snap", mas usar a origem é mais seguro e consistente.

    Piece p = board.pieceAt(m.from);
    if (m.flags & PROMOTION) p = (Piece)m.promotion; // Visualmente já vira a Dama

    MovingPiece anim;
    anim.piece = p;
    anim.toSq = m.to; // Oculta a peça estática no destino
    anim.currentPos = startPos;
    anim.targetPos = endPos;
    anim.finished = false;
    animations.push_back(anim);

    // 2. Configura Animação da Torre (se for Roque)
    if (m.flags & KING_CASTLE || m.flags & QUEEN_CASTLE) {
        bool kingside = (m.flags & KING_CASTLE);
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

    // 3. Aplica Lógica
    board = board.applyMove(m);
    board.updateAttackBoards();
    legalMoves = MoveGen::generateMoves(board);
  
    // No futuro: Som
    // PlaySound(moveSound);
    
    Debug::printBoard(board);
}

void ChessGUI::makeEngineMove() {
    // Feedback visual simples
    BeginDrawing();
        // Desenha uma caixinha no topo
        DrawRectangle(0,0,220,30, BLACK);
        DrawText(" Engine thinking... ", 10, 5, 20, WHITE);
    EndDrawing();

    Move best = Search::searchBestMove(board, 5);
    
    if (best.from != best.to) { // Se move for válido
        performMove(best);
    }
}

// =========================================================
//  Renderização (Draw)
// =========================================================

void ChessGUI::draw() {
    BeginDrawing();
    ClearBackground(RAYWHITE); 

    float sz = getSquareSize();

    // -----------------------------------------------------
    // 1. Tabuleiro
    // -----------------------------------------------------
    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            Color c = ((r + f) % 2 == 0) ? Color{238, 238, 210, 255} : Color{118, 150, 86, 255};
            
            // Highlight do último lance? (Implementar depois usando um histórico)
            
            DrawRectangle(f * sz, r * sz, sz, sz, c);
        }
    }

    // -----------------------------------------------------
    // 2. Highlights (Casas possíveis)
    // -----------------------------------------------------
    if (isDragging && sourceSquare != -1) {
        // Casa de origem
        Vector2 srcPos = getSquarePos(sourceSquare);
        DrawRectangle(srcPos.x, srcPos.y, sz, sz, Fade(YELLOW, 0.6f));

        // Destinos
        for (const auto& m : legalMoves) {
            if (m.from == sourceSquare) {
                Vector2 destPos = getSquarePos(m.to);
                
                if (board.pieceAt(m.to) != EMPTY) {
                     // Captura: Anel
                     DrawRing({destPos.x + sz/2, destPos.y + sz/2}, sz*0.4f, sz*0.45f, 0, 360, 0, Fade(BLACK, 0.2f));
                } else {
                    // Move quieto: Bolinha
                    DrawCircle(destPos.x + sz/2, destPos.y + sz/2, sz * 0.15f, Fade(BLACK, 0.2f));
                }
            }
        }
    }

    // -----------------------------------------------------
    // 3. Peças Estáticas
    // -----------------------------------------------------
    for (int sq = 0; sq < 64; sq++) {
        // NÃO desenha se a peça estiver sendo arrastada pelo mouse
        if (isDragging && sq == sourceSquare) continue;

        // NÃO desenha se a peça estiver sendo animada chegando nesta casa (evita duplicação)
        bool isAnimatingDest = false;
        for (const auto& anim : animations) {
            if (anim.toSq == sq) {
                isAnimatingDest = true;
                break;
            }
        }
        if (isAnimatingDest) continue; 

        Piece p = board.pieceAt(sq);
        if (p != EMPTY) {
            Rectangle src = getPieceRect(p);
            Vector2 pos = getSquarePos(sq);
            Rectangle dest = { pos.x, pos.y, sz, sz };
            DrawTexturePro(pieceTextures, src, dest, {0,0}, 0.0f, WHITE);
        }
    }

    // -----------------------------------------------------
    // 4. Peças Animadas (Flying Pieces)
    // -----------------------------------------------------
    for (const auto& anim : animations) {
        Rectangle src = getPieceRect(anim.piece);
        // Usa currentPos (interpolado)
        Rectangle dest = { anim.currentPos.x, anim.currentPos.y, sz, sz };
        
        // Sombra leve para dar profundidade ao movimento
        DrawTexturePro(pieceTextures, src, {dest.x+3, dest.y+3, dest.width, dest.height}, {0,0}, 0.0f, Fade(BLACK, 0.3f));
        DrawTexturePro(pieceTextures, src, dest, {0,0}, 0.0f, WHITE);
    }

    // -----------------------------------------------------
    // 5. Peça Arrastada (No Mouse - Top Layer)
    // -----------------------------------------------------
    if (isDragging && sourceSquare != -1) {
        Piece p = board.pieceAt(sourceSquare);
        Rectangle src = getPieceRect(p);
        
        // Centraliza no cursor
        Rectangle dest = { 
            dragPos.x - sz/2, 
            dragPos.y - sz/2, 
            sz, sz 
        };
        // Sombra
        DrawTexturePro(pieceTextures, src, {dest.x+5, dest.y+5, dest.width, dest.height}, {0,0}, 0.0f, Fade(BLACK, 0.3f));
        DrawTexturePro(pieceTextures, src, dest, {0,0}, 0.0f, WHITE);
    }

    EndDrawing();
}
