CXX      := g++
CXXFLAGS := -std=c++23 -Wall -Wextra -Wshadow -MMD -MP -fconstexpr-ops-limit=100000000
LDFLAGS  := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# ==========================================
# Diretórios
# ==========================================

SRC_ROOT := src
BIN_DIR  := bin
type ?= release

BUILD_DIR := build/$(type)
DEBUG_BIN_DIR := $(BIN_DIR)/debug/$(type)

# ==========================================
# CORE da engine
# ==========================================

ENGINE_CORE_SRCS := $(shell find $(SRC_ROOT) \
    -path $(SRC_ROOT)/debug -prune -o \
    -name '*.cpp' -print | grep -v '/main.cpp$$')

ENGINE_CORE_OBJS := $(ENGINE_CORE_SRCS:%.cpp=$(BUILD_DIR)/%.o)
OBJS := $(ENGINE_CORE_OBJS)

TARGET := $(BIN_DIR)/chess_engine

# ==========================================
# Configuração de build
# ==========================================

ifeq ($(type),debug)
    CXXFLAGS += -O3 -march=native -flto -DDEBUG
    TARGET := $(TARGET)_debug
else
    CXXFLAGS += -O3 -march=native -flto -DNDEBUG
endif

# ==========================================
# Regras
# ==========================================

.PHONY: all clean run directories debug-tool

all: directories $(if $(NO_ENGINE),,$(TARGET))

directories:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(BUILD_DIR)

# ---------- Link ----------
$(TARGET): $(OBJS) src/main.cpp
	@echo "--------------------------------------"
	@echo "Linking $(TARGET)..."
	@$(CXX) $(CXXFLAGS) $(OBJS) src/main.cpp -o $(TARGET) $(LDFLAGS)
	@echo "Build successful -> $(TARGET)"
	@echo "--------------------------------------"

# ---------- Compile ----------
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# ---------- Clean ----------
clean:
	@echo "Cleaning up..."
	@rm -rf build bin

# ==========================================
# MAKERUN
# ==========================================

run:
	@if [ -z "$(word 2,$(MAKECMDGOALS))" ]; then \
		$(MAKE) all; \
		echo "--------------------------------------"; \
		echo "Running $(TARGET)..."; \
		echo "--------------------------------------"; \
		./$(TARGET); \
	else \
		$(MAKE) debug-tool NAME=$(word 2,$(MAKECMDGOALS)) NO_ENGINE=1; \
	fi

debug-tool: directories $(ENGINE_CORE_OBJS)
	@mkdir -p $(DEBUG_BIN_DIR)/$(dir $(NAME))
	@echo "--------------------------------------"
	@echo "Building debug tool: src/debug/$(NAME).cpp"
	@$(CXX) $(CXXFLAGS) src/debug/$(NAME).cpp $(ENGINE_CORE_OBJS) -o $(DEBUG_BIN_DIR)/$(NAME)
	@echo "Running $(NAME)..."
	@echo "--------------------------------------"
	@./$(DEBUG_BIN_DIR)/$(NAME)

%:
	@:

-include $(OBJS:.o=.d)
