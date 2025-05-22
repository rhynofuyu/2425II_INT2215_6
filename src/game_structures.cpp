#include "include/game_structures.h"
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

const std::string levelFileNames[] = {
    "levels/level1.txt",
    "levels/level2.txt",
    "levels/level3.txt",
    "levels/level4.txt",
    "levels/level5.txt",
    "levels/level6.txt",
    "levels/level7.txt",
    "levels/level8.txt",
    "levels/level9.txt",
    "levels/level10.txt",
    "levels/level11.txt",
    "levels/level12.txt",
    "levels/level13.txt",
    "levels/level14.txt",
    "levels/level15.txt",
    "levels/level16.txt",
    "levels/level17.txt",
    "levels/level18.txt",
    "levels/level19.txt",
    "levels/level20.txt"
};

int totalLoadedLevels = sizeof(levelFileNames) / sizeof(levelFileNames[0]);

// Biến theo dõi level hiện tại
int currentLevelIndex = 0;

// Khởi tạo biến game toàn cục
GameData game;

// Filepath constants for settings
const char* SETTINGS_FILEPATH = "game_settings.dat";

// Function to initialize a level
void initializeLevel(Level* level, PlayerInfo* player, int playerStartX, int playerStartY) {
    // Copy original map to current map
    for (int y = 0; y < level->height; y++) {
        for (int x = 0; x < level->width; x++) {
            level->currentMap[y][x] = level->originalMap[y][x];
        }
    }
    
    // Set player position
    player->x = playerStartX;
    player->y = playerStartY;
    
    // Reset player stats
    player->moves = 0;
    player->pushes = 0;
    
    // Hiển thị nhân vật trên bản đồ tại vị trí khởi tạo
    // Kiểm tra xem vị trí ban đầu có phải là target hay không
    if (level->originalMap[playerStartY][playerStartX] == TARGET) {
        level->currentMap[playerStartY][playerStartX] = PLAYER_ON_TARGET;
    } else {
        level->currentMap[playerStartY][playerStartX] = PLAYER;
    }
}

// Function to load a level from a text file
bool loadLevelFromFile(const char* filename, Level* outLevel) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false; // Failed to open file
    }
    
    // Read all lines from file to determine dimensions
    std::vector<std::string> lines;
    std::string line;
    int maxWidth = 0;
    
    while (std::getline(file, line)) {
        lines.push_back(line);
        if (static_cast<int>(line.length()) > maxWidth) {
            maxWidth = line.length();
        }
    }
    
    // Set dimensions
    int height = lines.size();
    int width = maxWidth;
    
    if (height == 0 || width == 0) {
        return false; // Empty or invalid file
    }
    
    // Clean up any existing maps in the level
    if (outLevel->currentMap) {
        for (int y = 0; y < outLevel->height; y++) {
            delete[] outLevel->currentMap[y];
        }
        delete[] outLevel->currentMap;
        outLevel->currentMap = nullptr;
    }
    
    if (outLevel->originalMap) {
        for (int y = 0; y < outLevel->height; y++) {
            delete[] outLevel->originalMap[y];
        }
        delete[] outLevel->originalMap;
        outLevel->originalMap = nullptr;
    }
    
    // Allocate memory for the maps
    outLevel->width = width;
    outLevel->height = height;
    
    outLevel->originalMap = new TileType*[height];
    outLevel->currentMap = new TileType*[height];
    
    for (int y = 0; y < height; y++) {
        outLevel->originalMap[y] = new TileType[width];
        outLevel->currentMap[y] = new TileType[width];
        
        // Initialize with WALL for areas outside of the map shape
        for (int x = 0; x < width; x++) {
            outLevel->originalMap[y][x] = WALL; // Default to wall for empty areas
            outLevel->currentMap[y][x] = WALL;
        }
    }
    
    // Parse the file content
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < static_cast<int>(lines[y].length()); x++) {
            char c = lines[y][x];
            
            switch (c) {
                case '#': // Wall
                    outLevel->originalMap[y][x] = WALL;
                    break;
                    
                case ' ': // Empty floor
                    outLevel->originalMap[y][x] = EMPTY;
                    break;
                    
                case '@': // Player
                    outLevel->originalMap[y][x] = EMPTY; // The tile underneath is empty
                    outLevel->playerStartX = x;
                    outLevel->playerStartY = y;
                    break;
                    
                case '$': // Box
                    outLevel->originalMap[y][x] = BOX;
                    break;
                    
                case '.': // Target
                    outLevel->originalMap[y][x] = TARGET;
                    break;
                    
                case '*': // Box on target (for compatibility with standard Sokoban format)
                    outLevel->originalMap[y][x] = BOX_ON_TARGET;
                    break;
                    
                case '+': // Player on target (for compatibility with standard Sokoban format)
                    outLevel->originalMap[y][x] = TARGET; // The tile underneath is a target
                    outLevel->playerStartX = x;
                    outLevel->playerStartY = y;
                    break;
                    
                default: // Treat any other character as wall to prevent access
                    outLevel->originalMap[y][x] = WALL;
                    break;
            }
        }
    }
    
    // Copy originalMap to currentMap
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            outLevel->currentMap[y][x] = outLevel->originalMap[y][x];
        }
    }
    
    return true;
}

// Record a move for undo functionality
void recordMove(const MoveRecord& move) {
    game.moveHistory.push_back(move);
}

// Undo the last move
bool undoMove() {
    if (game.moveHistory.empty()) {
        return false; // No moves to undo
    }
    
    // Get the last move record
    MoveRecord lastMove = game.moveHistory.back();
    game.moveHistory.pop_back();
    
    // Get reference to current map
    TileType** map = game.activeLevel.currentMap;
    
    // First, clear the player's current position (which is where they moved to)
    Point currentPos = {game.player.x, game.player.y};
    
    // Restore the original tile type where the player is currently
    if (game.activeLevel.originalMap[currentPos.y][currentPos.x] == TARGET) {
        map[currentPos.y][currentPos.x] = TARGET;
    } else {
        map[currentPos.y][currentPos.x] = EMPTY;
    }
    
    // Revert player position
    game.player.x = lastMove.playerPos.x;
    game.player.y = lastMove.playerPos.y;
    
    // Set player's restored position
    Point oldPos = {game.player.x, game.player.y};
    if (game.activeLevel.originalMap[oldPos.y][oldPos.x] == TARGET) {
        map[oldPos.y][oldPos.x] = PLAYER_ON_TARGET;
    } else {
        map[oldPos.y][oldPos.x] = PLAYER;
    }
    
    // If a box was moved, revert its position too
    if (lastMove.wasBoxMoved) {
        // Remove box from its current position
        Point boxPos = lastMove.movedBoxPos;
        
        // Check if box was on a target
        if (game.activeLevel.originalMap[boxPos.y][boxPos.x] == TARGET) {
            map[boxPos.y][boxPos.x] = TARGET;
        } else {
            map[boxPos.y][boxPos.x] = EMPTY;
        }
        
        // Put box back to its previous position
        Point boxPrevPos = lastMove.boxPrevPos;
        if (game.activeLevel.originalMap[boxPrevPos.y][boxPrevPos.x] == TARGET) {
            map[boxPrevPos.y][boxPrevPos.x] = BOX_ON_TARGET;
        } else {
            map[boxPrevPos.y][boxPrevPos.x] = BOX;
        }
        
        // Decrement push counter
        game.player.pushes--;
    }
    
    // Decrement move counter
    game.player.moves--;
    
    return true;
}

// Load high scores from file
bool loadHighScores(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        // If file doesn't exist, initialize with default values
        game.highScores.resize(totalLoadedLevels);
        return false;
    }
    
    // Clear existing high scores
    game.highScores.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        int moves, pushes;
        
        if (ss >> moves >> pushes) {
            HighScore score;
            score.moves = moves;
            score.pushes = pushes;
            game.highScores.push_back(score);
        }
    }
    
    // Make sure we have enough scores for all levels
    if (static_cast<int>(game.highScores.size()) < totalLoadedLevels) {
        game.highScores.resize(totalLoadedLevels);
    }
    
    return true;
}

// Save high scores to file
bool saveHighScores(const char* filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& score : game.highScores) {
        file << score.moves << " " << score.pushes << std::endl;
    }
    
    return true;
}

// Check if current score is a new high score
bool isNewHighScore(int levelIndex, int moves, int pushes) {
    if (levelIndex >= static_cast<int>(game.highScores.size())) {
        return false;
    }
    
    // Check if moves count is better (lower)
    if (moves < game.highScores[levelIndex].moves) {
        game.highScores[levelIndex].moves = moves;
        game.highScores[levelIndex].pushes = pushes;
        return true;
    }
    // If moves are equal, check if pushes is better
    else if (moves == game.highScores[levelIndex].moves && 
             pushes < game.highScores[levelIndex].pushes) {
        game.highScores[levelIndex].pushes = pushes;
        return true;
    }
    
    return false;
}

// Load settings from file
bool loadSettings(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        // If file doesn't exist, use default settings
        std::cout << "Settings file not found, using defaults." << std::endl;
        return false;
    }
    
    // Read settings data
    try {
        // Read skin
        int skinValue;
        file.read(reinterpret_cast<char*>(&skinValue), sizeof(int));
        game.settings.currentSkin = static_cast<PlayerSkin>(skinValue);
        
        // Read other settings
        file.read(reinterpret_cast<char*>(&game.settings.bgmEnabled), sizeof(bool));
        file.read(reinterpret_cast<char*>(&game.settings.sfxEnabled), sizeof(bool));
        file.read(reinterpret_cast<char*>(&game.settings.fullscreenEnabled), sizeof(bool));
        
        // Validate skin value
        if (game.settings.currentSkin < 0 || game.settings.currentSkin >= SKIN_COUNT) {
            game.settings.currentSkin = SKIN_DEFAULT;
        }
    } 
    catch(std::exception& e) {
        std::cerr << "Error reading settings file: " << e.what() << std::endl;
        return false;
    }
    
    return true;
}

// Save settings to file
bool saveSettings(const char* filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open settings file for writing." << std::endl;
        return false;
    }
    
    try {
        // Write skin as integer
        int skinValue = static_cast<int>(game.settings.currentSkin);
        file.write(reinterpret_cast<const char*>(&skinValue), sizeof(int));
        
        // Write other settings
        file.write(reinterpret_cast<const char*>(&game.settings.bgmEnabled), sizeof(bool));
        file.write(reinterpret_cast<const char*>(&game.settings.sfxEnabled), sizeof(bool));
        file.write(reinterpret_cast<const char*>(&game.settings.fullscreenEnabled), sizeof(bool));
    }
    catch(std::exception& e) {
        std::cerr << "Error writing settings file: " << e.what() << std::endl;
        return false;
    }
    
    return true;
}