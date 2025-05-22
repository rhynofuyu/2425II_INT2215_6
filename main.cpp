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
#include <dirent.h> // Add this for directory operations
#include <thread>   // Add this for solver thread

// Include game components
#include "src/include/game_structures.h"
#include "src/include/texture_manager.h"
#include "src/include/solver.h"

// Function prototypes
void initGame();
bool initMenuBackground(SDL_Renderer* renderer);
bool initTutorialImage(SDL_Renderer* renderer);
void handleInput(SDL_Event& event);
void renderText(SDL_Renderer* renderer, const char* text, int x, int y, TTF_Font* font, SDL_Color textColor);
void cleanupMenuResources();
void scanLevelsDirectory(const std::string& levelDirPath);

// Render functions
void renderMenu(SDL_Renderer* renderer, TTF_Font* font);
void renderHUD(SDL_Renderer* renderer, TTF_Font* font, int levelNum, int moves, int pushes);
void renderLevelComplete(SDL_Renderer* renderer, TTF_Font* normalFont, TTF_Font* largeFont, int levelNum, int moves, int pushes);
void renderGameComplete(SDL_Renderer* renderer, TTF_Font* normalFont, TTF_Font* largeFont, int moves, int pushes);
void renderLevelSelect(SDL_Renderer* renderer, TTF_Font* font);
void renderSettings(SDL_Renderer* renderer, TTF_Font* font);
void renderSkinSelect(SDL_Renderer* renderer, TTF_Font* font);
void renderTutorial(SDL_Renderer* renderer);
void renderSolverStatus(SDL_Renderer* renderer, TTF_Font* font);

// Game logic
bool checkWinCondition(Level* level);

// Global constants
const int TILE_SIZE = 40;  // Tile size in pixels

// Global UI state variables
int currentMenuSelection = MENU_START_GAME;
int currentSettingsSelection = SETTINGS_BACKGROUND_MUSIC;
bool showingTutorial = false; // To track if tutorial image is being shown
int currentSkinSelection = SKIN_DEFAULT;

// Level complete animation variables
Uint32 levelCompleteTime = 0;
bool showLevelCompleteAnim = false;

// Solver variables
bool solverActive = false;
bool solverRunning = false;
bool solverFoundSolution = false;
std::vector<char> solverSolution;
size_t currentSolutionStep = 0;
Uint32 lastSolutionStepTime = 0;
const Uint32 SOLUTION_STEP_DELAY = 300; // 300ms between automated moves
bool showSolverStats = false;           // Flag to toggle solver statistics display
extern int solverNodesExplored;         // Defined in solver.cpp
extern int solverMaxQueueSize;          // Defined in solver.cpp
extern int solverExecutionTimeMs;       // Defined in solver.cpp

// Global window reference for use in other functions
SDL_Window* window = nullptr;

// Global variables for menu background animation
SDL_Texture* menuBackgroundTexture = nullptr;
SDL_Texture* levelSelectBackgroundTexture = nullptr;
SDL_Texture* gameLevelBackgroundTexture = nullptr;  // Background for gameplay levels
SDL_Texture* tutorialTexture = nullptr;  // Tutorial guide image

// Music/sound globals
Mix_Music* backgroundMusic = nullptr;
Mix_Chunk* soundEffects[3] = {nullptr}; // 0: move, 1: push, 2: success

// Player skin image names for each skin
const char* playerSkinNames[SKIN_COUNT][2] = {
    {"assets/images/players/default/player.png", "assets/images/players/default/player_on_target.png"},
    {"assets/images/players/alt1/player.png", "assets/images/players/alt1/player_on_target.png"},
    {"assets/images/players/alt2/player.png", "assets/images/players/alt2/player_on_target.png"},
    {"assets/images/players/alt3/player.png", "assets/images/players/alt3/player_on_target.png"},
    {"assets/images/players/alt4/player.png", "assets/images/players/alt4/player_on_target.png"},
    {"assets/images/players/alt5/player.png", "assets/images/players/alt5/player_on_target.png"},  // Kera Candy
    {"assets/images/players/alt6/player.png", "assets/images/players/alt6/player_on_target.png"},  // Win Sweet
    {"assets/images/players/alt7/player.png", "assets/images/players/alt7/player_on_target.png"}   // When Event
};

// Add the dynamic level files vector but use extern for the variables already defined in game_structures.cpp
std::vector<std::string> dynamicLevelFiles;
extern int totalLoadedLevels; // Changed to extern to avoid multiple definition
extern int currentLevelIndex; // Changed to extern to avoid multiple definition

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        std::cout << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return -1;
    }
    
    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cout << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        // Continue without audio
    }

    // Create window
    window = SDL_CreateWindow(
        "Sokoban Game",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cout << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    if (!renderer) {
        std::cout << "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return -1;
    }
    
    // Load fonts
    TTF_Font* font = TTF_OpenFont("assets/fonts/arial.ttf", 24);
    TTF_Font* largeFont = TTF_OpenFont("assets/fonts/arial.ttf", 48);
    if (!font || !largeFont) {
        std::cout << "Failed to load font! SDL_ttf Error: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return -1;
    }
    
    // Load high scores
    if (!loadHighScores("highscores.dat")) {
        std::cout << "No high score file found, will create one when scores are saved." << std::endl;
    }
    
    // Load settings
    if (!loadSettings("game_settings.dat")) {
        std::cout << "No settings file found, using default settings." << std::endl;
    }
    
    // Initialize game data
    initGame();
    
    // Load game textures
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
    soundEffects[2] = Mix_LoadWAV("assets/sounds/complete.wav");    // Initialize menu background
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
    
    // Initialize tutorial image
    if (!initTutorialImage(renderer)) {
        std::cout << "Failed to initialize tutorial image!" << std::endl;
        // We can continue without the tutorial image
    }
    
    // Initialize tutorial image
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
    
    // Apply fullscreen setting if enabled
    if (game.settings.fullscreenEnabled) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }

    // Main loop
    bool running = true;
    SDL_Event event;

    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            
            // Pass all keyboard events to handleInput, it will handle them based on game state
            if (event.type == SDL_KEYDOWN) {
                handleInput(event);
            }
        }

        // Clear screen with dark gray background
        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
        SDL_RenderClear(renderer);
        
        // Render game elements based on current state
        if (game.currentState == MENU) {
            // Render the menu
            renderMenu(renderer, font);        } else if (game.currentState == PLAYING) {
            // First draw the game background
            if (gameLevelBackgroundTexture) {
                SDL_RenderCopy(renderer, gameLevelBackgroundTexture, nullptr, nullptr);
            }
            
            // Render the active level
            renderLevel(renderer, game.activeLevel, game.player, gameTextures);
            
            // Render HUD
            renderHUD(renderer, font, currentLevelIndex + 1, game.player.moves, game.player.pushes);
            
            // Render solver status
            renderSolverStatus(renderer, font);
        } else if (game.currentState == LEVEL_COMPLETE) {
            // First draw the game level background
            if (gameLevelBackgroundTexture) {
                SDL_RenderCopy(renderer, gameLevelBackgroundTexture, nullptr, nullptr);
            }
            
            // Then render the level
            renderLevel(renderer, game.activeLevel, game.player, gameTextures);
            
            // Finally render the completion overlay and message
            renderLevelComplete(renderer, font, largeFont, currentLevelIndex + 1, game.player.moves, game.player.pushes);
        } else if (game.currentState == GAME_OVER) {
            // Render the game completion screen
            renderGameComplete(renderer, font, largeFont, game.player.moves, game.player.pushes);
        } else if (game.currentState == LEVEL_SELECT) {
            // Render the level selection screen
            renderLevelSelect(renderer, font);        } else if (game.currentState == SETTINGS) {
            // Render the settings menu
            renderSettings(renderer, font);
            
            // If tutorial is being shown, render it on top
            if (showingTutorial) {
                renderTutorial(renderer);
            }
        } else if (game.currentState == SKIN_SELECT) {
            // Render the skin selection screen
            renderSkinSelect(renderer, font);
        }
        
        // Present the renderer
        SDL_RenderPresent(renderer);
        
        // Cap the frame rate
        SDL_Delay(16);  // ~60 FPS
        
        // Check win condition if playing
        if (game.currentState == PLAYING && checkWinCondition(&game.activeLevel)) {
            // Check if this is a new high score
            game.isNewRecord = isNewHighScore(currentLevelIndex, game.player.moves, game.player.pushes);
            
            // Save high scores to file
            if (game.isNewRecord) {
                saveHighScores("highscores.dat");
            }
            
            // Reset solver when level is completed
            solverActive = false;
            solverRunning = false;
            solverFoundSolution = false;
            solverSolution.clear();
            currentSolutionStep = 0;
            showSolverStats = false;
            
            // Change game state to level complete
            game.currentState = LEVEL_COMPLETE;
        }
    }

    // Save high scores before exiting
    saveHighScores("highscores.dat");

    // Clean up
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

// Initialize game data with level loading from file
void initGame() {
    // Initialize game state
    game.currentState = MENU;
    
    // Scan for level files
    scanLevelsDirectory("levels");
    
    // Load the current level from file to prepare it
    if (totalLoadedLevels > 0) {
        if (!loadLevelFromFile(dynamicLevelFiles[currentLevelIndex].c_str(), &game.activeLevel)) {
            std::cerr << "Error: Failed to load level from " << dynamicLevelFiles[currentLevelIndex] << std::endl;
            exit(-1);
        }
    } else {
        std::cerr << "No level files found in 'levels' directory" << std::endl;
        exit(-1);
    }
    
    // We don't initialize the level and player until the game actually starts from the menu
}

// Handle player input
void handleInput(SDL_Event& event) {
    if (event.type != SDL_KEYDOWN) {
        return;
    }
    
    // Handle menu state
    if (game.currentState == MENU) {
        switch (event.key.keysym.sym) {
            case SDLK_UP:
                // Navigate up in the menu
                do {
                    currentMenuSelection = (currentMenuSelection - 1 + MENU_COUNT) % MENU_COUNT;
                } while (currentMenuSelection == MENU_SELECT_LEVEL && !totalLoadedLevels); // Skip level select if no levels
                break;
            case SDLK_DOWN:
                // Navigate down in the menu
                do {
                    currentMenuSelection = (currentMenuSelection + 1) % MENU_COUNT;
                } while (currentMenuSelection == MENU_SELECT_LEVEL && !totalLoadedLevels); // Skip level select if no levels
                break;
            case SDLK_RETURN:
            case SDLK_SPACE:
                // Process menu selection
                if (currentMenuSelection == MENU_START_GAME) {
                    // Start the game with the first level
                    currentLevelIndex = 0;
                    if (!loadLevelFromFile(dynamicLevelFiles[currentLevelIndex].c_str(), &game.activeLevel)) {
                        std::cerr << "Error: Failed to load level from " << dynamicLevelFiles[currentLevelIndex] << std::endl;
                        exit(-1);
                    }
                    // Clear move history
                    game.moveHistory.clear();
                    game.isNewRecord = false;
                    
                    initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
                    game.currentState = PLAYING;
                } 
                else if (currentMenuSelection == MENU_SELECT_LEVEL) {
                    // Go to level select screen
                    game.currentState = LEVEL_SELECT;
                }
                else if (currentMenuSelection == MENU_SETTINGS) {
                    // Go to settings screen
                    game.currentState = SETTINGS;
                }
                else if (currentMenuSelection == MENU_SELECT_SKIN) {
                    // Go to skin selection screen
                    game.currentState = SKIN_SELECT;
                }
                else if (currentMenuSelection == MENU_QUIT) {
                    // Set flag to exit game in main loop
                    SDL_Event quitEvent;
                    quitEvent.type = SDL_QUIT;
                    SDL_PushEvent(&quitEvent); // Fix: Pass the address of the quitEvent
                }
                break;
            default:
                break;
        }
        return;
    }
    
    // Handle level select state
    if (game.currentState == LEVEL_SELECT) {
        switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                // Return to main menu
                game.currentState = MENU;
                return;
            
            case SDLK_UP:
                // Navigate up (to previous row)
                if (currentLevelIndex >= 4) {
                    currentLevelIndex -= 4;
                }
                return;
                
            case SDLK_DOWN:
                // Navigate down (to next row)
                if (currentLevelIndex + 4 < totalLoadedLevels) {
                    currentLevelIndex += 4;
                }
                return;
                
            case SDLK_LEFT:
                // Navigate left
                if (currentLevelIndex > 0) {
                    currentLevelIndex--;
                }
                return;
                
            case SDLK_RIGHT:
                // Navigate right
                if (currentLevelIndex + 1 < totalLoadedLevels) {
                    currentLevelIndex++;
                }
                return;
                
            case SDLK_PAGEUP:
                // Navigate to previous page
                if (currentLevelIndex >= 16) {
                    currentLevelIndex -= 16;
                }
                return;
                
            case SDLK_PAGEDOWN:
                // Navigate to next page
                if (currentLevelIndex + 16 < totalLoadedLevels) {
                    currentLevelIndex += 16;
                } else if (totalLoadedLevels > 0) {
                    // Go to last level if can't go to next page
                    currentLevelIndex = totalLoadedLevels - 1;
                }
                return;
                
            case SDLK_HOME:
                // Go to first level
                currentLevelIndex = 0;
                return;
                
            case SDLK_END:
                // Go to last level
                if (totalLoadedLevels > 0) {
                    currentLevelIndex = totalLoadedLevels - 1;
                }
                return;
                
            case SDLK_RETURN:
            case SDLK_SPACE:
                // Select the current level
                // Load the selected level
                if (!loadLevelFromFile(dynamicLevelFiles[currentLevelIndex].c_str(), &game.activeLevel)) {
                    std::cerr << "Error: Failed to load level from " << dynamicLevelFiles[currentLevelIndex] << std::endl;
                    exit(-1);
                }
                
                // Clear move history
                game.moveHistory.clear();
                game.isNewRecord = false;
                
                // Initialize level and player
                initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
                
                // Change state to playing
                game.currentState = PLAYING;
                return;
        }
        
        return;
    }
      // Handle settings state
    if (game.currentState == SETTINGS) {
        switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                // If tutorial is showing, close it, otherwise return to main menu
                if (showingTutorial) {
                    showingTutorial = false;
                    return;
                }
                // Return to main menu
                game.currentState = MENU;
                return;
            
            case SDLK_UP:
                // Navigate up in settings
                currentSettingsSelection = (currentSettingsSelection - 1 + SETTINGS_COUNT) % SETTINGS_COUNT;
                return;
                
            case SDLK_DOWN:
                // Navigate down in settings
                currentSettingsSelection = (currentSettingsSelection + 1) % SETTINGS_COUNT;
                return;
                
            case SDLK_LEFT:
                // Change setting value to the left
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
                // Change setting value to the right
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
                // Select the current setting
                if (currentSettingsSelection == SETTINGS_TUTORIALS) {
                    // Toggle tutorial display
                    showingTutorial = true;
                }
                else if (currentSettingsSelection == SETTINGS_BACK) {
                    game.currentState = MENU;
                }
                return;
        }
        
        return;
    }
    
    // Handle skin select state
    if (game.currentState == SKIN_SELECT) {
        switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                // Return to main menu
                game.currentState = MENU;
                return;
            
            case SDLK_UP:
                // Navigate to the "Back to Main Menu" option
                currentSkinSelection = SKIN_COUNT;
                return;
                
            case SDLK_DOWN:
                // If on "Back" option, go to first skin, otherwise stay on current skin
                if (currentSkinSelection == SKIN_COUNT) {
                    currentSkinSelection = 0;
                }
                return;
                
            case SDLK_LEFT:
                // Navigate left in skin selection (previous skin)
                if (currentSkinSelection < SKIN_COUNT) {
                    // Only cycle through actual skins, not the back option
                    currentSkinSelection = (currentSkinSelection - 1 + SKIN_COUNT) % SKIN_COUNT;
                }
                return;
                
            case SDLK_RIGHT:
                // Navigate right in skin selection (next skin)
                if (currentSkinSelection < SKIN_COUNT) {
                    // Only cycle through actual skins, not the back option
                    currentSkinSelection = (currentSkinSelection + 1) % SKIN_COUNT;
                }
                return;
                
            case SDLK_RETURN:
            case SDLK_SPACE:
                // Select the current skin
                if (currentSkinSelection < SKIN_COUNT) {
                    game.settings.currentSkin = static_cast<PlayerSkin>(currentSkinSelection);
                    // Save the settings when skin is changed
                    saveSettings("game_settings.dat");
                } 
                game.currentState = MENU;
                return;
        }
        
        return;
    }
    
    // Handle level complete state
    if (game.currentState == LEVEL_COMPLETE) {
        if (event.key.keysym.sym == SDLK_SPACE) {
            // Move to the next level
            currentLevelIndex++;
            
            // Check if there are more levels to play
            if (currentLevelIndex < totalLoadedLevels) {
                // Load the next level
                if (!loadLevelFromFile(dynamicLevelFiles[currentLevelIndex].c_str(), &game.activeLevel)) {
                    std::cerr << "Error: Failed to load level from " << dynamicLevelFiles[currentLevelIndex] << std::endl;
                    exit(-1);
                }
                
                // Clear move history for new level
                game.moveHistory.clear();
                game.isNewRecord = false;
                
                // Initialize the level and player
                initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
                
                // Change state back to playing
                game.currentState = PLAYING;
            } else {
                // All levels completed, game over
                game.currentState = GAME_OVER;
            }
        }
        return;
    }
    
    // Handle game over state
    if (game.currentState == GAME_OVER) {
        switch(event.key.keysym.sym) {
            case SDLK_ESCAPE:
                // Return to menu
                game.currentState = MENU;
                break;
            case SDLK_q:
                // Quit the game
                SDL_Event quitEvent;
                quitEvent.type = SDL_QUIT;
                SDL_PushEvent(&quitEvent); // Fix: Pass the address of the quitEvent
                break;
        }
        return;
    }
    
    // Handle gameplay (PLAYING state)
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
            case SDLK_z:  // Undo last move
                undoMove();
                return;
            case SDLK_r:  // Reset current level
                initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
                game.moveHistory.clear(); // Clear move history on reset
                return;
            case SDLK_n:  // Next level
                if (currentLevelIndex < totalLoadedLevels - 1) {
                    currentLevelIndex++;
                    game.moveHistory.clear(); // Clear move history
                    game.isNewRecord = false;
                    if (!loadLevelFromFile(dynamicLevelFiles[currentLevelIndex].c_str(), &game.activeLevel)) {
                        std::cerr << "Error: Failed to load level from " << dynamicLevelFiles[currentLevelIndex] << std::endl;
                        exit(-1);
                    }
                    initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
                }
                return;
            case SDLK_p:  // Previous level
                if (currentLevelIndex > 0) {
                    currentLevelIndex--;
                    game.moveHistory.clear(); // Clear move history
                    game.isNewRecord = false;
                    if (!loadLevelFromFile(dynamicLevelFiles[currentLevelIndex].c_str(), &game.activeLevel)) {
                        std::cerr << "Error: Failed to load level from " << dynamicLevelFiles[currentLevelIndex] << std::endl;
                        exit(-1);
                    }
                    initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
                }
                return;
            case SDLK_ESCAPE:  // Return to menu
                game.currentState = MENU;
                return;
            case SDLK_s:  // Start solver
                if (!solverRunning) {
                    solverRunning = true;
                    solverActive = true;
                    solverFoundSolution = false;
                    solverSolution.clear();
                    currentSolutionStep = 0;
                    showSolverStats = true; // Show solver stats when solving

                    Uint32 startTime = SDL_GetTicks();
                    solverFoundSolution = solveLevel(game.activeLevel, solverSolution, solverNodesExplored, solverMaxQueueSize);
                    solverExecutionTimeMs = SDL_GetTicks() - startTime;
                    solverRunning = false;
                }
                return;
            case SDLK_a:  // Abort solver
                if (solverRunning) {
                    solverRunning = false;
                    solverActive = false;
                    solverSolution.clear();
                    currentSolutionStep = 0;
                }
                return;
            case SDLK_F1:  // Solve the current level
                if (!solverActive) {
                    solverActive = true;
                    solverRunning = true;
                    solverFoundSolution = false;
                    solverSolution.clear();
                    currentSolutionStep = 0;
                    showSolverStats = true; // Show solver stats when solving
                    
                    Uint32 startTime = SDL_GetTicks();
                    solverSolution = solveSokoban(game.activeLevel, game.player.x, game.player.y, solverNodesExplored, solverMaxQueueSize);
                    solverExecutionTimeMs = SDL_GetTicks() - startTime;
                    solverRunning = false;
                    solverFoundSolution = !solverSolution.empty();
                }
                return;

            case SDLK_F3:  // Reset solver
                solverActive = false;
                solverRunning = false;
                solverFoundSolution = false;
                solverSolution.clear();
                currentSolutionStep = 0;
                showSolverStats = false; // Hide solver stats
                return;
                
            case SDLK_i: // Toggle solver info display
                showSolverStats = !showSolverStats;
                return;
                
            default:
                return;  // Ignore other keys
        }
        
        if (dx != 0 || dy != 0) {
            // Record state before move
            MoveRecord moveRecord;
            moveRecord.playerPos = {game.player.x, game.player.y};
            moveRecord.wasBoxMoved = false;
            
            // Calculate target position
            int targetX = game.player.x + dx;
            int targetY = game.player.y + dy;
            Level& level = game.activeLevel;
            
            // Check if target position is valid
            if (targetX < 0 || targetX >= level.width || targetY < 0 || targetY >= level.height) {
                return;  // Out of bounds
            }
            
            // Get the tile type at the target position
            TileType targetTile = level.currentMap[targetY][targetX];
            
            // Movement logic based on target tile
            if (targetTile == WALL) {
                return;  // Can't move into walls
            }
            else if (targetTile == EMPTY || targetTile == TARGET) {
                // Basic movement - determine what tile to leave behind
                bool wasOnTarget = (level.originalMap[game.player.y][game.player.x] == TARGET);
                
                // Update the current player position tile
                if (wasOnTarget) {
                    level.currentMap[game.player.y][game.player.x] = TARGET;
                } else {
                    level.currentMap[game.player.y][game.player.x] = EMPTY;
                }
                
                // Update the player's position
                game.player.x = targetX;
                game.player.y = targetY;
                
                // Update the new player position tile
                if (targetTile == TARGET) {
                    level.currentMap[targetY][targetX] = PLAYER_ON_TARGET;
                } else {
                    level.currentMap[targetY][targetX] = PLAYER;
                }
                
                // Record the move
                recordMove(moveRecord);
                
                // Increment move counter
                game.player.moves++;
                
                // Play move sound effect
                if (game.settings.sfxEnabled && soundEffects[0]) {
                    Mix_PlayChannel(-1, soundEffects[0], 0);
                }
            }
            else if (targetTile == BOX || targetTile == BOX_ON_TARGET) {
                // Calculate the position behind the box (in the direction of push)
                int nextToTargetX = targetX + dx;
                int nextToTargetY = targetY + dy;
                
                // Check if the position behind the box is valid
                if (nextToTargetX < 0 || nextToTargetX >= level.width || nextToTargetY < 0 || nextToTargetY >= level.height) {
                    return;  // Out of bounds
                }
                
                // Get the tile type at the position behind the box
                TileType nextToTargetTile = level.currentMap[nextToTargetY][nextToTargetX];
                
                // Check if box can be pushed
                if (nextToTargetTile == WALL || nextToTargetTile == BOX || nextToTargetTile == BOX_ON_TARGET) {
                    return;  // Can't push into walls or other boxes
                }
                
                // Record box movement info
                moveRecord.wasBoxMoved = true;
                moveRecord.boxPrevPos = {targetX, targetY};
                moveRecord.movedBoxPos = {nextToTargetX, nextToTargetY};
                
                // Update the position behind the box
                if (nextToTargetTile == TARGET) {
                    level.currentMap[nextToTargetY][nextToTargetX] = BOX_ON_TARGET;
                } else {
                    level.currentMap[nextToTargetY][nextToTargetX] = BOX;
                }
                
                // Determine what the box was on (to update the tile where the player will move to)
                bool boxWasOnTarget = (level.originalMap[targetY][targetX] == TARGET);
                
                // Update the box's original position (where player will move to)
                if (boxWasOnTarget) {
                    level.currentMap[targetY][targetX] = PLAYER_ON_TARGET;
                } else {
                    level.currentMap[targetY][targetX] = PLAYER;
                }
                
                // Determine what the player was on (to leave behind)
                bool playerWasOnTarget = (level.originalMap[game.player.y][game.player.x] == TARGET);
                
                // Update the player's original position
                if (playerWasOnTarget) {
                    level.currentMap[game.player.y][game.player.x] = TARGET;
                } else {
                    level.currentMap[game.player.y][game.player.x] = EMPTY;
                }
                
                // Update player position
                game.player.x = targetX;
                game.player.y = targetY;
                
                // Record the move
                recordMove(moveRecord);
                
                // Increment both moves and pushes
                game.player.moves++;
                game.player.pushes++;
                
                // Play push sound effect
                if (game.settings.sfxEnabled && soundEffects[1]) {
                    Mix_PlayChannel(-1, soundEffects[1], 0);
                }
            }
        }
    }
}

// Check if all boxes are on targets
bool checkWinCondition(Level* level) {
    int targetCount = 0;
    int boxOnTargetCount = 0;
    
    // Count all targets and boxes on targets in the level
    for (int y = 0; y < level->height; y++) {
        for (int x = 0; x < level->width; x++) {
            // Count targets in original map (includes empty targets and those under player)
            if (level->originalMap[y][x] == TARGET) {
                targetCount++;
            }
            
            // Count boxes that are currently on targets
            if (level->currentMap[y][x] == BOX_ON_TARGET) {
                boxOnTargetCount++;
            }
        }
    }
    
    // Win condition: number of boxes on targets equals the total number of targets
    // and there is at least one target in the level
    return (targetCount > 0 && boxOnTargetCount == targetCount);
}

// Render text on the screen
void renderText(SDL_Renderer* renderer, const char* text, int x, int y, TTF_Font* font, SDL_Color textColor) {
    // Kiểm tra text rỗng hoặc null để tránh lỗi "Text has zero width"
    if (!text || strlen(text) == 0) {
        return;
    }
    
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, textColor);
    if (!textSurface) {
        std::cout << "Unable to render text surface! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return;
    }
    
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        std::cout << "Unable to create texture from rendered text! SDL Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(textSurface);
        return;
    }
    
    SDL_Rect renderQuad = {x, y, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);
    
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

// Render HUD with game statistics
void renderHUD(SDL_Renderer* renderer, TTF_Font* font, int levelNum, int moves, int pushes) {
    int screenWidth = 1280; // Assuming this is the width of your screen
    SDL_Color textColor = {200, 200, 200, 255}; // Light gray text
    SDL_Color numberColor = {255, 255, 255, 255}; // White color for numbers
    
    // Left-aligned level name
    std::string levelText = "Sokoban " + std::to_string(levelNum);
    renderText(renderer, levelText.c_str(), 20, 20, font, textColor);
    
    // Right-aligned moves counter - position it away from pushes
    std::string movesStr = std::to_string(moves);
    std::string movesLabel = " moves";
    
    // Calculate positions to place them properly
    int pushesWidth = (std::to_string(pushes).length() + 7) * 12; // "X pushes" width
    int movesTextX = screenWidth - 360 - pushesWidth; // Moved 100 pixels left from original position
    
    // Render moves count with bold-like effect for numbers
    renderText(renderer, movesStr.c_str(), movesTextX, 20, font, numberColor);
    renderText(renderer, movesLabel.c_str(), movesTextX + movesStr.length() * 14, 20, font, textColor);
    
    // Right-aligned pushes counter - position it at the original place
    std::string pushesStr = std::to_string(pushes);
    std::string pushesLabel = " pushes";
    
    // Render pushes count with bold-like effect for numbers
    renderText(renderer, pushesStr.c_str(), screenWidth - 200, 20, font, numberColor);
    renderText(renderer, pushesLabel.c_str(), screenWidth - 200 + pushesStr.length() * 14, 20, font, textColor);
    
    // Draw dotted line underneath with better styling
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255); // Gray line color
    for (int x = 10; x < screenWidth - 10; x += 4) {
        SDL_RenderDrawPoint(renderer, x, 45);
    }
}

// Render solver status
void renderSolverStatus(SDL_Renderer* renderer, TTF_Font* font) {
    if (!solverActive && !showSolverStats) return; // Don't render if solver isn't active and stats aren't shown
    
    SDL_Color textColor = {255, 255, 255, 255}; // White
    SDL_Color activeColor = {0, 255, 0, 255}; // Green
    SDL_Color errorColor = {255, 0, 0, 255}; // Red
    SDL_Color infoColor = {135, 206, 250, 255}; // Light blue

    // Create smaller font (50% of original size)
    TTF_Font* smallFont = TTF_OpenFont("assets/fonts/arial.ttf", 12); // Assuming original font is 24px
    if (!smallFont) {
        std::cout << "Failed to load small font for solver stats" << std::endl;
        return;
    }
    
    // Calculate how many lines of text we'll have to size the background properly
    int lineCount = 1; // Start with 1 for "Solver" title
    if (solverRunning) lineCount++;
    else if (solverActive) {
        lineCount++; // For solution found or failed message
        if (solverFoundSolution && solverSolution.size() > 0) lineCount++; // For progress
    }
    
    if (showSolverStats) {
        if (solverNodesExplored > 0) lineCount++;
        if (solverMaxQueueSize > 0) lineCount++;
        if (solverExecutionTimeMs > 0) lineCount++;
        lineCount++; // For help text
    }
    
    // Add semi-transparent background panel - sized for smaller text
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180); // Semi-transparent black
    
    // Calculate panel size based on content and smaller font
    int panelWidth = 550; // Increased from 300 to 550 (added 250 pixels)
    int panelHeight = lineCount * 13 + 15; // Height based on line count + padding
    SDL_Rect solverInfoRect = {10, 50, panelWidth, panelHeight};
    SDL_RenderFillRect(renderer, &solverInfoRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    int yPos = 60;
    std::string solverText;

    renderText(renderer, "Solver", 20, yPos, smallFont, infoColor);
    yPos += 13; // Reduced spacing for smaller font

    if (solverRunning) {
        solverText = "Solver is running...";
        renderText(renderer, solverText.c_str(), 20, yPos, smallFont, activeColor);
        yPos += 13;
    } else if (solverActive) {
        if (solverFoundSolution) {
            solverText = "Solution found! " + std::to_string(solverSolution.size()) + " moves";
            renderText(renderer, solverText.c_str(), 20, yPos, smallFont, activeColor);
            yPos += 13;
            
            // Execute solution steps
            Uint32 currentTime = SDL_GetTicks();
            if (currentSolutionStep < solverSolution.size() && currentTime - lastSolutionStepTime >= SOLUTION_STEP_DELAY) {
                char move = solverSolution[currentSolutionStep];
                SDL_Event moveEvent;
                moveEvent.type = SDL_KEYDOWN;
                moveEvent.key.keysym.scancode = SDL_SCANCODE_UNKNOWN;
                moveEvent.key.keysym.mod = KMOD_NONE;
                moveEvent.key.repeat = 0;

                switch (move) {
                    case 'U': moveEvent.key.keysym.sym = SDLK_UP; break;
                    case 'D': moveEvent.key.keysym.sym = SDLK_DOWN; break;
                    case 'L': moveEvent.key.keysym.sym = SDLK_LEFT; break;
                    case 'R': moveEvent.key.keysym.sym = SDLK_RIGHT; break;
                }

                SDL_PushEvent(&moveEvent); // Fix: Pass the address of the moveEvent
                currentSolutionStep++;
                lastSolutionStepTime = currentTime;
            }
            
            // Display progress
            if (solverSolution.size() > 0) {
                std::string progressText = "Progress: " + std::to_string(currentSolutionStep) + 
                                        " / " + std::to_string(solverSolution.size());
                renderText(renderer, progressText.c_str(), 20, yPos, smallFont, textColor);
                yPos += 13;
            }
        } else {
            solverText = "Solver failed to find a solution.";
            renderText(renderer, solverText.c_str(), 20, yPos, smallFont, errorColor);
            yPos += 13;
        }
    }

    // Show advanced statistics if available and requested
    if (showSolverStats) {
        if (solverNodesExplored > 0) {
            std::string nodesText = "Nodes explored: " + std::to_string(solverNodesExplored);
            renderText(renderer, nodesText.c_str(), 20, yPos, smallFont, textColor);
            yPos += 13;
        }
        
        if (solverMaxQueueSize > 0) {
            std::string queueText = "Max queue size: " + std::to_string(solverMaxQueueSize);
            renderText(renderer, queueText.c_str(), 20, yPos, smallFont, textColor);
            yPos += 13;
        }
        
        if (solverExecutionTimeMs > 0) {
            std::string timeText = "Execution time: " + std::to_string(solverExecutionTimeMs) + " ms";
            renderText(renderer, timeText.c_str(), 20, yPos, smallFont, textColor);
            yPos += 13;
        }
    }
    
    // Display help text with smaller font
    if (showSolverStats) {
        std::string helpText = "F1: Solve  F3: Reset  I: Toggle Info";
        renderText(renderer, helpText.c_str(), 20, yPos, smallFont, infoColor);
    }
    
    // Clean up the small font
    TTF_CloseFont(smallFont);
}

// Render menu with highlighted selection
void renderMenu(SDL_Renderer* renderer, TTF_Font* font) {
    // Draw menu background
    if (menuBackgroundTexture) {
        // Just render the background texture without any color modification
        SDL_RenderCopy(renderer, menuBackgroundTexture, nullptr, nullptr);
    }

    // Menu items - updated order to match the enum order
    const char* menuItems[] = {
        "Start Game",
        "Select Level",
        "Select Skin",    // Swapped position with Settings to match enum order
        "Settings",       // Swapped position with Select Skin to match enum order
        "Quit"
    };
    
    const int itemCount = 5;
    const int startY = 230; // Moved down since we removed the title
    const int itemSpacing = 60; // Increased spacing between items
    
    // Render each menu item
    for (int i = 0; i < itemCount; i++) {
        SDL_Color textColor;
        std::string itemText;
        
        // Set text color based on selection (white for regular, dark green for selected)
        if (i == currentMenuSelection) {
            textColor = {0, 200, 0, 255}; // Dark green for selected item
            itemText = "> " + std::string(menuItems[i]) + " <";
        } else {
            textColor = {255, 255, 255, 255}; // White for unselected items
            itemText = menuItems[i];
        }
        
        // Calculate text width for proper centering
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, itemText.c_str(), textColor);
        int textWidth = textSurface ? textSurface->w : itemText.length() * 15;
        if (textSurface) {
            SDL_FreeSurface(textSurface);
        }
        
        // Calculate X position - moved 275 pixels to the left from center
        int x = ((1280 - textWidth) / 2) - 275;
        
        // Render the menu item
        renderText(renderer, itemText.c_str(), x, startY + i * itemSpacing, font, textColor);
    }
}

// Render level selection screen
void renderLevelSelect(SDL_Renderer* renderer, TTF_Font* font) {
    // Draw the level select background
    if (levelSelectBackgroundTexture) {
        SDL_RenderCopy(renderer, levelSelectBackgroundTexture, nullptr, nullptr);
    }

    // Draw semi-transparent background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 46, 96, 95, 170);
    SDL_Rect backgroundRect = {100, 80, 1080, 560};
    SDL_RenderFillRect(renderer, &backgroundRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    // Draw level options
    SDL_Color textColor = {255, 255, 255, 255}; // White
    SDL_Color currentLevelColor = {0, 255, 128, 255}; // Bright green for current level
    SDL_Color completedLevelColor = {135, 206, 250, 255}; // Light blue for completed levels
    
    const int startY = 180; // Moved up a bit to fit more rows
    const int itemSpacing = 70; // Slightly reduced spacing between rows
    const int itemsPerRow = 4; // Increased from 3 to 4 items per row
    const int itemWidth = 240; // Reduced width to fit 4 items
    const int levelsPerPage = 16; // 4x4 grid per page
    
    // Create a smaller font for instruction text (50% of original size)
    TTF_Font* smallFont = TTF_OpenFont("assets/fonts/arial.ttf", 12); // 50% of 24 = 12
    
    // Create a font for score display (doubled size from original small font)
    TTF_Font* scoreFont = TTF_OpenFont("assets/fonts/arial.ttf", 10); // Doubled from 5 to 10
    
    // Calculate current page based on selected level
    int currentPage = currentLevelIndex / levelsPerPage;
    int startLevel = currentPage * levelsPerPage;
    int endLevel = std::min(startLevel + levelsPerPage, totalLoadedLevels);
    
    // Draw level selection instructions with 50% smaller font, centered
    std::string instructionsText = "Use arrow keys to navigate and ENTER to select a level";
    SDL_Surface* instrSurface = TTF_RenderText_Solid(smallFont, instructionsText.c_str(), textColor);
    int instrWidth = instrSurface ? instrSurface->w : instructionsText.length() * 7;
    if (instrSurface) {
        SDL_FreeSurface(instrSurface);
    }
    int instrX = (1280 - instrWidth) / 2;
    renderText(renderer, instructionsText.c_str(), instrX, 140, smallFont, textColor);
    
    // Draw page indicator if there are multiple pages, centered
    if (totalLoadedLevels > levelsPerPage) {
        std::string pageText = "Page " + std::to_string(currentPage + 1) + 
                             " of " + std::to_string((totalLoadedLevels + levelsPerPage - 1) / levelsPerPage);
        SDL_Surface* pageSurface = TTF_RenderText_Solid(font, pageText.c_str(), textColor);
        int pageTextWidth = pageSurface ? pageSurface->w : pageText.length() * 14;
        if (pageSurface) {
            SDL_FreeSurface(pageSurface);
        }
        int pageX = (1280 - pageTextWidth) / 2;
        renderText(renderer, pageText.c_str(), pageX, 550, font, textColor);
    }
    
    // Draw level options for current page
    for (int i = startLevel; i < endLevel; i++) {
        int localIndex = i - startLevel;
        int row = localIndex / itemsPerRow;
        int col = localIndex % itemsPerRow;
        
        int x = 150 + col * itemWidth;
        int y = startY + row * itemSpacing;
        
        // Draw level number box - Changed from blue to dark green
        if (i == currentLevelIndex) {
            SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255); // Dark green for highlight color
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 70, 0, 255); // Darker green for regular levels
        }
        SDL_Rect levelBox = {x - 10, y - 5, 220, 65};
        SDL_RenderFillRect(renderer, &levelBox);
        
        // Level text with number
        std::string levelText = "Level " + std::to_string(i + 1);
        
        // Calculate text width for centering
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, levelText.c_str(), textColor);
        int textWidth = textSurface ? textSurface->w : levelText.length() * 15;
        if (textSurface) {
            SDL_FreeSurface(textSurface);
        }
        
        // Center the text in the box
        int centeredX = x + (200 - textWidth) / 2;
        
        // Highlight current level
        if (i == currentLevelIndex) {
            renderText(renderer, levelText.c_str(), centeredX, y, font, currentLevelColor);
        } else {
            renderText(renderer, levelText.c_str(), centeredX, y, font, textColor);
        }
        
        // If we have high scores, show them with doubled size font and centered
        std::string scoreText;
        if (i < static_cast<int>(game.highScores.size()) && game.highScores[i].moves < INT_MAX) {
            scoreText = std::to_string(game.highScores[i].moves) + " moves, " + 
                       std::to_string(game.highScores[i].pushes) + " pushes";
        } else {
            scoreText = "Not completed";
        }
        
        // Calculate score text width for centering
        SDL_Surface* scoreSurface = TTF_RenderText_Solid(scoreFont, scoreText.c_str(), textColor);
        int scoreWidth = scoreSurface ? scoreSurface->w : scoreText.length() * 5;
        if (scoreSurface) {
            SDL_FreeSurface(scoreSurface);
        }
        int centeredScoreX = x + (200 - scoreWidth) / 2;
        
        // Render the score text with its color
        SDL_Color scoreColor = (i < static_cast<int>(game.highScores.size()) && game.highScores[i].moves < INT_MAX) ? 
                             completedLevelColor : textColor;
        renderText(renderer, scoreText.c_str(), centeredScoreX, y + 35, scoreFont, scoreColor);
    }
    
    // Draw page navigation instructions if needed with smaller font and centered
    if (totalLoadedLevels > levelsPerPage) {
        SDL_Color pageNavColor = {100, 255, 255, 255}; // Cyan
        std::string pageNavText = "Press PageUp/PageDown to change pages";
        
        // Calculate text width for centering
        SDL_Surface* navSurface = TTF_RenderText_Solid(smallFont, pageNavText.c_str(), pageNavColor);
        int navWidth = navSurface ? navSurface->w : pageNavText.length() * 7;
        if (navSurface) {
            SDL_FreeSurface(navSurface);
        }
        int navX = (1280 - navWidth) / 2;
        
        renderText(renderer, pageNavText.c_str(), navX, 470, smallFont, pageNavColor);
    }
    
    // Draw navigation instructions with smaller font and centered
    SDL_Color navColor = {255, 160, 0, 255}; // Orange
    std::string backText = "Press ESC to return to menu";
    
    // Calculate text width for centering
    SDL_Surface* backSurface = TTF_RenderText_Solid(smallFont, backText.c_str(), navColor);
    int backWidth = backSurface ? backSurface->w : backText.length() * 7;
    if (backSurface) {
        SDL_FreeSurface(backSurface);
    }
    int backX = (1280 - backWidth) / 2;
    
    renderText(renderer, backText.c_str(), backX, 515, smallFont, navColor);
    
    // Clean up the fonts
    if (smallFont) {
        TTF_CloseFont(smallFont);
    }
    if (scoreFont) {
        TTF_CloseFont(scoreFont);
    }
}

// Render level complete screen
void renderLevelComplete(SDL_Renderer* renderer, TTF_Font* normalFont, TTF_Font* largeFont, int levelNum, int moves, int pushes) {
    // Draw semi-transparent overlay to create a dimming effect
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128); // Semi-transparent black
    SDL_Rect overlayRect = {0, 0, 1280, 720};
    SDL_RenderFillRect(renderer, &overlayRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Centering helper variables
    int screenWidth = 1280;
    int screenHeight = 720;

    // Draw large "EXCELLENT!" message in the center with bright gold color
    SDL_Color goldColor = {255, 215, 0, 255}; // Gold
    const char* excellentText = "EXCELLENT!";
    SDL_Surface* excellentSurface = TTF_RenderText_Solid(largeFont, excellentText, goldColor);
    SDL_Texture* excellentTexture = SDL_CreateTextureFromSurface(renderer, excellentSurface);
    SDL_Rect excellentRect = {
        (screenWidth - excellentSurface->w) / 2, // Center horizontally
        screenHeight / 4 - excellentSurface->h / 2, // Position near the top
        excellentSurface->w,
        excellentSurface->h
    };
    SDL_RenderCopy(renderer, excellentTexture, nullptr, &excellentRect);
    SDL_FreeSurface(excellentSurface);
    SDL_DestroyTexture(excellentTexture);

    // Draw level completion message
    SDL_Color whiteColor = {255, 255, 255, 255}; // White
    std::string levelCompleteText = "Level " + std::to_string(levelNum) + " Complete!";
    SDL_Surface* levelCompleteSurface = TTF_RenderText_Solid(normalFont, levelCompleteText.c_str(), whiteColor);
    SDL_Texture* levelCompleteTexture = SDL_CreateTextureFromSurface(renderer, levelCompleteSurface);
    SDL_Rect levelCompleteRect = {
        (screenWidth - levelCompleteSurface->w) / 2, // Center horizontally
        screenHeight / 2 - levelCompleteSurface->h / 2, // Center vertically
        levelCompleteSurface->w,
        levelCompleteSurface->h
    };
    SDL_RenderCopy(renderer, levelCompleteTexture, nullptr, &levelCompleteRect);
    SDL_FreeSurface(levelCompleteSurface);
    SDL_DestroyTexture(levelCompleteTexture);

    // Draw statistics
    std::string statsText = "Moves: " + std::to_string(moves) + "  Pushes: " + std::to_string(pushes);
    SDL_Surface* statsSurface = TTF_RenderText_Solid(normalFont, statsText.c_str(), whiteColor);
    SDL_Texture* statsTexture = SDL_CreateTextureFromSurface(renderer, statsSurface);
    SDL_Rect statsRect = {
        (screenWidth - statsSurface->w) / 2, // Center horizontally
        screenHeight / 2 + 50, // Slightly below the level complete message
        statsSurface->w,
        statsSurface->h
    };
    SDL_RenderCopy(renderer, statsTexture, nullptr, &statsRect);
    SDL_FreeSurface(statsSurface);
    SDL_DestroyTexture(statsTexture);

    // Display high score message if achieved
    if (game.isNewRecord) {
        SDL_Color brightGreenColor = {0, 255, 128, 255}; // Bright green
        const char* newRecordText = "NEW HIGH SCORE!";
        SDL_Surface* newRecordSurface = TTF_RenderText_Solid(normalFont, newRecordText, brightGreenColor);
        SDL_Texture* newRecordTexture = SDL_CreateTextureFromSurface(renderer, newRecordSurface);
        SDL_Rect newRecordRect = {
            (screenWidth - newRecordSurface->w) / 2, // Center horizontally
            screenHeight / 2 + 100, // Below the stats
            newRecordSurface->w,
            newRecordSurface->h
        };
        SDL_RenderCopy(renderer, newRecordTexture, nullptr, &newRecordRect);
        SDL_FreeSurface(newRecordSurface);
        SDL_DestroyTexture(newRecordTexture);
    }

    // Draw instruction
    SDL_Color yellowColor = {255, 255, 0, 255}; // Yellow
    const char* continueText = "Press SPACE to Continue";
    SDL_Surface* continueSurface = TTF_RenderText_Solid(normalFont, continueText, yellowColor);
    SDL_Texture* continueTexture = SDL_CreateTextureFromSurface(renderer, continueSurface);
    SDL_Rect continueRect = {
        (screenWidth - continueSurface->w) / 2, // Center horizontally
        screenHeight - 100, // Near the bottom
        continueSurface->w,
        continueSurface->h
    };
    SDL_RenderCopy(renderer, continueTexture, nullptr, &continueRect);
    SDL_FreeSurface(continueSurface);
    SDL_DestroyTexture(continueTexture);

    // Play level complete sound effect
    if (game.settings.sfxEnabled && soundEffects[2]) {
        Mix_PlayChannel(-1, soundEffects[2], 0);
    }
}

// Render game completion screen
void renderGameComplete(SDL_Renderer* renderer, TTF_Font* normalFont, TTF_Font* largeFont, int moves, int pushes) {
    // Use the done background image
    SDL_Surface* doneBackgroundSurface = IMG_Load("assets/images/menu/done_background.png");
    if (doneBackgroundSurface) {
        SDL_Texture* doneBackgroundTexture = SDL_CreateTextureFromSurface(renderer, doneBackgroundSurface);
        SDL_FreeSurface(doneBackgroundSurface);
        
        if (doneBackgroundTexture) {
            // Draw the background
            SDL_RenderCopy(renderer, doneBackgroundTexture, nullptr, nullptr);
            SDL_DestroyTexture(doneBackgroundTexture);
        } else {
            // Fallback if texture creation failed
            SDL_SetRenderDrawColor(renderer, 0, 0, 64, 255);
            SDL_RenderClear(renderer);
        }
    } else {
        // Fallback if image loading failed
        SDL_SetRenderDrawColor(renderer, 0, 0, 64, 255);
        SDL_RenderClear(renderer);
    }
    
    const int screenWidth = 1280; // Screen width
      // Draw congratulations message with bright green color (centered)
    SDL_Color brightGreenColor = {0, 255, 128, 255}; // Bright green
    
    // Calculate text width for centering
    SDL_Surface* congratsSurface = TTF_RenderText_Solid(largeFont, "CONGRATULATIONS!", brightGreenColor);
    if (congratsSurface) {
        int congratsWidth = congratsSurface->w;
        int congratsX = (screenWidth - congratsWidth) / 2;
        SDL_FreeSurface(congratsSurface);
        
        renderText(renderer, "CONGRATULATIONS!", congratsX, 120, largeFont, brightGreenColor);
    }
    
    // Removed star decorations as requested
    
    // Create a 40% smaller font for instructions
    TTF_Font* smallerFont = TTF_OpenFont("assets/fonts/arial.ttf", 14); // 60% of normal font (assuming normal is ~24)
    if (!smallerFont) {
        smallerFont = normalFont; // Fallback to normal font if smaller font creation fails
    }
      // Draw instructions with smaller font and moved up 150 pixels from previous position (centered)
    SDL_Color lightBlueColor = {135, 206, 250, 255}; // Light blue
      // "Press ESC" text
    SDL_Surface* escSurface = TTF_RenderText_Solid(smallerFont, "Press ESC to return to Menu", lightBlueColor);
    if (escSurface) {
        int escWidth = escSurface->w;
        int escX = (screenWidth - escWidth) / 2;
        SDL_FreeSurface(escSurface);
        
        renderText(renderer, "Press ESC to return to Menu", escX, 190, smallerFont, lightBlueColor);
    }
    
    // "Press Q" text
    SDL_Surface* quitSurface = TTF_RenderText_Solid(smallerFont, "Press Q to Quit", lightBlueColor);
    if (quitSurface) {
        int quitWidth = quitSurface->w;
        int quitX = (screenWidth - quitWidth) / 2;
        SDL_FreeSurface(quitSurface);
        
        renderText(renderer, "Press Q to Quit", quitX, 220, smallerFont, lightBlueColor);
    }
    
    // Close the smaller font if we created a new one
    if (smallerFont != normalFont) {
        TTF_CloseFont(smallerFont);
    }
}

// Render settings menu
void renderSettings(SDL_Renderer* renderer, TTF_Font* font) {
    // Draw the settings background (reuse level select background for now)
    if (levelSelectBackgroundTexture) {
        SDL_RenderCopy(renderer, levelSelectBackgroundTexture, nullptr, nullptr);
    }

    // Draw semi-transparent background panel
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 46, 96, 95, 190);
    SDL_Rect backgroundRect = {320, 120, 640, 480};
    SDL_RenderFillRect(renderer, &backgroundRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    // Draw settings title
    SDL_Color titleColor = {255, 255, 100, 255}; // Light yellow
    renderText(renderer, "Game Settings", 520, 140, font, titleColor);
    
    // Draw horizontal divider
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawLine(renderer, 350, 180, 930, 180);    // Settings menu items
    const char* settingsItems[] = {
        "Background Music",
        "Sound Effects",
        "Tutorials",
        "Back to Main Menu"
    };    // Current values for settings
    std::string bgmValue = game.settings.bgmEnabled ? "ON" : "OFF";
    std::string sfxValue = game.settings.sfxEnabled ? "ON" : "OFF";
    
    const std::string settingsValues[] = {
        bgmValue,
        sfxValue,
        "",  // Tutorials option (remove "Press Enter")
        ""  // Back option has no value
    };
    
    const int startY = 220;
    const int itemSpacing = 60;
    
    // Render each settings item with its current value
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        SDL_Color textColor;
        SDL_Color valueColor = {255, 255, 100, 255}; // Light yellow for values
        std::string itemText;
        
        // Set text color based on selection
        if (i == currentSettingsSelection) {
            textColor = {0, 255, 0, 255}; // Green for selected item
            itemText = "> " + std::string(settingsItems[i]);
        } else {
            textColor = {255, 255, 255, 255}; // White for unselected items
            itemText = settingsItems[i];
        }        // Render the setting name
        renderText(renderer, itemText.c_str(), 370, startY + i * itemSpacing, font, textColor);
          // Render the setting value (except for Back option and empty strings)
        if (i < SETTINGS_BACK && !settingsValues[i].empty()) {
            renderText(renderer, settingsValues[i].c_str(), 825, startY + i * itemSpacing, font, valueColor);
        }
    }
    
    // Create smaller font for instructions (20% smaller than original)
    TTF_Font* smallFont = TTF_OpenFont("assets/fonts/arial.ttf", 15); // 24 * 0.8 = 19
    
    // Draw navigation instructions - centered
    SDL_Color instructionColor = {150, 220, 255, 255}; // Light blue
    
    // Split the navigation instructions into two lines
    std::string navInstructionLine1 = "Use UP/DOWN to navigate";
    std::string navInstructionLine2 = "LEFT/RIGHT to change settings";
    
    // Calculate text width for centering the first line
    SDL_Surface* instrSurface1 = TTF_RenderText_Solid(smallFont, navInstructionLine1.c_str(), instructionColor);
    int instrWidth1 = instrSurface1 ? instrSurface1->w : navInstructionLine1.length() * 10;
    if (instrSurface1) {
        SDL_FreeSurface(instrSurface1);
    }
    
    // Center the first line horizontally
    int instrX1 = 320 + (640 - instrWidth1) / 2;
    
    // Calculate text width for centering the second line
    SDL_Surface* instrSurface2 = TTF_RenderText_Solid(smallFont, navInstructionLine2.c_str(), instructionColor);
    int instrWidth2 = instrSurface2 ? instrSurface2->w : navInstructionLine2.length() * 10;
    if (instrSurface2) {
        SDL_FreeSurface(instrSurface2);
    }
    
    // Center the second line horizontally
    int instrX2 = 320 + (640 - instrWidth2) / 2;
    
    // Draw both instruction lines with appropriate spacing
    renderText(renderer, navInstructionLine1.c_str(), instrX1, 500, smallFont, instructionColor);
    renderText(renderer, navInstructionLine2.c_str(), instrX2, 530, smallFont, instructionColor);
    
    // Clean up the font
    if (smallFont) {
        TTF_CloseFont(smallFont);
    }
}

// Render player skin selection screen
void renderSkinSelect(SDL_Renderer* renderer, TTF_Font* font) {
    // Draw the background (reuse level select background for now)
    if (levelSelectBackgroundTexture) {
        SDL_RenderCopy(renderer, levelSelectBackgroundTexture, nullptr, nullptr);
    }
    
    // Draw semi-transparent background panel
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 46, 96, 95, 190);
    SDL_Rect backgroundRect = {320, 120, 640, 480};
    SDL_RenderFillRect(renderer, &backgroundRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    // Draw title
    SDL_Color titleColor = {255, 255, 100, 255}; // Light yellow
    
    // Calculate width of title for proper centering
    std::string titleText = "Select Player Skin";
    SDL_Surface* titleSurface = TTF_RenderText_Solid(font, titleText.c_str(), titleColor);
    int titleWidth = titleSurface ? titleSurface->w : titleText.length() * 15;
    if (titleSurface) {
        SDL_FreeSurface(titleSurface);
    }
    
    // Center the title text
    int titleX = 320 + (640 - titleWidth) / 2;
    renderText(renderer, titleText.c_str(), titleX, 140, font, titleColor);
    
    // Draw horizontal divider
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawLine(renderer, 350, 180, 930, 180);    // Skin options
    const char* skinNames[] = {
        "Con meo da den",
        "Bombardino coccodrillo",
        "Capybara",
        "Tralalelo Tralala",
        "Tung tung tung sahur",
        "Kera Candy",
        "Win Sweet",
        "When Event"
    };
    
    // Only show current skin selection in center
    int currentSkin = currentSkinSelection;
    
    // Ensure currentSkin is within the valid range of actual skins (not including "Back" option)
    if (currentSkin >= SKIN_COUNT) {
        currentSkin = 0;
    }
    
    // Define colors
    SDL_Color normalTextColor = {255, 255, 255, 255}; // White for regular text
    SDL_Color arrowColor = {0, 255, 0, 255}; // Green for arrows
    
    // Calculate center position
    int centerX = 320 + 640 / 2; // Center of panel
    int centerY = 280; // Vertical position for skin display
    
    // Draw left arrow
    std::string leftArrow = "<";
    renderText(renderer, leftArrow.c_str(), centerX - 150, centerY, font, arrowColor);
    
    // Draw right arrow
    std::string rightArrow = ">";
    renderText(renderer, rightArrow.c_str(), centerX + 120, centerY, font, arrowColor);
    
    // Display the current skin image in the center (larger size)
    SDL_Surface* playerSkinSurface = IMG_Load(playerSkinNames[currentSkin][0]);
    if (playerSkinSurface) {
        SDL_Texture* playerSkinTexture = SDL_CreateTextureFromSurface(renderer, playerSkinSurface);
        SDL_FreeSurface(playerSkinSurface);
        
        if (playerSkinTexture) {
            // Display an enlarged version of the skin
            SDL_Rect destRect = {centerX - 40, centerY - 40, 80, 80};
            SDL_RenderCopy(renderer, playerSkinTexture, nullptr, &destRect);
            SDL_DestroyTexture(playerSkinTexture);
        }
    }
    
    // Display skin name below the image
    std::string skinName = skinNames[currentSkin];
    
    // Calculate text width for centering
    SDL_Surface* nameSurface = TTF_RenderText_Solid(font, skinName.c_str(), normalTextColor);
    int nameWidth = nameSurface ? nameSurface->w : skinName.length() * 15;
    if (nameSurface) {
        SDL_FreeSurface(nameSurface);
    }
    
    // Center the name under the skin image
    int nameX = centerX - nameWidth / 2;
    renderText(renderer, skinName.c_str(), nameX, centerY + 100, font, normalTextColor);
    
    // Add "Back to Main Menu" option
    SDL_Color backColor = currentSkinSelection == SKIN_COUNT ? 
                         SDL_Color{0, 255, 0, 255} : SDL_Color{255, 255, 255, 255};
    std::string backText = currentSkinSelection == SKIN_COUNT ? 
                         "> Back to Main Menu <" : "Back to Main Menu";
                         
    // Calculate text width for centering
    SDL_Surface* backSurface = TTF_RenderText_Solid(font, backText.c_str(), backColor);
    int backWidth = backSurface ? backSurface->w : backText.length() * 15;
    if (backSurface) {
        SDL_FreeSurface(backSurface);
    }
    
    // Center the back text at the bottom of the panel
    int backX = 320 + (640 - backWidth) / 2;
    renderText(renderer, backText.c_str(), backX, 450, font, backColor);
    
    // Create smaller font for instructions (30% smaller than original)
    TTF_Font* smallFont = TTF_OpenFont("assets/fonts/arial.ttf", 15); // 24 * 0.7 = 16.8 ≈ 17
    
    // Draw navigation instructions - centered
    SDL_Color instructionColor = {150, 220, 255, 255}; // Light blue
    
    // Split the navigation instructions into two lines with LEFT/RIGHT instead of UP/DOWN
    std::string navInstructionLine1 = "Use LEFT/RIGHT to navigate";
    std::string navInstructionLine2 = "UP/DOWN for Back option, ENTER to select";
    
    // Calculate text width for centering the first line
    SDL_Surface* instrSurface1 = TTF_RenderText_Solid(smallFont, navInstructionLine1.c_str(), instructionColor);
    int instrWidth1 = instrSurface1 ? instrSurface1->w : navInstructionLine1.length() * 10;
    if (instrSurface1) {
        SDL_FreeSurface(instrSurface1);
    }
    
    // Center the first line horizontally
    int instrX1 = 320 + (640 - instrWidth1) / 2;
    
    // Calculate text width for centering the second line
    SDL_Surface* instrSurface2 = TTF_RenderText_Solid(smallFont, navInstructionLine2.c_str(), instructionColor);
    int instrWidth2 = instrSurface2 ? instrSurface2->w : navInstructionLine2.length() * 10;
    if (instrSurface2) {
        SDL_FreeSurface(instrSurface2);
    }
    
    // Center the second line horizontally
    int instrX2 = 320 + (640 - instrWidth2) / 2;
    
    // Draw both instruction lines with appropriate spacing
    renderText(renderer, navInstructionLine1.c_str(), instrX1, 500, smallFont, instructionColor);
    renderText(renderer, navInstructionLine2.c_str(), instrX2, 530, smallFont, instructionColor);
    
    // Clean up the font
    if (smallFont) {
        TTF_CloseFont(smallFont);
    }
}

// Initialize menu background
bool initMenuBackground(SDL_Renderer* renderer) {
    // Load menu background
    SDL_Surface* menuBackgroundSurface = IMG_Load("assets/images/menu/menu_background.png");
    if (menuBackgroundSurface) {
        menuBackgroundTexture = SDL_CreateTextureFromSurface(renderer, menuBackgroundSurface);
        SDL_FreeSurface(menuBackgroundSurface);
        if (!menuBackgroundTexture) {
            std::cerr << "Failed to create menu background texture" << std::endl;
            return false;
        }
    }

    // Load level select background
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

// Initialize tutorial image
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

// Render tutorial image overlay
void renderTutorial(SDL_Renderer* renderer) {
    if (!tutorialTexture) {
        return;
    }
    
    // Draw a semi-transparent background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200); // Semi-transparent black
    SDL_Rect fullScreenRect = {0, 0, 1280, 720};
    SDL_RenderFillRect(renderer, &fullScreenRect);
    
    // Get the dimensions of the tutorial image
    int imgWidth, imgHeight;
    SDL_QueryTexture(tutorialTexture, NULL, NULL, &imgWidth, &imgHeight);
    
    // Calculate position to center the image
    int x = (1280 - imgWidth) / 2;
    int y = (720 - imgHeight) / 2;
    
    // Create a rectangle for rendering the texture
    SDL_Rect destRect = {x, y, imgWidth, imgHeight};
    
    // Render the tutorial image
    SDL_RenderCopy(renderer, tutorialTexture, NULL, &destRect);
    
    // Add "Press ESC to return" text at the bottom
    SDL_Color textColor = {255, 255, 255, 255}; // White
    const char* escText = "Press ESC to close tutorial";
    
    // Create a temporary font for the text
    TTF_Font* font = TTF_OpenFont("assets/fonts/arial.ttf", 18);
    if (font) {
        // Calculate text position at the bottom center
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, escText, textColor);
        if (textSurface) {
            int textX = (1280 - textSurface->w) / 2;
            int textY = y + imgHeight + 20;
            SDL_FreeSurface(textSurface);
            
            renderText(renderer, escText, textX, textY, font, textColor);
        }
        
        TTF_CloseFont(font);
    }
}

// Clean up menu resources
void cleanupMenuResources() {
    // Xóa nhiều texture cùng một lúc với vòng lặp
    for (auto& texture : {&menuBackgroundTexture, &levelSelectBackgroundTexture, 
                          &gameLevelBackgroundTexture, &tutorialTexture}) {
        if (*texture) {
            SDL_DestroyTexture(*texture);
            *texture = nullptr;
        }
    }
}

// Modified function to scan levels directory
void scanLevelsDirectory(const std::string& path) {
    dynamicLevelFiles.clear();
    totalLoadedLevels = 0;
    
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        std::cerr << "Error opening: " << path << std::endl;
        return;
    }
    
    // Sử dụng vòng lặp for ngắn gọn hơn
    for (struct dirent* entry; (entry = readdir(dir));) {
        std::string file = entry->d_name;
        if (file.size() > 4 && file.substr(file.size() - 4) == ".txt") {
            dynamicLevelFiles.push_back(path + "/" + file);
            totalLoadedLevels++;
        }
    }
    closedir(dir);
    
    // Sắp xếp các file theo số thứ tự
    std::sort(dynamicLevelFiles.begin(), dynamicLevelFiles.end(), 
        [](const auto& a, const auto& b) {
            std::string fileA = a.substr(a.find_last_of('/') + 1);
            std::string fileB = b.substr(b.find_last_of('/') + 1);
            return std::stoi(fileA.substr(5, fileA.length() - 9)) < 
                   std::stoi(fileB.substr(5, fileB.length() - 9));
        });
    
    std::cout << "Loaded " << totalLoadedLevels << " levels from " << path << std::endl;
}
