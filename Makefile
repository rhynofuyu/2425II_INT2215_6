# Makefile for Sokoban Game on Windows

# Compiler settings
CXX = g++
CXXFLAGS = -Wall -std=c++17 -Dmain=SDL_main

# Include and library paths
INCLUDES = -I./src/include
LIBS = -L./src/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

# Source and output files
SRCS := main.cpp src/game_structures.cpp src/texture_manager.cpp src/solver.cpp src/game_resources.cpp src/renderer.cpp
OUTPUT = main.exe

# Build rule
all: $(OUTPUT)

$(OUTPUT): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

# Clean rule
clean:
	del $(OUTPUT)

# Run rule
run: $(OUTPUT)
	.\$(OUTPUT)

.PHONY: run