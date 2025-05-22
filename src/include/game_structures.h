#ifndef GAME_STRUCTURES_H
#define GAME_STRUCTURES_H

#include <SDL2/SDL.h>
#include <vector>
#include <climits>
#include <string>

enum TileType {
    EMPTY,
    WALL,
    PLAYER,
    BOX,
    TARGET,
    BOX_ON_TARGET,
    PLAYER_ON_TARGET
};

enum MenuOption {
    MENU_START_GAME,
    MENU_SELECT_LEVEL,
    MENU_SELECT_SKIN,
    MENU_SETTINGS,
    MENU_QUIT,
    MENU_COUNT
};

enum SettingsOption {
    SETTINGS_BACKGROUND_MUSIC,
    SETTINGS_SOUND_EFFECTS,
    SETTINGS_TUTORIALS,
    SETTINGS_BACK,
    SETTINGS_COUNT
};

enum PlayerSkin {
    SKIN_DEFAULT,
    SKIN_ALT1,
    SKIN_ALT2,
    SKIN_ALT3,
    SKIN_ALT4,
    SKIN_ALT5,
    SKIN_ALT6,
    SKIN_ALT7,
    SKIN_COUNT
};

struct GameSettings {
    PlayerSkin currentSkin;
    bool bgmEnabled;
    bool sfxEnabled;
    bool fullscreenEnabled;
    
    GameSettings() : currentSkin(SKIN_DEFAULT), bgmEnabled(true), 
                    sfxEnabled(true), fullscreenEnabled(false) {}
};

extern const std::string levelFileNames[];
extern int currentLevelIndex;
extern int totalLoadedLevels;

struct Point {
    int x;
    int y;
    
    Point(int _x = 0, int _y = 0) : x(_x), y(_y) {}
    
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

struct MoveRecord {
    Point playerPos;
    Point movedBoxPos;
    Point boxPrevPos;
    bool wasBoxMoved;
    TileType playerPrevTile;
};

struct Level {
    TileType** currentMap;
    TileType** originalMap;
    int width;
    int height;
    int playerStartX;
    int playerStartY;
    
    Level() : currentMap(nullptr), originalMap(nullptr), 
              width(0), height(0), playerStartX(0), playerStartY(0) {}
              
    ~Level() {
        if (currentMap) {
            for (int y = 0; y < height; y++) {
                delete[] currentMap[y];
            }
            delete[] currentMap;
        }
        
        if (originalMap) {
            for (int y = 0; y < height; y++) {
                delete[] originalMap[y];
            }
            delete[] originalMap;
        }
    }
};

struct PlayerInfo {
    int x;
    int y;
    int moves;
    int pushes;
    
    PlayerInfo() : x(0), y(0), moves(0), pushes(0) {}
};

struct HighScore {
    int moves;
    int pushes;
    
    HighScore() : moves(INT_MAX), pushes(INT_MAX) {}
};

enum GameState {
    MENU,
    PLAYING,
    LEVEL_COMPLETE,
    GAME_OVER,
    LEVEL_SELECT,
    SETTINGS,
    SKIN_SELECT
};

struct GameData {
    Level activeLevel;
    PlayerInfo player;
    GameState currentState;
    std::vector<MoveRecord> moveHistory;
    std::vector<HighScore> highScores;
    bool isNewRecord;
    GameSettings settings;
    
    GameData() : currentState(MENU), isNewRecord(false) {}
};

extern GameData game;

void initializeLevel(Level* level, PlayerInfo* player, int playerStartX, int playerStartY);
bool loadLevelFromFile(const char* filename, Level* outLevel);
void recordMove(const MoveRecord& move);
bool undoMove();
bool loadHighScores(const char* filename);
bool saveHighScores(const char* filename);
bool isNewHighScore(int levelIndex, int moves, int pushes);

#endif