#include <iostream>
#include <fstream>
#include "include/game_logic.h"
#include "include/game_structures.h"

// Use extern to reference global variables defined in game_structures.cpp
extern const std::string levelFileNames[];
extern int currentLevelIndex;
extern int totalLoadedLevels;

// Check win condition for the current level
bool checkWinCondition(Level* level) {
    // The win condition is that all boxes are on targets
    // This means there should be no BOX tiles left, only BOX_ON_TARGET
    
    for (int y = 0; y < level->height; y++) {
        for (int x = 0; x < level->width; x++) {
            if (level->currentMap[y][x] == BOX) {
                // If there's at least one box not on a target, we haven't won yet
                return false;
            }
        }
    }
    
    // If we've reached here, all boxes are on targets
    return true;
}

// Reset the level to its original state
void resetLevel(Level* level, PlayerInfo* player) {
    // Copy the original map back to the current map
    for (int y = 0; y < level->height; y++) {
        for (int x = 0; x < level->width; x++) {
            level->currentMap[y][x] = level->originalMap[y][x];
        }
    }
    
    // Reset player position and stats
    player->x = level->playerStartX;
    player->y = level->playerStartY;
    player->moves = 0;
    player->pushes = 0;
}

// Handle level completion
void handleLevelCompletion(int levelIndex, int moves, int pushes) {
    // Check if this is a new high score
    game.isNewRecord = isNewHighScore(levelIndex, moves, pushes);
    
    // Save high scores to file if it's a new record
    if (game.isNewRecord) {
        saveHighScores("highscores.dat");
    }
}

// Load the next level
bool loadNextLevel() {
    // Check if there is a next level
    if (currentLevelIndex + 1 >= totalLoadedLevels) {
        return false;
    }
    
    // Increment the level index
    currentLevelIndex++;
    
    // Load the next level
    if (!loadLevelFromFile(levelFileNames[currentLevelIndex].c_str(), &game.activeLevel)) {
        std::cerr << "Error: Failed to load level from " << levelFileNames[currentLevelIndex] << std::endl;
        return false;
    }
    
    // Clear move history
    game.moveHistory.clear();
    game.isNewRecord = false;
    
    // Initialize level
    initializeLevel(&game.activeLevel, &game.player, game.activeLevel.playerStartX, game.activeLevel.playerStartY);
    
    return true;
}
