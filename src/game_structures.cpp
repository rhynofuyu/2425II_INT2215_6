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

int currentLevelIndex = 0;

GameData game;

const char* SETTINGS_FILEPATH = "game_settings.dat";

void initializeLevel(Level* level, PlayerInfo* player, int playerStartX, int playerStartY) {
    for (int y = 0; y < level->height; y++) {
        for (int x = 0; x < level->width; x++) {
            level->currentMap[y][x] = level->originalMap[y][x];
        }
    }
    
    player->x = playerStartX;
    player->y = playerStartY;
    
    player->moves = 0;
    player->pushes = 0;
    
    if (level->originalMap[playerStartY][playerStartX] == TARGET) {
        level->currentMap[playerStartY][playerStartX] = PLAYER_ON_TARGET;
    } else {
        level->currentMap[playerStartY][playerStartX] = PLAYER;
    }
}

bool loadLevelFromFile(const char* filename, Level* outLevel) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::vector<std::string> lines;
    std::string line;
    int maxWidth = 0;
    
    while (std::getline(file, line)) {
        lines.push_back(line);
        if (static_cast<int>(line.length()) > maxWidth) {
            maxWidth = line.length();
        }
    }
    
    int height = lines.size();
    int width = maxWidth;
    
    if (height == 0 || width == 0) {
        return false;
    }
    
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
    
    outLevel->width = width;
    outLevel->height = height;
    
    outLevel->originalMap = new TileType*[height];
    outLevel->currentMap = new TileType*[height];
    
    for (int y = 0; y < height; y++) {
        outLevel->originalMap[y] = new TileType[width];
        outLevel->currentMap[y] = new TileType[width];
        
        for (int x = 0; x < width; x++) {
            outLevel->originalMap[y][x] = WALL;
            outLevel->currentMap[y][x] = WALL;
        }
    }
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < static_cast<int>(lines[y].length()); x++) {
            char c = lines[y][x];
            
            switch (c) {
                case '#':
                    outLevel->originalMap[y][x] = WALL;
                    break;
                    
                case ' ':
                    outLevel->originalMap[y][x] = EMPTY;
                    break;
                    
                case '@':
                    outLevel->originalMap[y][x] = EMPTY;
                    outLevel->playerStartX = x;
                    outLevel->playerStartY = y;
                    break;
                    
                case '$':
                    outLevel->originalMap[y][x] = BOX;
                    break;
                    
                case '.':
                    outLevel->originalMap[y][x] = TARGET;
                    break;
                    
                case '*':
                    outLevel->originalMap[y][x] = BOX_ON_TARGET;
                    break;
                    
                case '+':
                    outLevel->originalMap[y][x] = TARGET;
                    outLevel->playerStartX = x;
                    outLevel->playerStartY = y;
                    break;
                    
                default:
                    outLevel->originalMap[y][x] = WALL;
                    break;
            }
        }
    }
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            outLevel->currentMap[y][x] = outLevel->originalMap[y][x];
        }
    }
    
    return true;
}

void recordMove(const MoveRecord& move) {
    game.moveHistory.push_back(move);
}

bool undoMove() {
    if (game.moveHistory.empty()) {
        return false;
    }
    
    MoveRecord lastMove = game.moveHistory.back();
    game.moveHistory.pop_back();
    
    TileType** map = game.activeLevel.currentMap;
    
    Point currentPos = {game.player.x, game.player.y};
    
    if (game.activeLevel.originalMap[currentPos.y][currentPos.x] == TARGET) {
        map[currentPos.y][currentPos.x] = TARGET;
    } else {
        map[currentPos.y][currentPos.x] = EMPTY;
    }
    
    game.player.x = lastMove.playerPos.x;
    game.player.y = lastMove.playerPos.y;
    
    Point oldPos = {game.player.x, game.player.y};
    if (game.activeLevel.originalMap[oldPos.y][oldPos.x] == TARGET) {
        map[oldPos.y][oldPos.x] = PLAYER_ON_TARGET;
    } else {
        map[oldPos.y][oldPos.x] = PLAYER;
    }
    
    if (lastMove.wasBoxMoved) {
        Point boxPos = lastMove.movedBoxPos;
        
        if (game.activeLevel.originalMap[boxPos.y][boxPos.x] == TARGET) {
            map[boxPos.y][boxPos.x] = TARGET;
        } else {
            map[boxPos.y][boxPos.x] = EMPTY;
        }
        
        Point boxPrevPos = lastMove.boxPrevPos;
        if (game.activeLevel.originalMap[boxPrevPos.y][boxPrevPos.x] == TARGET) {
            map[boxPrevPos.y][boxPrevPos.x] = BOX_ON_TARGET;
        } else {
            map[boxPrevPos.y][boxPrevPos.x] = BOX;
        }
        
        game.player.pushes--;
    }
    
    game.player.moves--;
    
    return true;
}

bool loadHighScores(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        game.highScores.resize(totalLoadedLevels);
        return false;
    }
    
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
    
    if (static_cast<int>(game.highScores.size()) < totalLoadedLevels) {
        game.highScores.resize(totalLoadedLevels);
    }
    
    return true;
}

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

bool isNewHighScore(int levelIndex, int moves, int pushes) {
    if (levelIndex >= static_cast<int>(game.highScores.size())) {
        return false;
    }
    
    if (moves < game.highScores[levelIndex].moves) {
        game.highScores[levelIndex].moves = moves;
        game.highScores[levelIndex].pushes = pushes;
        return true;
    }
    else if (moves == game.highScores[levelIndex].moves && 
             pushes < game.highScores[levelIndex].pushes) {
        game.highScores[levelIndex].pushes = pushes;
        return true;
    }
    
    return false;
}

bool loadSettings(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cout << "Settings file not found, using defaults." << std::endl;
        return false;
    }
    
    try {
        int skinValue;
        file.read(reinterpret_cast<char*>(&skinValue), sizeof(int));
        game.settings.currentSkin = static_cast<PlayerSkin>(skinValue);
        
        file.read(reinterpret_cast<char*>(&game.settings.bgmEnabled), sizeof(bool));
        file.read(reinterpret_cast<char*>(&game.settings.sfxEnabled), sizeof(bool));
        file.read(reinterpret_cast<char*>(&game.settings.fullscreenEnabled), sizeof(bool));
        
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

bool saveSettings(const char* filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open settings file for writing." << std::endl;
        return false;
    }
    
    try {
        int skinValue = static_cast<int>(game.settings.currentSkin);
        file.write(reinterpret_cast<const char*>(&skinValue), sizeof(int));
        
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