<h1>
   <img src="assets/capy.png" width="150" align="middle" />
   Capy Chess Engine ♟️
</h1>

![Status](https://img.shields.io/badge/Status-Active-success)
![Language](https://img.shields.io/badge/Language-C%2B%2B23-blue)
![License](https://img.shields.io/badge/License-MIT-green)

**Capy Chess Engine** is a chess engine and GUI built in **C++23**, designed to explore chess heuristics, search algorithms and bitboard-based state representation. It features a custom graphical interface powered by **Raylib**, providing a seamless experience for playing, analyzing, and managing chess games.
The project emphasizes code clarity and modern C++ practices making it an excellent resource for understanding chess programming fundamentals.

---

## 🚀 Key Features

*   **Advanced Engine Core:**
    *   **Bitboard Representation:** Highly optimized board state management using 64-bit integers for blazing fast move generation and evaluation.
    *   **Search:** Implements Alpha-Beta pruning with iterative deepening for strong tactical play.
    *   **Transposition Table:** Utilizes Zobrist Hashing (64MB default) to cache search results and detect repetitions.
    *   **Evaluation Function:** Hand-crafted static evaluation considering material and piece-square tables.

*   **GUI (Capy Interface):**
    *   **Fluid Animations:** Smooth piece movements and intuitive drag-and-drop mechanics.
    *   **Game Management:** Save and load games (PGN format) with a visual library of saved matches.
    *   **Replay System:** Integrated replay mode to analyze past games move-by-move.
    *   **State Control:** Supports FEN/PGN copying, board flipping, and instant game reset.
    *   **Visual Feedback:** Highlights for legal moves, last move, checks, and game termination reasons (Checkmate, Stalemate, 50-move rule, etc.).

---

## 🛠️ Tech Stack

*   **Language:** C++23 (GCC 13+ or Clang 16+ recommended)
*   **Graphics:** [Raylib 5.0](https://www.raylib.com/)
*   **Build System:** GNU Make
*   **Platform:** Linux (Tested on Arch Linux)

---

## 📦 Installation & Build

### Prerequisites

Ensure you have a C++23 compatible compiler and the Raylib development libraries installed.

**Ubuntu / Debian:**
```bash
sudo apt update
sudo apt install build-essential git libraylib-dev libx11-dev libgl1-mesa-dev
```

**Arch Linux:**
```bash
sudo pacman -S base-devel git raylib libx11 mesa
```
### Building the Project

Clone the repository and compile using `make`:

```bash
git clone https://github.com/your-username/capy-chess-engine.git
cd capy-chess-engine

# Build and run in Release mode (Recommended)
make run

# Build in Debug mode (for development)
make run type=debug
```

The executable will be generated in `bin/chess_engine`.

---

## 🎮 How to Play

1.  **Launch:** Run `make run` to open the main menu.
2.  **Play:** Select **"PLAY"**, choose your mode (Classic), and pick a side (White or Black).
    *   *Note: The engine (Capy) will play the opposite color.*
3.  **Controls:**
    *   **Drag & Drop:** Move pieces with the mouse.
    *   **Arrow Keys:** Navigate through the move history.
    *   **'F' Key:** Flip the board view.
4.  **Saved Games:** Access your match history from the "SAVED GAMES" menu. You can replay matches or delete old ones.

---

## 📂 Project Structure

```
capy-chess-engine/
├── src/
│   ├── board/      # Bitboard implementation and piece logic
│   ├── eval/       # Static evaluation heuristics
│   ├── gui/        # Raylib-based graphical interface
│   ├── move/       # Move generation and validation
│   ├── search/     # Alpha-Beta search algorithm
│   ├── tt/         # Transposition Table implementation
│   ├── zobrist/    # Zobrist Hashing for board states
│   └── main.cpp    # Entry point
├── assets/         # Images and resources
├── local/          # Saved PGN games
└── Makefile        # Build configuration
```

---

## 📄 License

Distributed under the MIT License. See `LICENSE` for more information.

---

*Feito com carinho por um jogador capivaroso*
