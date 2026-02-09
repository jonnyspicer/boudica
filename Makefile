# Boudica Chess Engine - Production Makefile
# Supports clang++ (preferred) and g++ fallback

# Compiler detection
CXX := $(shell command -v clang++ 2>/dev/null)
ifeq ($(CXX),)
    CXX := $(shell command -v g++ 2>/dev/null)
endif
ifeq ($(CXX),)
    $(error No C++ compiler found. Install clang++ or g++)
endif

# Project structure
SRC_DIR := src
TUNER_DIR := src/tuner
BUILD_DIR := build
TARGET := boudica
TUNER_TARGET := tuner

# Source files - Engine
ENGINE_SRCS := $(wildcard $(SRC_DIR)/*.cpp)
ENGINE_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(ENGINE_SRCS))

# Source files - Tuner
TUNER_SRCS := $(wildcard $(TUNER_DIR)/*.cpp)
TUNER_OBJS := $(patsubst $(TUNER_DIR)/%.cpp,$(BUILD_DIR)/tuner/%.o,$(TUNER_SRCS))

# Engine core objects (excluding main.cpp for tuner linking)
ENGINE_CORE_OBJS := $(filter-out $(BUILD_DIR)/main.o,$(ENGINE_OBJS))

# All objects for dependency tracking
OBJS := $(ENGINE_OBJS)
DEPS := $(OBJS:.o=.d) $(TUNER_OBJS:.o=.d)

# C++ standard
CXXSTD := -std=c++17

# Warning flags
WARNINGS := -Wall -Wextra -Wpedantic

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LDFLAGS += -lpthread
endif
ifeq ($(UNAME_S),Darwin)
    # macOS specific flags if needed
endif

# Optimization flags (release build)
OPT_FLAGS := -O3 -march=native -flto -DNDEBUG

# Debug flags
DEBUG_FLAGS := -g -O0 -DDEBUG

# Dependency generation flags
DEP_FLAGS := -MMD -MP

# Default build is optimized release
CXXFLAGS := $(CXXSTD) $(WARNINGS) $(OPT_FLAGS) $(DEP_FLAGS)

# Phony targets
.PHONY: all debug clean test help tuner tune

# Default target
all: $(TARGET)

# Debug build
debug: CXXFLAGS := $(CXXSTD) $(WARNINGS) $(DEBUG_FLAGS) $(DEP_FLAGS)
debug: clean $(TARGET)

# Link target
$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create build directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/tuner

# Tuner target
$(TUNER_TARGET): $(ENGINE_CORE_OBJS) $(TUNER_OBJS)
	@echo "Linking $@..."
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Build complete: $(TUNER_TARGET)"

# Compile tuner source files
$(BUILD_DIR)/tuner/%.o: $(TUNER_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Convenience alias
.PHONY: build-tuner
build-tuner: $(TUNER_TARGET)

# Run tuner with default settings
tune: $(TUNER_TARGET)
	@echo "Running tuner..."
	@if [ -f data/quiet_labeled.epd ]; then \
		./$(TUNER_TARGET) --epd data/quiet_labeled.epd --epochs 100 --output data/tuned_params.txt; \
	else \
		echo "Error: No training data found. Please add EPD file to data/quiet_labeled.epd"; \
		echo "You can download training data from: https://github.com/official-stockfish/books"; \
	fi

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -f $(TARGET) $(TUNER_TARGET)
	@echo "Clean complete"

# Run perft tests (placeholder)
test: $(TARGET)
	@echo "Running perft tests..."
	@echo "TODO: Implement perft test runner"
	@echo "Example: ./$(TARGET) bench"

# Help target
help:
	@echo "Boudica Chess Engine Build System"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build optimized release binary (default)"
	@echo "  debug   - Build debug binary with symbols"
	@echo "  tuner   - Build the Texel tuning tool"
	@echo "  tune    - Run tuner with default settings"
	@echo "  clean   - Remove build artifacts"
	@echo "  test    - Run perft tests"
	@echo "  help    - Show this help message"
	@echo ""
	@echo "Tuner Usage:"
	@echo "  ./tuner --epd data/quiet.epd --epochs 100 --output tuned.txt"
	@echo "  ./tuner --load tuned.txt --print-cpp"
	@echo ""
	@echo "Compiler: $(CXX)"
	@echo "Platform: $(UNAME_S)"

# Include dependency files
-include $(DEPS)
