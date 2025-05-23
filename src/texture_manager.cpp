#include "include/texture_manager.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <random>

TextureManager gameTextures;
MusicManager gameSounds;

bool TextureManager::loadTextures(SDL_Renderer* renderer) {
    const char* texturePaths[] = {
        "assets/images/floor.png",
        "assets/images/wall.png",
        "assets/images/player.png", 
        "assets/images/box.png",
        "assets/images/target.png",
        "assets/images/box_on_target.png",
        "assets/images/player_on_target.png"
    };
    
    bool success = true;
    for (int i = 0; i < 7; i++) {
        SDL_Surface* surface = IMG_Load(texturePaths[i]);
        if (!surface) {
            std::cerr << "Failed to load image " << texturePaths[i] << ": " << IMG_GetError() << std::endl;
            success = false;
        } else {
            tileTextures[i] = SDL_CreateTextureFromSurface(renderer, surface);
            if (!tileTextures[i]) {
                std::cerr << "Failed to create texture from " << texturePaths[i] << ": " << SDL_GetError() << std::endl;
                success = false;
            }
            SDL_FreeSurface(surface);
        }
    }
    
    for (int i = 0; i < WALL_TEXTURE_COUNT; i++) {
        std::string wallPath = "assets/images/wall" + std::to_string(i + 1) + ".png";
        
        SDL_Surface* surface = IMG_Load(wallPath.c_str());
        if (!surface) {
            std::cerr << "Failed to load wall texture " << wallPath << ": " << IMG_GetError() << std::endl;
            wallTextures[i] = tileTextures[WALL];
        } else {
            SDL_Texture* newTexture = SDL_CreateTextureFromSurface(renderer, surface);
            
            if (!newTexture) {
                std::cerr << "Failed to create texture from " << wallPath << ": " << SDL_GetError() << std::endl;
                wallTextures[i] = tileTextures[WALL];
            } else {
                if (i == 0) {
                    tileTextures[WALL] = newTexture;
                }
                
                wallTextures[i] = newTexture;
            }
            
            SDL_FreeSurface(surface);
        }
    }
    
    bool anyWallTexturesLoaded = false;
    for (int i = 0; i < WALL_TEXTURE_COUNT; i++) {
        if (wallTextures[i] != nullptr && wallTextures[i] != tileTextures[WALL]) {
            anyWallTexturesLoaded = true;
            break;
        }
    }
    
    if (!anyWallTexturesLoaded) {
        for (int i = 0; i < WALL_TEXTURE_COUNT; i++) {
            wallTextures[i] = tileTextures[WALL];
        }
    }
    
    if (!loadPlayerSkins(renderer)) {
        std::cerr << "Failed to load player skin textures." << std::endl;
        success = false;
    }
    
    return success;
}

bool TextureManager::loadPlayerSkins(SDL_Renderer* renderer) {
    playerSkins[SKIN_DEFAULT][0] = tileTextures[PLAYER];
    playerSkins[SKIN_DEFAULT][1] = tileTextures[PLAYER_ON_TARGET];

    for (int i = 1; i < SKIN_COUNT; i++) {
        playerSkins[i][0] = tileTextures[PLAYER];
        playerSkins[i][1] = tileTextures[PLAYER_ON_TARGET];
    }
    
    const char* skinBasePaths[SKIN_COUNT][2] = {
        {"assets/images/players/default/player.png", "assets/images/players/default/player_on_target.png"},
        {"assets/images/players/alt1/player.png", "assets/images/players/alt1/player_on_target.png"},
        {"assets/images/players/alt2/player.png", "assets/images/players/alt2/player_on_target.png"},
        {"assets/images/players/alt3/player.png", "assets/images/players/alt3/player_on_target.png"},
        {"assets/images/players/alt4/player.png", "assets/images/players/alt4/player_on_target.png"},
        {"assets/images/players/alt5/player.png", "assets/images/players/alt5/player_on_target.png"},
        {"assets/images/players/alt6/player.png", "assets/images/players/alt6/player_on_target.png"},
        {"assets/images/players/alt7/player.png", "assets/images/players/alt7/player_on_target.png"}
    };
    
    for (int i = 0; i < SKIN_COUNT; i++) {
        for (int j = 0; j < 2; j++) {
            SDL_Surface* surface = IMG_Load(skinBasePaths[i][j]);
            if (surface) {
                SDL_Texture* newTexture = SDL_CreateTextureFromSurface(renderer, surface);
                if (newTexture) {
                    if (i == 0) {
                        if (j == 0) SDL_DestroyTexture(tileTextures[PLAYER]);
                        if (j == 1) SDL_DestroyTexture(tileTextures[PLAYER_ON_TARGET]);
                        
                        playerSkins[i][j] = newTexture;
                        if (j == 0) tileTextures[PLAYER] = newTexture;
                        if (j == 1) tileTextures[PLAYER_ON_TARGET] = newTexture;
                    } else {
                        playerSkins[i][j] = newTexture;
                    }
                }
                SDL_FreeSurface(surface);
            } else {
                std::cerr << "Warning: Could not load skin image " << skinBasePaths[i][j] << std::endl;
                std::cerr << "SDL_image Error: " << IMG_GetError() << std::endl;
                playerSkins[i][j] = (j == 0) ? tileTextures[PLAYER] : tileTextures[PLAYER_ON_TARGET];
            }
        }
    }
    
    return true;
}

void TextureManager::destroyTextures() {
    for (int i = 0; i < 7; i++) {
        if (tileTextures[i]) {
            SDL_DestroyTexture(tileTextures[i]);
            tileTextures[i] = nullptr;
        }
    }
    
    for (int i = 0; i < WALL_TEXTURE_COUNT; i++) {
        if (wallTextures[i]) {
            SDL_DestroyTexture(wallTextures[i]);
            wallTextures[i] = nullptr;
        }
    }
    
    for (int i = 0; i < SKIN_COUNT; i++) {
        for (int j = 0; j < 2; j++) {
            if (playerSkins[i][j] && 
                playerSkins[i][j] != tileTextures[PLAYER] && 
                playerSkins[i][j] != tileTextures[PLAYER_ON_TARGET]) {
                SDL_DestroyTexture(playerSkins[i][j]);
                playerSkins[i][j] = nullptr;
            }
        }
    }
}

extern SDL_Texture* gameLevelBackgroundTexture;

void renderLevel(SDL_Renderer* renderer, const Level& level, const PlayerInfo& player, TextureManager& textures) {
    const int TILE_SIZE = 40;
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> distribution(0, WALL_TEXTURE_COUNT - 1);
    
    int windowWidth = 1280;
    int windowHeight = 720;
    
    int levelPixelWidth = level.width * TILE_SIZE;
    int levelPixelHeight = level.height * TILE_SIZE;
    int offsetX = (windowWidth - levelPixelWidth) / 2;
    int offsetY = (windowHeight - levelPixelHeight) / 2;
    
    for (int y = 0; y < level.height; y++) {
        for (int x = 0; x < level.width; x++) {
            TileType tile = level.currentMap[y][x];
            
            if (tile == EMPTY) {
                tile = EMPTY;
            }
            
            SDL_Rect destRect = {
                offsetX + x * TILE_SIZE,
                offsetY + y * TILE_SIZE,
                TILE_SIZE,
                TILE_SIZE
            };
            
            SDL_Texture* texture = nullptr;
            
            if (tile == PLAYER) {
                texture = textures.playerSkins[game.settings.currentSkin][0];
            }
            else if (tile == PLAYER_ON_TARGET) {
                texture = textures.playerSkins[game.settings.currentSkin][1];
            }
            else if (tile == WALL) {
                int seed = (y * level.width + x) * 12345;
                int randomIndex = seed % WALL_TEXTURE_COUNT;
                texture = textures.wallTextures[randomIndex];
            }
            else {
                texture = textures.tileTextures[tile];
            }
            
            if (tile != WALL && tile != EMPTY) {
                SDL_RenderCopy(renderer, textures.tileTextures[EMPTY], nullptr, &destRect);
            }
            
            if (texture) {
                SDL_RenderCopy(renderer, texture, nullptr, &destRect);
            }
        }
    }
}

bool MusicManager::loadAudio() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }
    
    bgm = Mix_LoadMUS("assets/sounds/bgm/background.mp3");
    if (!bgm) {
        std::cerr << "Failed to load background music! SDL_mixer Error: " << Mix_GetError() << std::endl;
    }
    
    const char* sfxPaths[] = {
        "assets/sounds/move.wav",
        "assets/sounds/push.wav",
        "assets/sounds/complete.wav"
    };
    
    bool success = true;
    for (int i = 0; i < 3; i++) {
        sfx[i] = Mix_LoadWAV(sfxPaths[i]);
        if (!sfx[i]) {
            std::cerr << "Failed to load sound effect " << sfxPaths[i] << "! SDL_mixer Error: " << Mix_GetError() << std::endl;
            success = false;
        }
    }
    
    return success;
}

void MusicManager::playBGM() const {
    if (bgm && game.settings.bgmEnabled && Mix_PlayingMusic() == 0) {
        Mix_PlayMusic(bgm, -1);
    }
}

void MusicManager::stopBGM() const {
    if (Mix_PlayingMusic()) {
        Mix_HaltMusic();
    }
}

void MusicManager::playMove() const {
    if (sfx[0] && game.settings.sfxEnabled) {
        Mix_PlayChannel(-1, sfx[0], 0);
    }
}

void MusicManager::playPush() const {
    if (sfx[1] && game.settings.sfxEnabled) {
        Mix_PlayChannel(-1, sfx[1], 0);
    }
}

void MusicManager::playComplete() const {
    if (sfx[2] && game.settings.sfxEnabled) {
        Mix_PlayChannel(-1, sfx[2], 0);
    }
}