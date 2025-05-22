#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h> 
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <cmath>
#include <map>
#include <algorithm>
#include <unistd.h>
#include <random>
#include <fstream>
#include <dirent.h>
#include <thread>

#include "src/include/game_structures.h"
#include "src/include/texture_manager.h"
#include "src/include/solver.h"
#include "src/include/game_resources.h"
#include "src/include/renderer.h" // Added the new header

void initGame();
bool initMenuBackground(SDL_Renderer* renderer);
bool initTutorialImage(SDL_Renderer* renderer);
void handleInput(SDL_Event& event);
void cleanupMenuResources();
void scanLevelsDirectory(const std::string& levelDirPath);

// Function declarations (implementations moved to renderer.cpp)
void renderMenu(SDL_Renderer* renderer, TTF_Font* font);
void renderHUD(SDL_Renderer* renderer, TTF_Font* font, int levelNum, int moves, int pushes);
void renderLevelComplete(SDL_Renderer* renderer, TTF_Font* normalFont, TTF_Font* largeFont, int levelNum, int moves, int pushes);
void renderGameComplete(SDL_Renderer* renderer, TTF_Font* normalFont, TTF_Font* largeFont, int moves, int pushes);
void renderLevelSelect(SDL_Renderer* renderer, TTF_Font* font);
void renderSettings(SDL_Renderer* renderer, TTF_Font* font);
void renderSkinSelect(SDL_Renderer* renderer, TTF_Font* font);
void renderTutorial(SDL_Renderer* renderer);
void renderSolverStatus(SDL_Renderer* renderer, TTF_Font* font);
void renderText(SDL_Renderer* renderer, const char* text, int x, int y, TTF_Font* font, SDL_Color textColor);

bool checkWinCondition(Level* level);

const int TILE_SIZE = 40;

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
extern int solverNodesExplored;
extern int solverMaxQueueSize;
extern int solverExecutionTimeMs;

SDL_Window* window = nullptr;

SDL_Texture* menuBackgroundTexture = nullptr;
SDL_Texture* levelSelectBackgroundTexture = nullptr;
SDL_Texture* gameLevelBackgroundTexture = nullptr;
SDL_Texture* tutorialTexture = nullptr;

std::vector<std::string> dynamicLevelFiles;
extern int totalLoadedLevels;
extern int currentLevelIndex;

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    if (TTF_Init() == -1) {
        std::cout << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return -1;
    }
    
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cout << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
    }

    window = SDL_CreateWindow(
        "Sokoban Game",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    TTF_Font* font = TTF_OpenFont("assets/fonts/arial.ttf", 24);
    TTF_Font* largeFont = TTF_OpenFont("assets/fonts/arial.ttf", 48);
    
    if (!loadHighScores("highscores.dat")) {
        std::cout << "No high score file found, will create one when scores are saved." << std::endl;
    }
    
    if (!loadSettings("game_settings.dat")) {
        std::cout << "No settings file found, using default settings." << std::endl;
    }
    
    initGame();
    
    if (!gameTextures.loadTextures(renderer)) {
        std::cout << "Failed to load game textures!" << std::endl;
        TTF_CloseFont(largeFont);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return -1;
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
        TTF_CloseFont(largeFont);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return -1;
    }
    
    if (!initTutorialImage(renderer)) {
        std::cout << "Failed to initialize tutorial image!" << std::endl;
    }
    
    if (!initTutorialImage(renderer)) {
        std::cout << "Failed to initialize tutorial image!" << std::endl;
        TTF_CloseFont(largeFont);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return -1;
    }
    
    if (game.settings.fullscreenEnabled) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }

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

        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
        SDL_RenderClear(renderer);
        
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
        
        SDL_Delay(16);
        
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

    saveHighScores("highscores.dat");

    cleanupMenuResources();
    TTF_CloseFont(largeFont);
    TTF_CloseFont(font);
    gameTextures.destroyTextures();
    if (backgroundMusic) {
        Mix_FreeMusic(backgroundMusic);
    }
    for (int i = 0; i < 3; i++) {
        if (soundEffects[i]) {
            Mix_FreeChunk(soundEffects[i]);
        }
    }
    Mix_CloseAudio();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}

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

void handleInput(SDL_Event& event) {
    if (event.type != SDL_KEYDOWN) {
        return;
    }
    
    if (game.currentState == MENU) {
        switch (event.key.keysym.sym) {
            case SDLK_UP:
                do {
                    currentMenuSelection = (currentMenuSelection - 1 + MENU_COUNT) % MENU_COUNT;
                } while (currentMenuSelection == MENU_SELECT_LEVEL && !totalLoadedLevels);
                break;
            case SDLK_DOWN:
                do {
                    currentMenuSelection = (currentMenuSelection + 1) % MENU_COUNT;
                } while (currentMenuSelection == MENU_SELECT_LEVEL && !totalLoadedLevels);
                break;
            case SDLK_RETURN:
            case SDLK_SPACE:
                if (currentMenuSelection == MENU_START_GAME) {
                    currentLevelIndex = 0;
                    if (!loadLevelFromFile(dynamicLevelFiles[currentLevelIndex].c_str(), &game.activeLevel)) {
                        std::cerr << "Error: Failed to load level from " << dynamicLevelFiles[currentLevelIndex] << std::endl;
                        exit(-1);
                    }
                    game.moveHistory.clear();
                    game.isNewRecord = false;
                    
                    initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
                    game.currentState = PLAYING;
                } 
                else if (currentMenuSelection == MENU_SELECT_LEVEL) {
                    game.currentState = LEVEL_SELECT;
                }
                else if (currentMenuSelection == MENU_SETTINGS) {
                    game.currentState = SETTINGS;
                }
                else if (currentMenuSelection == MENU_SELECT_SKIN) {
                    game.currentState = SKIN_SELECT;
                }
                else if (currentMenuSelection == MENU_QUIT) {
                    SDL_Event quitEvent;
                    quitEvent.type = SDL_QUIT;
                    SDL_PushEvent(&quitEvent);
                }
                break;
            default:
                break;
        }
        return;
    }
    
    if (game.currentState == LEVEL_SELECT) {
        switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                game.currentState = MENU;
                return;
            
            case SDLK_UP:
                if (currentLevelIndex >= 4) {
                    currentLevelIndex -= 4;
                }
                return;
                
            case SDLK_DOWN:
                if (currentLevelIndex + 4 < totalLoadedLevels) {
                    currentLevelIndex += 4;
                }
                return;
                
            case SDLK_LEFT:
                if (currentLevelIndex > 0) {
                    currentLevelIndex--;
                }
                return;
                
            case SDLK_RIGHT:
                if (currentLevelIndex + 1 < totalLoadedLevels) {
                    currentLevelIndex++;
                }
                return;
                
            case SDLK_PAGEUP:
                if (currentLevelIndex >= 16) {
                    currentLevelIndex -= 16;
                }
                return;
                
            case SDLK_PAGEDOWN:
                if (currentLevelIndex + 16 < totalLoadedLevels) {
                    currentLevelIndex += 16;
                } else if (totalLoadedLevels > 0) {
                    currentLevelIndex = totalLoadedLevels - 1;
                }
                return;
                
            case SDLK_HOME:
                currentLevelIndex = 0;
                return;
                
            case SDLK_END:
                if (totalLoadedLevels > 0) {
                    currentLevelIndex = totalLoadedLevels - 1;
                }
                return;
                
            case SDLK_RETURN:
            case SDLK_SPACE:
                if (!loadLevelFromFile(dynamicLevelFiles[currentLevelIndex].c_str(), &game.activeLevel)) {
                    std::cerr << "Error: Failed to load level from " << dynamicLevelFiles[currentLevelIndex] << std::endl;
                    exit(-1);
                }
                
                game.moveHistory.clear();
                game.isNewRecord = false;
                
                initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
                
                game.currentState = PLAYING;
                return;
        }
        
        return;
    }
    
    if (game.currentState == SETTINGS) {
        switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                if (showingTutorial) {
                    showingTutorial = false;
                    return;
                }
                game.currentState = MENU;
                return;
            
            case SDLK_UP:
                currentSettingsSelection = (currentSettingsSelection - 1 + SETTINGS_COUNT) % SETTINGS_COUNT;
                return;
                
            case SDLK_DOWN:
                currentSettingsSelection = (currentSettingsSelection + 1) % SETTINGS_COUNT;
                return;
                
            case SDLK_LEFT:
                if (currentSettingsSelection == SETTINGS_BACKGROUND_MUSIC) {
                    game.settings.bgmEnabled = !game.settings.bgmEnabled;
                    if (game.settings.bgmEnabled && backgroundMusic) {
                        Mix_PlayMusic(backgroundMusic, -1);
                    } else {
                        Mix_HaltMusic();
                    }
                } else if (currentSettingsSelection == SETTINGS_SOUND_EFFECTS) {
                    game.settings.sfxEnabled = !game.settings.sfxEnabled;
                }
                return;
                
            case SDLK_RIGHT:
                if (currentSettingsSelection == SETTINGS_BACKGROUND_MUSIC) {
                    game.settings.bgmEnabled = !game.settings.bgmEnabled;
                    if (game.settings.bgmEnabled && backgroundMusic) {
                        Mix_PlayMusic(backgroundMusic, -1);
                    } else {
                        Mix_HaltMusic();
                    }
                } else if (currentSettingsSelection == SETTINGS_SOUND_EFFECTS) {
                    game.settings.sfxEnabled = !game.settings.sfxEnabled;
                }
                return;
                
            case SDLK_RETURN:
            case SDLK_SPACE:
                if (currentSettingsSelection == SETTINGS_TUTORIALS) {
                    showingTutorial = true;
                }
                else if (currentSettingsSelection == SETTINGS_BACK) {
                    game.currentState = MENU;
                }
                return;
        }
        
        return;
    }
    
    if (game.currentState == SKIN_SELECT) {
        switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                game.currentState = MENU;
                return;
            
            case SDLK_UP:
                currentSkinSelection = SKIN_COUNT;
                return;
                
            case SDLK_DOWN:
                if (currentSkinSelection == SKIN_COUNT) {
                    currentSkinSelection = 0;
                }
                return;
                
            case SDLK_LEFT:
                if (currentSkinSelection < SKIN_COUNT) {
                    currentSkinSelection = (currentSkinSelection - 1 + SKIN_COUNT) % SKIN_COUNT;
                }
                return;
                
            case SDLK_RIGHT:
                if (currentSkinSelection < SKIN_COUNT) {
                    currentSkinSelection = (currentSkinSelection + 1) % SKIN_COUNT;
                }
                return;
                
            case SDLK_RETURN:
            case SDLK_SPACE:
                if (currentSkinSelection < SKIN_COUNT) {
                    game.settings.currentSkin = static_cast<PlayerSkin>(currentSkinSelection);
                    saveSettings("game_settings.dat");
                } 
                game.currentState = MENU;
                return;
        }
        
        return;
    }
    
    if (game.currentState == LEVEL_COMPLETE) {
        if (event.key.keysym.sym == SDLK_SPACE) {
            currentLevelIndex++;
            
            if (currentLevelIndex < totalLoadedLevels) {
                if (!loadLevelFromFile(dynamicLevelFiles[currentLevelIndex].c_str(), &game.activeLevel)) {
                    std::cerr << "Error: Failed to load level from " << dynamicLevelFiles[currentLevelIndex] << std::endl;
                    exit(-1);
                }
                
                game.moveHistory.clear();
                game.isNewRecord = false;
                
                initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
                
                game.currentState = PLAYING;
            } else {
                game.currentState = GAME_OVER;
            }
        }
        return;
    }
    
    if (game.currentState == GAME_OVER) {
        switch(event.key.keysym.sym) {
            case SDLK_ESCAPE:
                game.currentState = MENU;
                break;
            case SDLK_q:
                SDL_Event quitEvent;
                quitEvent.type = SDL_QUIT;
                SDL_PushEvent(&quitEvent);
                break;
        }
        return;
    }
    
    if (game.currentState == PLAYING) {
        int dx = 0, dy = 0;
        
        switch (event.key.keysym.sym) {
            case SDLK_UP:
                dy = -1;
                break;
            case SDLK_DOWN:
                dy = 1;
                break;
            case SDLK_LEFT:
                dx = -1;
                break;
            case SDLK_RIGHT:
                dx = 1;
                break;
            case SDLK_z:
                undoMove();
                return;
            case SDLK_r:
                initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
                game.moveHistory.clear();
                return;
            case SDLK_n:
                if (currentLevelIndex < totalLoadedLevels - 1) {
                    currentLevelIndex++;
                    game.moveHistory.clear();
                    game.isNewRecord = false;
                    if (!loadLevelFromFile(dynamicLevelFiles[currentLevelIndex].c_str(), &game.activeLevel)) {
                        std::cerr << "Error: Failed to load level from " << dynamicLevelFiles[currentLevelIndex] << std::endl;
                        exit(-1);
                    }
                    initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
                }
                return;
            case SDLK_p:
                if (currentLevelIndex > 0) {
                    currentLevelIndex--;
                    game.moveHistory.clear();
                    game.isNewRecord = false;
                    if (!loadLevelFromFile(dynamicLevelFiles[currentLevelIndex].c_str(), &game.activeLevel)) {
                        std::cerr << "Error: Failed to load level from " << dynamicLevelFiles[currentLevelIndex] << std::endl;
                        exit(-1);
                    }
                    initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
                }
                return;
            case SDLK_ESCAPE:
                game.currentState = MENU;
                return;
            case SDLK_s:
                if (!solverRunning) {
                    solverRunning = true;
                    solverActive = true;
                    solverFoundSolution = false;
                    solverSolution.clear();
                    currentSolutionStep = 0;
                    showSolverStats = true;

                    Uint32 startTime = SDL_GetTicks();
                    solverFoundSolution = solveLevel(game.activeLevel, solverSolution, solverNodesExplored, solverMaxQueueSize);
                    solverExecutionTimeMs = SDL_GetTicks() - startTime;
                    solverRunning = false;
                }
                return;
            case SDLK_a:
                if (solverRunning) {
                    solverRunning = false;
                    solverActive = false;
                    solverSolution.clear();
                    currentSolutionStep = 0;
                }
                return;
            case SDLK_F1:
                if (!solverActive) {
                    solverActive = true;
                    solverRunning = true;
                    solverFoundSolution = false;
                    solverSolution.clear();
                    currentSolutionStep = 0;
                    showSolverStats = true;
                    
                    Uint32 startTime = SDL_GetTicks();
                    solverSolution = solveSokoban(game.activeLevel, game.player.x, game.player.y, solverNodesExplored, solverMaxQueueSize);
                    solverExecutionTimeMs = SDL_GetTicks() - startTime;
                    solverRunning = false;
                    solverFoundSolution = !solverSolution.empty();
                }
                return;

            case SDLK_F3:
                solverActive = false;
                solverRunning = false;
                solverFoundSolution = false;
                solverSolution.clear();
                currentSolutionStep = 0;
                showSolverStats = false;
                return;
                
            case SDLK_i:
                showSolverStats = !showSolverStats;
                return;
                
            default:
                return;
        }
        
        if (dx != 0 || dy != 0) {
            MoveRecord moveRecord;
            moveRecord.playerPos = {game.player.x, game.player.y};
            moveRecord.wasBoxMoved = false;
            
            int targetX = game.player.x + dx;
            int targetY = game.player.y + dy;
            Level& level = game.activeLevel;
            
            if (targetX < 0 || targetX >= level.width || targetY < 0 || targetY >= level.height) {
                return;
            }
            
            TileType targetTile = level.currentMap[targetY][targetX];
            
            if (targetTile == WALL) {
                return;
            }
            else if (targetTile == EMPTY || targetTile == TARGET) {
                bool wasOnTarget = (level.originalMap[game.player.y][game.player.x] == TARGET);
                
                if (wasOnTarget) {
                    level.currentMap[game.player.y][game.player.x] = TARGET;
                } else {
                    level.currentMap[game.player.y][game.player.x] = EMPTY;
                }
                
                game.player.x = targetX;
                game.player.y = targetY;
                
                if (targetTile == TARGET) {
                    level.currentMap[targetY][targetX] = PLAYER_ON_TARGET;
                } else {
                    level.currentMap[targetY][targetX] = PLAYER;
                }
                
                recordMove(moveRecord);
                
                game.player.moves++;
                
                if (game.settings.sfxEnabled && soundEffects[0]) {
                    Mix_PlayChannel(-1, soundEffects[0], 0);
                }
            }
            else if (targetTile == BOX || targetTile == BOX_ON_TARGET) {
                int nextToTargetX = targetX + dx;
                int nextToTargetY = targetY + dy;
                
                if (nextToTargetX < 0 || nextToTargetX >= level.width || nextToTargetY < 0 || nextToTargetY >= level.height) {
                    return;
                }
                
                TileType nextToTargetTile = level.currentMap[nextToTargetY][nextToTargetX];
                
                if (nextToTargetTile == WALL || nextToTargetTile == BOX || nextToTargetTile == BOX_ON_TARGET) {
                    return;
                }
                
                moveRecord.wasBoxMoved = true;
                moveRecord.boxPrevPos = {targetX, targetY};
                moveRecord.movedBoxPos = {nextToTargetX, nextToTargetY};
                
                if (nextToTargetTile == TARGET) {
                    level.currentMap[nextToTargetY][nextToTargetX] = BOX_ON_TARGET;
                } else {
                    level.currentMap[nextToTargetY][nextToTargetX] = BOX;
                }
                
                bool boxWasOnTarget = (level.originalMap[targetY][targetX] == TARGET);
                
                if (boxWasOnTarget) {
                    level.currentMap[targetY][targetX] = PLAYER_ON_TARGET;
                } else {
                    level.currentMap[targetY][targetX] = PLAYER;
                }
                
                bool playerWasOnTarget = (level.originalMap[game.player.y][game.player.x] == TARGET);
                
                if (playerWasOnTarget) {
                    level.currentMap[game.player.y][game.player.x] = TARGET;
                } else {
                    level.currentMap[game.player.y][game.player.x] = EMPTY;
                }
                
                game.player.x = targetX;
                game.player.y = targetY;
                
                recordMove(moveRecord);
                
                game.player.moves++;
                game.player.pushes++;
                
                if (game.settings.sfxEnabled && soundEffects[1]) {
                    Mix_PlayChannel(-1, soundEffects[1], 0);
                }
            }
        }
    }
}

bool checkWinCondition(Level* level) {
    int targetCount = 0;
    int boxOnTargetCount = 0;
    
    for (int y = 0; y < level->height; y++) {
        for (int x = 0; x < level->width; x++) {
            if (level->originalMap[y][x] == TARGET) {
                targetCount++;
            }
            
            if (level->currentMap[y][x] == BOX_ON_TARGET) {
                boxOnTargetCount++;
            }
        }
    }
    
    return (targetCount > 0 && boxOnTargetCount == targetCount);
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
