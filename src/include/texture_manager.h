#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "game_structures.h"

#define WALL_TEXTURE_COUNT 4

struct TextureManager {
    SDL_Texture* tileTextures[7];
    SDL_Texture* wallTextures[WALL_TEXTURE_COUNT];
    SDL_Texture* playerSkins[SKIN_COUNT][2];
    
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
    
    ~TextureManager() {
        destroyTextures();
    }
    
    bool loadTextures(SDL_Renderer* renderer);
    bool loadPlayerSkins(SDL_Renderer* renderer);
    void destroyTextures();
};

void renderLevel(SDL_Renderer* renderer, const Level& level, const PlayerInfo& player, TextureManager& textures);

struct MusicManager {
    Mix_Music* bgm;
    Mix_Chunk* sfx[3];
    
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

bool loadSettings(const char* filename);
bool saveSettings(const char* filename);

#endif