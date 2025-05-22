#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h> 
#include <iostream>

#include "src/include/game_structures.h"
#include "src/include/texture_manager.h"
#include "src/include/solver.h"
#include "src/include/game_resources.h"
#include "src/include/renderer.h"
#include "src/include/input_handler.h"
#include "src/include/game_init.h"

int main(int argc, char* argv[]) {
    if (!initSDL()) {
        std::cout << "Failed to initialize SDL!" << std::endl;
        return -1;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!createWindowAndRenderer(&window, &renderer)) {
        std::cout << "Failed to create window or renderer!" << std::endl;
        cleanupSDL();
        return -1;
    }

    if (!initGameResources(renderer)) {
        std::cout << "Failed to initialize game resources!" << std::endl;
        cleanupGameResources();
        destroyWindowAndRenderer(window, renderer);
        cleanupSDL();
        return -1;
    }
    
    initGame();
    
    bool running = true;
    SDL_Event event;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            
            if (event.type == SDL_KEYDOWN) {
                handleInput(event);
            }
        }

        updateGame();
        
        renderGame(renderer);
        
        SDL_Delay(16);
    }

    saveHighScores("highscores.dat");
    cleanupGameResources();
    destroyWindowAndRenderer(window, renderer);
    cleanupSDL();

    return 0;
}
