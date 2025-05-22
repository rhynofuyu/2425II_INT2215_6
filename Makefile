CC = g++
CFLAGS = -Wall -std=c++17 -Dmain=SDL_main -I./src/include
LDFLAGS = -L./src/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

SOURCES = main.cpp \
          src/game_structures.cpp \
          src/texture_manager.cpp \
          src/solver.cpp \
          src/game_resources.cpp \
          src/renderer.cpp \
          src/input_handler.cpp \
          src/game_init.cpp

EXECUTABLE = main.exe

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LDFLAGS)

run: $(EXECUTABLE)
	./$(EXECUTABLE)

clean:
	rm -f $(EXECUTABLE)
