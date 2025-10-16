# Makefile for Torpedo (Battleship) game

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -O0 -ggdb
LIBS = -lpthread

# Directories
SRC_DIR = src
BIN_DIR = bin
BUILD_DIR = build

# Source files
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/arguments.c \
          $(SRC_DIR)/baudrate.c \
          $(SRC_DIR)/serial.c \
          $(SRC_DIR)/game.c

# Object files
OBJECTS = $(BUILD_DIR)/main.o \
          $(BUILD_DIR)/arguments.o \
          $(BUILD_DIR)/baudrate.o \
          $(BUILD_DIR)/serial.o \
          $(BUILD_DIR)/game.o

# Target executable
TARGET = $(BIN_DIR)/torpedo

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(BUILD_DIR)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	@echo "Build complete: $(TARGET)"

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build (with DEBUG flag)
debug: CFLAGS += -DDEBUG
debug: clean all

# Release build (optimized)
release: CFLAGS = -Wall -Wextra -O2
release: clean all

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)/*.o
	rm -f $(TARGET)
	@echo "Clean complete"

# Clean all generated directories
distclean: clean
	rm -rf $(BUILD_DIR)
	rm -rf $(BIN_DIR)
	@echo "Distribution clean complete"

# Run the program
run: $(TARGET)
	$(TARGET) -s dev=/dev/ttyACM1,speed=115200 -g x=10,y=5,ship_cnt_1=2,ship_cnt_2=3,ship_cnt_3=1

# Install (optional, copies to /usr/local/bin)
install: $(TARGET)
	@echo "Installing to /usr/local/bin (requires sudo)"
	sudo cp $(TARGET) /usr/local/bin/torpedo
	@echo "Installation complete"

# Uninstall
uninstall:
	@echo "Removing from /usr/local/bin (requires sudo)"
	sudo rm -f /usr/local/bin/torpedo
	@echo "Uninstall complete"

# Show help
help:
	@echo "Torpedo (Battleship) Game - Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all       - Build the project (default)"
	@echo "  debug     - Build with DEBUG flag enabled"
	@echo "  release   - Build optimized release version"
	@echo "  clean     - Remove build artifacts"
	@echo "  distclean - Remove all generated files and directories"
	@echo "  run       - Build and run with default parameters"
	@echo "  install   - Install to /usr/local/bin (requires sudo)"
	@echo "  uninstall - Remove from /usr/local/bin (requires sudo)"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Example usage:"
	@echo "  make              # Build the project"
	@echo "  make debug        # Build with debug output"
	@echo "  make run          # Build and run"
	@echo "  make clean        # Clean build artifacts"

# Phony targets (not actual files)
.PHONY: all directories debug release clean distclean run install uninstall help
