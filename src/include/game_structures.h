#ifndef GAME_STRUCTURES_H
#define GAME_STRUCTURES_H

#include <SDL2/SDL.h>
#include <vector>
#include <climits>
#include <string>

// Define tile types for the Sokoban game
enum TileType {
    EMPTY,              // Empty space (-)
    WALL,               // Wall - cannot pass through (+)
    PLAYER,             // Player position (*)
    BOX,                // Box/crate that can be pushed (@)
    TARGET,             // Target position for boxes (X)
    BOX_ON_TARGET,      // Box placed on a target ($)
    PLAYER_ON_TARGET    // Player standing on a target (%)
};

// Menu options enum
enum MenuOption {
    MENU_START_GAME,
    MENU_SELECT_LEVEL,  // Option to select specific level
    MENU_SELECT_SKIN,   // Option to select player skin
    MENU_SETTINGS,      // Settings option
    MENU_QUIT,
    MENU_COUNT          // Used to track the number of menu options
};

// Settings menu options enum
enum SettingsOption {
    SETTINGS_BACKGROUND_MUSIC,
    SETTINGS_SOUND_EFFECTS,
    SETTINGS_BACK,
    SETTINGS_COUNT
};

// Player skin options
enum PlayerSkin {
    SKIN_DEFAULT,
    SKIN_ALT1,
    SKIN_ALT2,
    SKIN_ALT3,
    SKIN_ALT4,
    SKIN_COUNT
};

// Game settings structure
struct GameSettings {
    PlayerSkin currentSkin;
    bool bgmEnabled;
    bool sfxEnabled;
    bool fullscreenEnabled;
    
    // Constructor with default values
    GameSettings() : currentSkin(SKIN_DEFAULT), bgmEnabled(true), 
                    sfxEnabled(true), fullscreenEnabled(false) {}
};

// Khai báo mảng tên file level
extern const std::string levelFileNames[];

// Level file name constants
extern int currentLevelIndex;

// Khai báo biến toàn cục để theo dõi số lượng level đã được tải
extern int totalLoadedLevels;

// Structure to store a point on the map
struct Point {
    int x;
    int y;
    
    Point(int _x = 0, int _y = 0) : x(_x), y(_y) {}
    
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

// Structure to record a move for undo functionality
struct MoveRecord {
    Point playerPos;         // Previous player position
    Point movedBoxPos;       // Position of the box that was moved (if any)
    Point boxPrevPos;        // Previous position of the moved box
    bool wasBoxMoved;        // Whether a box was moved in this move
    TileType playerPrevTile; // Type of tile where the player was
};

// Structure for level data
struct Level {
    TileType** currentMap;   // Current state of the map
    TileType** originalMap;  // Original state for resetting
    int width;               // Width of the map (in tiles)
    int height;              // Height of the map (in tiles)
    int playerStartX;        // Initial player X position
    int playerStartY;        // Initial player Y position
    
    // Constructor
    Level() : currentMap(nullptr), originalMap(nullptr), 
              width(0), height(0), playerStartX(0), playerStartY(0) {}
              
    // Destructor for proper cleanup
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

// Structure for player information
struct PlayerInfo {
    int x;          // Current X position (in tiles)
    int y;          // Current Y position (in tiles)
    int moves;      // Number of moves made
    int pushes;     // Number of times boxes were pushed
    
    // Constructor with default values
    PlayerInfo() : x(0), y(0), moves(0), pushes(0) {}
};

// Structure for high scores
struct HighScore {
    int moves;      // Lowest number of moves
    int pushes;     // Lowest number of pushes
    
    // Constructor with default values (max integer values)
    HighScore() : moves(INT_MAX), pushes(INT_MAX) {}
};

// Game state enumeration
enum GameState {
    MENU,           // Main menu
    PLAYING,        // Active gameplay
    LEVEL_COMPLETE, // Current level completed
    GAME_OVER,      // Game completed
    LEVEL_SELECT,   // Level selection screen
    SETTINGS,       // Settings menu
    SKIN_SELECT     // Player skin selection screen
};

// Main game data structure
struct GameData {
    Level activeLevel;                // Current active level
    PlayerInfo player;                // Player information
    GameState currentState;           // Current game state
    std::vector<MoveRecord> moveHistory; // History of moves for undo functionality
    std::vector<HighScore> highScores;   // High scores for each level
    bool isNewRecord;                    // Flag to indicate if a new high score was achieved
    GameSettings settings;               // Game settings
    
    // Constructor with default values
    GameData() : currentState(MENU), isNewRecord(false) {}
};

// Global game instance
extern GameData game;

// Function to initialize a level
void initializeLevel(Level* level, PlayerInfo* player, int playerStartX, int playerStartY);

// Function to load a level from a text file
bool loadLevelFromFile(const char* filename, Level* outLevel);

// Function to record a move for undo functionality
void recordMove(const MoveRecord& move);

// Function to undo the last move
bool undoMove();

// Functions for high score management
bool loadHighScores(const char* filename);
bool saveHighScores(const char* filename);
bool isNewHighScore(int levelIndex, int moves, int pushes);

#endif // GAME_STRUCTURES_H