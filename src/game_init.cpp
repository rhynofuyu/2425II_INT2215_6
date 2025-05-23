#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <dirent.h>

#include "include/game_structures.h"
#include "include/texture_manager.h"
#include "include/game_resources.h"
#include "include/game_init.h"
#include "include/renderer.h"
#include "include/solver.h"

bool checkWinCondition(Level* level);

extern int totalLoadedLevels;
extern int currentLevelIndex;
extern int solverNodesExplored;
extern int solverMaxQueueSize;
extern int solverExecutionTimeMs;

int currentMenuSelection = MENU_START_GAME;
int currentSettingsSelection = SETTINGS_BACKGROUND_MUSIC;
bool showingTutorial = false;
int currentSkinSelection = SKIN_DEFAULT;
Uint32 levelCompleteTime = 0;
bool showLevelCompleteAnim = false;
bool solverActive = false;
bool solverRunning = false;
bool solverFoundSolution = false;
std::vector<char> solverSolution;
size_t currentSolutionStep = 0;
Uint32 lastSolutionStepTime = 0;
const Uint32 SOLUTION_STEP_DELAY = 300;
bool showSolverStats = false;
SDL_Window* window = nullptr;

SDL_Texture* menuBackgroundTexture = nullptr;
SDL_Texture* levelSelectBackgroundTexture = nullptr;
SDL_Texture* gameLevelBackgroundTexture = nullptr;
SDL_Texture* tutorialTexture = nullptr;

std::vector<std::string> dynamicLevelFiles;

void initGame() {
    game.currentState = MENU;
    
    scanLevelsDirectory("levels");
    
    if (totalLoadedLevels > 0) {
        if (!loadLevelFromFile(dynamicLevelFiles[currentLevelIndex].c_str(), &game.activeLevel)) {
            std::cerr << "Error: Failed to load level from " << dynamicLevelFiles[currentLevelIndex] << std::endl;
            exit(-1);
        }
    } else {
        std::cerr << "No level files found in 'levels' directory" << std::endl;
        exit(-1);
    }
}

bool initMenuBackground(SDL_Renderer* renderer) {
    SDL_Surface* menuBackgroundSurface = IMG_Load("assets/images/menu/menu_background.png");
    if (menuBackgroundSurface) {
        menuBackgroundTexture = SDL_CreateTextureFromSurface(renderer, menuBackgroundSurface);
        SDL_FreeSurface(menuBackgroundSurface);
        if (!menuBackgroundTexture) {
            std::cerr << "Failed to create menu background texture" << std::endl;
            return false;
        }
    }

    SDL_Surface* levelSelectBackgroundSurface = IMG_Load("assets/images/menu/level_background.png");
    if (levelSelectBackgroundSurface) {
        levelSelectBackgroundTexture = SDL_CreateTextureFromSurface(renderer, levelSelectBackgroundSurface);
        SDL_FreeSurface(levelSelectBackgroundSurface);
        if (!levelSelectBackgroundTexture) {
            std::cerr << "Failed to create level select background texture" << std::endl;
            return false;
        }
    }
    
    SDL_Surface* gameLevelBackgroundSurface = IMG_Load("assets/images/menu/game_background.png");
    
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

bool initTutorialImage(SDL_Renderer* renderer) {
    SDL_Surface* tutorialSurface = IMG_Load("assets/images/tutorial/guide.png");
    
    if (tutorialSurface) {
        tutorialTexture = SDL_CreateTextureFromSurface(renderer, tutorialSurface);
        SDL_FreeSurface(tutorialSurface);
        
        if (!tutorialTexture) {
            std::cerr << "Failed to create tutorial texture" << std::endl;
            return false;
        }
        
        return true;
    }
    
    std::cerr << "Failed to load tutorial image" << std::endl;
    return false;
}

void cleanupMenuResources() {
    for (auto& texture : {&menuBackgroundTexture, &levelSelectBackgroundTexture, 
                          &gameLevelBackgroundTexture, &tutorialTexture}) {
        if (*texture) {
            SDL_DestroyTexture(*texture);
            *texture = nullptr;
        }
    }
}

void scanLevelsDirectory(const std::string& path) {
    dynamicLevelFiles.clear();
    totalLoadedLevels = 0;
    
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        std::cerr << "Error opening: " << path << std::endl;
        return;
    }
    
    for (struct dirent* entry; (entry = readdir(dir));) {
        std::string file = entry->d_name;
        if (file.size() > 4 && file.substr(file.size() - 4) == ".txt") {
            dynamicLevelFiles.push_back(path + "/" + file);
            totalLoadedLevels++;
        }
    }
    closedir(dir);
    
    std::sort(dynamicLevelFiles.begin(), dynamicLevelFiles.end(), 
        [](const auto& a, const auto& b) {
            std::string fileA = a.substr(a.find_last_of('/') + 1);
            std::string fileB = b.substr(b.find_last_of('/') + 1);
            return std::stoi(fileA.substr(5, fileA.length() - 9)) < 
                   std::stoi(fileB.substr(5, fileB.length() - 9));
        });
    
    std::cout << "Loaded " << totalLoadedLevels << " levels from " << path << std::endl;
}

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    if (TTF_Init() == -1) {
        std::cout << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return false;
    }
    
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cout << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
    }

    return true;
}

void cleanupSDL() {
    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

bool createWindowAndRenderer(SDL_Window** win, SDL_Renderer** renderer) {
    *win = SDL_CreateWindow(
        "Sokoban Game",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN
    );
    
    if (!*win) {
        std::cout << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    *renderer = SDL_CreateRenderer(
        *win,
        -1,
        SDL_RENDERER_ACCELERATED
    );
    
    if (!*renderer) {
        std::cout << "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(*win);
        return false;
    }
    
    window = *win;
    return true;
}

void destroyWindowAndRenderer(SDL_Window* win, SDL_Renderer* renderer) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
}

bool initGameResources(SDL_Renderer* renderer) {
    TTF_Font* font = TTF_OpenFont("assets/fonts/arial.ttf", 24);
    TTF_Font* largeFont = TTF_OpenFont("assets/fonts/arial.ttf", 48);
    
    if (!font || !largeFont) {
        std::cout << "Failed to load fonts! TTF_Error: " << TTF_GetError() << std::endl;
        return false;
    }
    
    TTF_CloseFont(font);
    TTF_CloseFont(largeFont);
    
    if (!loadHighScores("highscores.dat")) {
        std::cout << "No high score file found, will create one when scores are saved." << std::endl;
    }
    
    if (!loadSettings("game_settings.dat")) {
        std::cout << "No settings file found, using default settings." << std::endl;
    }
    
    if (!gameTextures.loadTextures(renderer)) {
        std::cout << "Failed to load game textures!" << std::endl;
        return false;
    }
    
    backgroundMusic = Mix_LoadMUS("assets/sounds/bgm/background.mp3");
    if (!backgroundMusic) {
        std::cout << "Failed to load background music! SDL_mixer Error: " << Mix_GetError() << std::endl;
    }
    else if (game.settings.bgmEnabled) {
        Mix_PlayMusic(backgroundMusic, -1);
    }
    
    soundEffects[0] = Mix_LoadWAV("assets/sounds/move.wav");
    soundEffects[1] = Mix_LoadWAV("assets/sounds/push.wav"); 
    soundEffects[2] = Mix_LoadWAV("assets/sounds/complete.wav");
    
    if (!initMenuBackground(renderer)) {
        std::cout << "Failed to initialize menu background!" << std::endl;
        return false;
    }
    
    if (!initTutorialImage(renderer)) {
        std::cout << "Failed to initialize tutorial image!" << std::endl;
        return false;
    }
    
    if (game.settings.fullscreenEnabled) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    
    return true;
}

void cleanupGameResources() {
    cleanupMenuResources();
    gameTextures.destroyTextures();
    if (backgroundMusic) {
        Mix_FreeMusic(backgroundMusic);
    }
    for (int i = 0; i < 3; i++) {
        if (soundEffects[i]) {
            Mix_FreeChunk(soundEffects[i]);
        }
    }
}

void updateGame() {
    if (game.currentState == PLAYING && checkWinCondition(&game.activeLevel)) {
        game.isNewRecord = isNewHighScore(currentLevelIndex, game.player.moves, game.player.pushes);
        
        if (game.isNewRecord) {
            saveHighScores("highscores.dat");
        }
        
        solverActive = false;
        solverRunning = false;
        solverFoundSolution = false;
        solverSolution.clear();
        currentSolutionStep = 0;
        showSolverStats = false;
        
        game.currentState = LEVEL_COMPLETE;
    }
}

void renderGame(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
    SDL_RenderClear(renderer);
    
    TTF_Font* font = TTF_OpenFont("assets/fonts/arial.ttf", 24);
    TTF_Font* largeFont = TTF_OpenFont("assets/fonts/arial.ttf", 48);
    
    if (!font || !largeFont) {
        std::cerr << "Failed to load fonts for rendering" << std::endl;
        return;
    }
    
    if (game.currentState == MENU) {
        renderMenu(renderer, font);
    } else if (game.currentState == PLAYING) {
        if (gameLevelBackgroundTexture) {
            SDL_RenderCopy(renderer, gameLevelBackgroundTexture, nullptr, nullptr);
        }
        
        renderLevel(renderer, game.activeLevel, game.player, gameTextures);
        
        renderHUD(renderer, font, currentLevelIndex + 1, game.player.moves, game.player.pushes);
        
        renderSolverStatus(renderer, font);
    } else if (game.currentState == LEVEL_COMPLETE) {
        if (gameLevelBackgroundTexture) {
            SDL_RenderCopy(renderer, gameLevelBackgroundTexture, nullptr, nullptr);
        }
        
        renderLevel(renderer, game.activeLevel, game.player, gameTextures);
        
        renderLevelComplete(renderer, font, largeFont, currentLevelIndex + 1, game.player.moves, game.player.pushes);
    } else if (game.currentState == GAME_OVER) {
        renderGameComplete(renderer, font, largeFont, game.player.moves, game.player.pushes);
    } else if (game.currentState == LEVEL_SELECT) {
        renderLevelSelect(renderer, font);
    } else if (game.currentState == SETTINGS) {
        renderSettings(renderer, font);
        
        if (showingTutorial) {
            renderTutorial(renderer);
        }
    } else if (game.currentState == SKIN_SELECT) {
        renderSkinSelect(renderer, font);
    }
    
    SDL_RenderPresent(renderer);
    
    TTF_CloseFont(font);
    TTF_CloseFont(largeFont);
}
