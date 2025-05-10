#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include "include/game_init.h"
#include "include/game_structures.h"
#include "include/game_logic.h"

// Biến toàn cục
SDL_Window* window = nullptr;
Mix_Music* backgroundMusic = nullptr;
Mix_Chunk* soundEffects[3] = {nullptr}; // 0: move, 1: push, 2: success
SDL_Texture* menuBackgroundTexture = nullptr;
SDL_Texture* levelSelectBackgroundTexture = nullptr;
SDL_Texture* gameLevelBackgroundTexture = nullptr;

// Tên của các skin người chơi
const char* playerSkinNames[SKIN_COUNT][2] = {
    {"assets/images/players/default/player.png", "assets/images/players/default/player_on_target.png"},
    {"assets/images/players/alt1/player.png", "assets/images/players/alt1/player_on_target.png"},
    {"assets/images/players/alt2/player.png", "assets/images/players/alt2/player_on_target.png"},
    {"assets/images/players/alt3/player.png", "assets/images/players/alt3/player_on_target.png"},
    {"assets/images/players/alt4/player.png", "assets/images/players/alt4/player_on_target.png"}
};

// Initialize game data with level loading from file
void initGame() {
    // Initialize game state
    game.currentState = MENU;
    
    // Load the current level from file to prepare it
    if (!loadLevelFromFile(levelFileNames[currentLevelIndex].c_str(), &game.activeLevel)) {
        std::cerr << "Error: Failed to load level from " << levelFileNames[currentLevelIndex] << std::endl;
        exit(-1);
    }
    
    // We don't initialize the level and player until the game actually starts from the menu
}

bool initSDL() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        std::cout << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return false;
    }
    
    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cout << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        // Continue without audio
    }
    
    return true;
}

bool createWindowAndRenderer(SDL_Window** window, SDL_Renderer** renderer) {
    // Create window
    *window = SDL_CreateWindow(
        "Sokoban Game",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN
    );

    if (!*window) {
        std::cout << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create renderer
    *renderer = SDL_CreateRenderer(
        *window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    if (!*renderer) {
        std::cout << "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(*window);
        return false;
    }
    
    return true;
}

bool loadFonts(TTF_Font** normalFont, TTF_Font** largeFont) {
    *normalFont = TTF_OpenFont("assets/fonts/arial.ttf", 24);
    *largeFont = TTF_OpenFont("assets/fonts/arial.ttf", 48);
    if (!*normalFont || !*largeFont) {
        std::cout << "Failed to load font! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return false;
    }
    return true;
}

bool loadAudio() {
    // Load audio files
    backgroundMusic = Mix_LoadMUS("assets/sounds/bgm/background.mp3");
    if (!backgroundMusic) {
        std::cout << "Failed to load background music! SDL_mixer Error: " << Mix_GetError() << std::endl;
    }
    else if (game.settings.bgmEnabled) {
        Mix_PlayMusic(backgroundMusic, -1);  // -1 means loop indefinitely
    }
    
    // Load sound effects
    soundEffects[0] = Mix_LoadWAV("assets/sounds/move.wav");
    soundEffects[1] = Mix_LoadWAV("assets/sounds/push.wav"); 
    soundEffects[2] = Mix_LoadWAV("assets/sounds/complete.wav");
    
    return (soundEffects[0] != nullptr && soundEffects[1] != nullptr && soundEffects[2] != nullptr);
}

// Initialize menu background
bool initMenuBackground(SDL_Renderer* renderer) {
    // Load menu background image
    SDL_Surface* menuBackgroundSurface = IMG_Load("assets/images/menu/menu_background.png");
    if (menuBackgroundSurface) {
        menuBackgroundTexture = SDL_CreateTextureFromSurface(renderer, menuBackgroundSurface);
        SDL_FreeSurface(menuBackgroundSurface);
        if (!menuBackgroundTexture) {
            std::cerr << "Failed to create menu background texture" << std::endl;
            return false;
        }
    }
    
    // Load level select background image
    SDL_Surface* levelSelectBackgroundSurface = IMG_Load("assets/images/menu/level_background.png");
    if (levelSelectBackgroundSurface) {
        levelSelectBackgroundTexture = SDL_CreateTextureFromSurface(renderer, levelSelectBackgroundSurface);
        SDL_FreeSurface(levelSelectBackgroundSurface);
        if (!levelSelectBackgroundTexture) {
            std::cerr << "Failed to create level select background texture" << std::endl;
            return false;
        }
    }
    
    // Load game level background
    // First try to load a dedicated game background
    SDL_Surface* gameLevelBackgroundSurface = IMG_Load("assets/images/menu/game_background.png");
    
    // If the dedicated background doesn't exist, fall back to the level select background
    if (!gameLevelBackgroundSurface) {
        gameLevelBackgroundSurface = IMG_Load("assets/images/menu/level_background.png");
        std::cout << "Game level background image not found, using level background instead." << std::endl;
    }
    
    if (gameLevelBackgroundSurface) {
        gameLevelBackgroundTexture = SDL_CreateTextureFromSurface(renderer, gameLevelBackgroundSurface);
        SDL_FreeSurface(gameLevelBackgroundSurface);
        if (!gameLevelBackgroundTexture) {
            std::cerr << "Failed to create game level background texture" << std::endl;
            return false;
        }
    }

    return true;
}

// Clean up menu resources
void cleanupMenuResources() {
    if (menuBackgroundTexture) {
        SDL_DestroyTexture(menuBackgroundTexture);
        menuBackgroundTexture = nullptr;
    }

    if (levelSelectBackgroundTexture) {
        SDL_DestroyTexture(levelSelectBackgroundTexture);
        levelSelectBackgroundTexture = nullptr;
    }
    
    if (gameLevelBackgroundTexture) {
        SDL_DestroyTexture(gameLevelBackgroundTexture);
        gameLevelBackgroundTexture = nullptr;
    }
}

// loadSettings and saveSettings are defined in game_structures.cpp
// Removed duplicate definitions here
