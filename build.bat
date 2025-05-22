@echo off
g++ -Wall -std=c++17 -Dmain=SDL_main -I./src/include main.cpp src/game_structures.cpp src/texture_manager.cpp src/solver.cpp src/game_resources.cpp src/renderer.cpp -o main.exe -L./src/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

if %ERRORLEVEL% EQU 0 (
    echo Build successful! Running the game...
    main.exe
)
