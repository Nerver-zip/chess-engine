<h1>
   <img src="assets/capy.png" width="150" align="middle" />
   Capy Chess Engine ♟️
</h1>

[![CI](https://github.com/Nerver-zip/chess-engine/actions/workflows/ci.yml/badge.svg)](https://github.com/Nerver-zip/chess-engine/actions/workflows/ci.yml)
[![Sanitizers](https://github.com/Nerver-zip/chess-engine/actions/workflows/sanitizers.yml/badge.svg)](https://github.com/Nerver-zip/chess-engine/actions/workflows/sanitizers.yml)
[![Release](https://github.com/Nerver-zip/chess-engine/actions/workflows/release.yml/badge.svg)](https://github.com/Nerver-zip/chess-engine/actions/workflows/release.yml)
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

*   **Continuous Integration & Quality:**
    *   Comprehensive unit test battery (smoke test, board/FEN, movegen, perft benchmarks, Zobrist hashing, static evaluation, mate search, UCI/SAN notation).
    *   GitHub Actions CI with matrix build (Debug and Release) on GCC.
    *   AddressSanitizer and UndefinedBehaviorSanitizer workflow.
    *   Automated GitHub Releases with packaged archives and assets triggered on git tags.

---

## 🛠️ Tech Stack

*   **Language:** C++23 (GCC 13+ or Clang 16+ recommended)
*   **Graphics:** [Raylib 5.0](https://www.raylib.com/)
*   **Build System:** CMake 3.25+ & GNU Make
*   **Testing:** GoogleTest & CTest
*   **Platform:** Linux (Tested on Arch Linux and Ubuntu 24.04)

---

## 📦 Installation & Build

### Prerequisites

Ensure you have a C++23 compatible compiler, CMake, Ninja, and the Raylib development libraries installed.

**Ubuntu / Debian:**
```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git libraylib-dev libx11-dev libgl1-mesa-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake ninja git raylib libx11 mesa
```

### Building the Project

Clone the repository:

```bash
git clone https://github.com/Nerver-zip/chess-engine.git
cd chess-engine
```

#### Option 1: Using Make (Quick Start)

```bash
# Build and run in Release mode (Recommended)
make run

# Build in Debug mode (for development)
make run type=debug

# Run the test suite
make test
```

The executable will be generated in `bin/chess_engine`.

#### Option 2: Using CMake & Presets (Recommended for CI & Development)

```bash
# Configure and build Debug preset
cmake --preset dev
cmake --build --preset dev

# Run all unit and perft tests
ctest --preset dev --output-on-failure

# Configure and build Release preset
cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure

# Build and run with Sanitizers (ASan + UBSan)
cmake --preset asan
cmake --build --preset asan
ctest --preset asan --output-on-failure
```

---

## 🧪 Running Tests

The test suite validates engine core correctness independently of the GUI:

```bash
# Via Make:
make test

# Or via CTest:
ctest --preset dev --output-on-failure
```

Test targets include:
- `engine_smoke_test`: Smoke tests for move generation and move application.
- `engine_board_test`: FEN parsing, attack maps, piece lookups, and attacker bitboards.
- `engine_movegen_test`: Legal move generation, castling rules, en passant capture, promotions, check evasions, and pins.
- `engine_perft_test`: Standard perft benchmark positions (Initial position, Kiwipete, Positions 3, 4, 5).
- `engine_zobrist_test`: Zobrist hash determinism, incremental update consistency, and transposition detection.
- `engine_eval_test`: Evaluation symmetry, material valuation, and piece-square tables.
- `engine_search_test`: Checkmate detection in 1 and 2 moves, best move generation, and Transposition Table probing/storage.
- `engine_notation_test`: Long algebraic (UCI) and Standard Algebraic Notation (SAN) conversions.

---

## 🚀 Releases & Downloads

To play without compiling from source, download the pre-built package from [Releases](https://github.com/Nerver-zip/chess-engine/releases):

1. Download the `.tar.gz` or `.zip` release archive.
2. Extract the archive:
   ```bash
   tar -xzf capy-chess-engine-*-Linux.tar.gz
   # or
   unzip capy-chess-engine-*-Linux.zip
   ```
3. Navigate into the extracted directory and launch the game:
   ```bash
   cd capy-chess-engine-*-Linux
   ./bin/chess_engine
   ```

---

## 🎮 How to Play

1.  **Launch:** Run `./bin/chess_engine` (or `make run`) to open the main menu.
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
chess-engine/
├── .github/
│   └── workflows/      # CI, Sanitizers, Release, and Clang-Tidy workflows
├── cmake/              # Compiler warnings and sanitizer helpers
├── tests/              # GoogleTest suite (smoke, board, movegen, perft, zobrist, eval, search, notation)
├── src/
│   ├── board/          # Bitboard implementation and piece logic
│   ├── eval/           # Static evaluation heuristics and PSTs
│   ├── gui/            # Raylib-based graphical interface
│   ├── move/           # Move generation, validation, and notation
│   ├── search/         # Alpha-Beta search with iterative deepening
│   ├── tt/             # Transposition Table implementation
│   ├── zobrist/        # Zobrist Hashing for board states
│   └── main.cpp        # Entry point
├── assets/             # Images and resources (sprites, capy avatar)
├── local/              # Saved PGN games
├── CMakeLists.txt      # Root CMake configuration
├── CMakePresets.json   # Presets for dev, release, and asan
├── Makefile            # GNU Make build and run configuration
└── README.md           # Project documentation
```

---

## 📄 License

Distributed under the MIT License. See `LICENSE` for more information.

---

*Feito com carinho por um jogador capivaroso*
