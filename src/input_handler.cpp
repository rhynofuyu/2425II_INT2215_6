#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <vector>
#include <string>

#include "include/input_handler.h"
#include "include/game_structures.h"
#include "include/solver.h"
#include "include/game_resources.h"

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
