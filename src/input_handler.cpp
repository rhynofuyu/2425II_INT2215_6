#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include "include/input_handler.h"
#include "include/game_structures.h"
#include "include/game_init.h"
#include "include/game_logic.h"

// Biến toàn cục menu
int currentMenuSelection = MENU_START_GAME;
int currentSettingsSelection = SETTINGS_BACKGROUND_MUSIC;
int currentSkinSelection = SKIN_DEFAULT;

// Handle player input
void handleInput(SDL_Event& event) {
    if (event.type != SDL_KEYDOWN) {
        return;
    }
    
    // Process input based on the current game state
    switch (game.currentState) {
        case MENU:
            handleMenuInput(event);
            break;
        case PLAYING:
            handleGameplayInput(event);
            break;
        case LEVEL_SELECT:
            handleLevelSelectInput(event);
            break;
        case SETTINGS:
            handleSettingsInput(event);
            break;
        case SKIN_SELECT:
            handleSkinSelectInput(event);
            break;
        case LEVEL_COMPLETE:
            handleLevelCompleteInput(event);
            break;
        case GAME_OVER:
            handleGameCompleteInput(event);
            break;
        default:
            break;
    }
}

// Handle menu state
void handleMenuInput(SDL_Event& event) {
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
                if (!loadLevelFromFile(levelFileNames[currentLevelIndex].c_str(), &game.activeLevel)) {
                    std::cerr << "Error: Failed to load level from " << levelFileNames[currentLevelIndex] << std::endl;
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
                SDL_PushEvent(&quitEvent);
            }
            break;
        default:
            break;
    }
}

// Handle level select state
void handleLevelSelectInput(SDL_Event& event) {
    switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
            // Return to main menu
            game.currentState = MENU;
            break;
        
        case SDLK_UP:
            // Navigate up (to previous row)
            if (currentLevelIndex >= 4) {
                currentLevelIndex -= 4;
            }
            break;
            
        case SDLK_DOWN:
            // Navigate down (to next row)
            if (currentLevelIndex + 4 < totalLoadedLevels) {
                currentLevelIndex += 4;
            }
            break;
            
        case SDLK_LEFT:
            // Navigate to previous level
            if (currentLevelIndex > 0) {
                currentLevelIndex--;
            }
            break;
            
        case SDLK_RIGHT:
            // Navigate to next level
            if (currentLevelIndex + 1 < totalLoadedLevels) {
                currentLevelIndex++;
            }
            break;
            
        case SDLK_RETURN:
        case SDLK_SPACE:
            // Load the selected level
            if (!loadLevelFromFile(levelFileNames[currentLevelIndex].c_str(), &game.activeLevel)) {
                std::cerr << "Error: Failed to load level from " << levelFileNames[currentLevelIndex] << std::endl;
                return;
            }
            
            // Clear move history
            game.moveHistory.clear();
            game.isNewRecord = false;
            
            // Initialize level
            initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
            
            // Start playing
            game.currentState = PLAYING;
            break;
            
        default:
            break;
    }
}

// Handle settings menu state
void handleSettingsInput(SDL_Event& event) {
    switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
            // Return to main menu and save settings
            saveSettings("game_settings.dat");
            game.currentState = MENU;
            break;
            
        case SDLK_UP:
            // Navigate up in settings menu
            currentSettingsSelection = (currentSettingsSelection - 1 + SETTINGS_COUNT) % SETTINGS_COUNT;
            break;
            
        case SDLK_DOWN:
            // Navigate down in settings menu
            currentSettingsSelection = (currentSettingsSelection + 1) % SETTINGS_COUNT;
            break;
            
        case SDLK_RETURN:
        case SDLK_SPACE:
            // Toggle setting or return to menu
            if (currentSettingsSelection == SETTINGS_BACKGROUND_MUSIC) {
                // Toggle background music
                game.settings.bgmEnabled = !game.settings.bgmEnabled;
                
                // Apply the change immediately
                if (game.settings.bgmEnabled && backgroundMusic) {
                    Mix_PlayMusic(backgroundMusic, -1);
                } else {
                    Mix_HaltMusic();
                }
            }
            else if (currentSettingsSelection == SETTINGS_SOUND_EFFECTS) {
                // Toggle sound effects
                game.settings.sfxEnabled = !game.settings.sfxEnabled;
            }
            else if (currentSettingsSelection == SETTINGS_BACK) {
                // Save settings and return to main menu
                saveSettings("game_settings.dat");
                game.currentState = MENU;
            }
            break;
            
        default:
            break;
    }
}

// Handle skin selection menu
void handleSkinSelectInput(SDL_Event& event) {
    switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
            // Return to main menu and save settings
            saveSettings("game_settings.dat");
            game.currentState = MENU;
            break;
            
        case SDLK_LEFT:
            // Select previous skin
            currentSkinSelection = (currentSkinSelection - 1 + SKIN_COUNT) % SKIN_COUNT;
            game.settings.currentSkin = static_cast<PlayerSkin>(currentSkinSelection);
            break;
            
        case SDLK_RIGHT:
            // Select next skin
            currentSkinSelection = (currentSkinSelection + 1) % SKIN_COUNT;
            game.settings.currentSkin = static_cast<PlayerSkin>(currentSkinSelection);
            break;
            
        case SDLK_RETURN:
        case SDLK_SPACE:
            // Confirm selection and return to main menu
            game.settings.currentSkin = static_cast<PlayerSkin>(currentSkinSelection);
            saveSettings("game_settings.dat");
            game.currentState = MENU;
            break;
            
        default:
            break;
    }
}

// Handle level complete input
void handleLevelCompleteInput(SDL_Event& event) {
    switch (event.key.keysym.sym) {
        case SDLK_RETURN:
        case SDLK_SPACE:
            // If this is the last level, go to game over screen
            if (currentLevelIndex + 1 >= totalLoadedLevels) {
                game.currentState = GAME_OVER;
            } else {
                // Otherwise, load the next level
                currentLevelIndex++;
                if (!loadLevelFromFile(levelFileNames[currentLevelIndex].c_str(), &game.activeLevel)) {
                    std::cerr << "Error: Failed to load level from " << levelFileNames[currentLevelIndex] << std::endl;
                    return;
                }
                
                // Clear move history
                game.moveHistory.clear();
                game.isNewRecord = false;
                
                // Initialize level
                initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
                
                // Start playing the next level
                game.currentState = PLAYING;
            }
            break;
            
        case SDLK_ESCAPE:
            // Return to main menu
            game.currentState = MENU;
            break;
            
        default:
            break;
    }
}

// Handle game complete input
void handleGameCompleteInput(SDL_Event& event) {
    switch (event.key.keysym.sym) {
        case SDLK_RETURN:
        case SDLK_SPACE:
        case SDLK_ESCAPE:
            // Return to main menu
            game.currentState = MENU;
            break;
            
        default:
            break;
    }
}

// Handle gameplay input
void handleGameplayInput(SDL_Event& event) {
    bool moved = false;
    
    switch (event.key.keysym.sym) {
        case SDLK_UP:
            moved = movePlayer(0, -1, &game.activeLevel, &game.player);
            break;
            
        case SDLK_DOWN:
            moved = movePlayer(0, 1, &game.activeLevel, &game.player);
            break;
            
        case SDLK_LEFT:
            moved = movePlayer(-1, 0, &game.activeLevel, &game.player);
            break;
            
        case SDLK_RIGHT:
            moved = movePlayer(1, 0, &game.activeLevel, &game.player);
            break;
            
        case SDLK_z:
            // Undo the last move if there's a move in the history
            if (undoMove()) {
                // Play sound effect - use "move" sound for undo
                if (game.settings.sfxEnabled && soundEffects[0]) {
                    Mix_PlayChannel(-1, soundEffects[0], 0);
                }
            }
            break;
            
        case SDLK_r:
            // Reset the level (reload from file)
            if (!loadLevelFromFile(levelFileNames[currentLevelIndex].c_str(), &game.activeLevel)) {
                std::cerr << "Error: Failed to reload level from " << levelFileNames[currentLevelIndex] << std::endl;
                return;
            }
            
            // Clear move history
            game.moveHistory.clear();
            
            // Re-initialize level
            initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
            break;
            
        case SDLK_ESCAPE:
            // Return to main menu
            game.currentState = MENU;
            break;
            
        default:
            break;
    }
    
    // If player moved, play appropriate sound effect
    if (moved && game.settings.sfxEnabled) {
        // Determine if this was a push or just a move
        MoveRecord& lastMove = game.moveHistory.back();
        if (lastMove.wasBoxMoved && soundEffects[1]) {
            // Play push sound
            Mix_PlayChannel(-1, soundEffects[1], 0);
        } else if (soundEffects[0]) {
            // Play move sound
            Mix_PlayChannel(-1, soundEffects[0], 0);
        }
    }
}

// Function to handle player movement and box pushing
bool movePlayer(int dx, int dy, Level* level, PlayerInfo* player) {
    // Calculate the new position
    int newX = player->x + dx;
    int newY = player->y + dy;
    
    // Check if the new position is valid (within the map boundaries)
    if (newX < 0 || newX >= level->width || newY < 0 || newY >= level->height) {
        return false;
    }
    
    // Check what tile is at the new position
    TileType targetTile = level->currentMap[newY][newX];
    
    // Record the move before we make any changes
    MoveRecord move;
    move.playerPos = Point(player->x, player->y);
    move.wasBoxMoved = false;
    move.playerPrevTile = (level->currentMap[player->y][player->x] == PLAYER_ON_TARGET) ? TARGET : EMPTY;
    
    // Check if we are trying to move into a wall
    if (targetTile == WALL) {
        return false;
    }
    
    // Check if we are trying to move into a box
    if (targetTile == BOX || targetTile == BOX_ON_TARGET) {
        // Calculate where the box would move to
        int boxNewX = newX + dx;
        int boxNewY = newY + dy;
        
        // Check if the box can be pushed (valid position and not occupied by wall/box)
        if (boxNewX < 0 || boxNewX >= level->width || boxNewY < 0 || boxNewY >= level->height ||
            level->currentMap[boxNewY][boxNewX] == WALL || 
            level->currentMap[boxNewY][boxNewX] == BOX || 
            level->currentMap[boxNewY][boxNewX] == BOX_ON_TARGET) {
            return false;
        }
        
        // Record the box's move
        move.movedBoxPos = Point(boxNewX, boxNewY);
        move.boxPrevPos = Point(newX, newY);
        move.wasBoxMoved = true;
        
        // Update the box's position
        if (level->currentMap[boxNewY][boxNewX] == TARGET) {
            level->currentMap[boxNewY][boxNewX] = BOX_ON_TARGET;
        } else {
            level->currentMap[boxNewY][boxNewX] = BOX;
        }
        
        // Update the empty space left by the box
        if (targetTile == BOX_ON_TARGET) {
            level->currentMap[newY][newX] = TARGET;
        } else {
            level->currentMap[newY][newX] = EMPTY;
        }
        
        // Increment the pushes counter
        player->pushes++;
    }
    
    // Update the player's position
    if (level->currentMap[player->y][player->x] == PLAYER_ON_TARGET) {
        level->currentMap[player->y][player->x] = TARGET;
    } else {
        level->currentMap[player->y][player->x] = EMPTY;
    }
    
    // Place the player at the new position
    if (level->currentMap[newY][newX] == TARGET) {
        level->currentMap[newY][newX] = PLAYER_ON_TARGET;
    } else {
        level->currentMap[newY][newX] = PLAYER;
    }
    
    // Update player coordinates
    player->x = newX;
    player->y = newY;
    
    // Increment the moves counter
    player->moves++;
    
    // Record the move in the history
    recordMove(move);
    
    return true;
}
