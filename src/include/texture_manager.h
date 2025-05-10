#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "game_structures.h"

// Define the number of different wall textures
#define WALL_TEXTURE_COUNT 4

// Structure to hold game textures
struct TextureManager {
    SDL_Texture* tileTextures[7]; // One texture for each TileType
    SDL_Texture* wallTextures[WALL_TEXTURE_COUNT]; // Array to store multiple wall textures
    SDL_Texture* playerSkins[SKIN_COUNT][2]; // [skin][0=normal, 1=on_target]
      // Constructor - initialize pointers to nullptr
    TextureManager() {
        for (int i = 0; i < 7; i++) {
            tileTextures[i] = nullptr;
        }
        
        for (int i = 0; i < WALL_TEXTURE_COUNT; i++) {
            wallTextures[i] = nullptr;
        }
        
        for (int i = 0; i < SKIN_COUNT; i++) {
            for (int j = 0; j < 2; j++) {
                playerSkins[i][j] = nullptr;
            }
        }
    }
    
    // Destructor - cleanup textures
    ~TextureManager() {
        destroyTextures();
    }
    
    // Load all game textures
    bool loadTextures(SDL_Renderer* renderer);
    
    // Load player skin textures
    bool loadPlayerSkins(SDL_Renderer* renderer);
    
    // Clean up and destroy all textures
    void destroyTextures();
};

// Function to render the level using textures
void renderLevel(SDL_Renderer* renderer, const Level& level, const PlayerInfo& player, TextureManager& textures);

// Game music manager
struct MusicManager {
    Mix_Music* bgm;
    Mix_Chunk* sfx[3]; // For move, push, and level complete sounds
    
    MusicManager() : bgm(nullptr) {
        for (int i = 0; i < 3; i++) {
            sfx[i] = nullptr;
        }
    }
    
    ~MusicManager() {
        if (bgm) {
            Mix_FreeMusic(bgm);
            bgm = nullptr;
        }
        
        for (int i = 0; i < 3; i++) {
            if (sfx[i]) {
                Mix_FreeChunk(sfx[i]);
                sfx[i] = nullptr;
            }
        }
    }
    
    bool loadAudio();
    void playBGM() const;
    void stopBGM() const;
    void playMove() const;
    void playPush() const;
    void playComplete() const;
};

extern TextureManager gameTextures;
extern MusicManager gameSounds;

// Functions to save/load settings
bool loadSettings(const char* filename);
bool saveSettings(const char* filename);

#endif // TEXTURE_MANAGER_H