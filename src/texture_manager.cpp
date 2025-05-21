#include "include/texture_manager.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <random> // Add this for random number generation

// Declare the global texture manager instance
TextureManager gameTextures;
MusicManager gameSounds;

// Load all game textures
bool TextureManager::loadTextures(SDL_Renderer* renderer) {
    // File paths for each tile type
    const char* texturePaths[] = {
        "assets/images/floor.png",
        "assets/images/wall.png", // We'll keep this as a fallback
        "assets/images/player.png", 
        "assets/images/box.png",
        "assets/images/target.png",
        "assets/images/box_on_target.png",
        "assets/images/player_on_target.png"
    };
    
    // Load each texture
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
      // Load the multiple wall textures
    
    // For each wall texture (wall1.png through wall4.png)
    for (int i = 0; i < WALL_TEXTURE_COUNT; i++) {
        // Create the file path for each wall texture
        std::string wallPath = "assets/images/wall" + std::to_string(i + 1) + ".png";
        
        SDL_Surface* surface = IMG_Load(wallPath.c_str());
        if (!surface) {
            std::cerr << "Failed to load wall texture " << wallPath << ": " << IMG_GetError() << std::endl;
            // Use the default wall texture as fallback
            wallTextures[i] = tileTextures[WALL];
        } else {
            // Create texture from surface
            SDL_Texture* newTexture = SDL_CreateTextureFromSurface(renderer, surface);
            
            if (!newTexture) {
                std::cerr << "Failed to create texture from " << wallPath << ": " << SDL_GetError() << std::endl;
                wallTextures[i] = tileTextures[WALL]; // Use the default wall as fallback
            } else {
                // If this is the first wall texture (wall1.png), update the default wall texture too
                if (i == 0) {
                    // Replace the default wall texture in tileTextures array too
                    tileTextures[WALL] = newTexture;
                }
                
                wallTextures[i] = newTexture;
            }
            
            SDL_FreeSurface(surface);
        }
    }
    
    // Check if any wall textures loaded successfully, if not use the default wall texture for all
    bool anyWallTexturesLoaded = false;
    for (int i = 0; i < WALL_TEXTURE_COUNT; i++) {
        if (wallTextures[i] != nullptr && wallTextures[i] != tileTextures[WALL]) {
            anyWallTexturesLoaded = true;
            break;
        }
    }
    
    // If no custom wall textures were loaded, make sure all slots have the default wall texture
    if (!anyWallTexturesLoaded) {
        for (int i = 0; i < WALL_TEXTURE_COUNT; i++) {
            wallTextures[i] = tileTextures[WALL];
        }
    }
    
    // Load player skins
    if (!loadPlayerSkins(renderer)) {
        std::cerr << "Failed to load player skin textures." << std::endl;
        success = false;
    }
    
    return success;
}

// Load player skin textures
bool TextureManager::loadPlayerSkins(SDL_Renderer* renderer) {
    // Default skin paths (already loaded in base textures)
    playerSkins[SKIN_DEFAULT][0] = tileTextures[PLAYER]; // Regular player
    playerSkins[SKIN_DEFAULT][1] = tileTextures[PLAYER_ON_TARGET]; // Player on target

    // For now, let's copy the default skin for the other skins
    // These will be replaced with custom skins when available
    for (int i = 1; i < SKIN_COUNT; i++) {
        playerSkins[i][0] = tileTextures[PLAYER]; // Regular player
        playerSkins[i][1] = tileTextures[PLAYER_ON_TARGET]; // Player on target
    }    // Try loading alternate skins if files exist
    const char* skinBasePaths[SKIN_COUNT][2] = {
        {"assets/images/players/default/player.png", "assets/images/players/default/player_on_target.png"},
        {"assets/images/players/alt1/player.png", "assets/images/players/alt1/player_on_target.png"},
        {"assets/images/players/alt2/player.png", "assets/images/players/alt2/player_on_target.png"},
        {"assets/images/players/alt3/player.png", "assets/images/players/alt3/player_on_target.png"},
        {"assets/images/players/alt4/player.png", "assets/images/players/alt4/player_on_target.png"},
        {"assets/images/players/alt5/player.png", "assets/images/players/alt5/player_on_target.png"},  // Kera Candy
        {"assets/images/players/alt6/player.png", "assets/images/players/alt6/player_on_target.png"},  // Win Sweet
        {"assets/images/players/alt7/player.png", "assets/images/players/alt7/player_on_target.png"}   // When Event
    };
    
    for (int i = 0; i < SKIN_COUNT; i++) {
        for (int j = 0; j < 2; j++) {
            SDL_Surface* surface = IMG_Load(skinBasePaths[i][j]);
            if (surface) {
                // If we loaded a new custom skin, replace the reference
                SDL_Texture* newTexture = SDL_CreateTextureFromSurface(renderer, surface);
                if (newTexture) {
                    // Replace the texture with the custom one
                    if (i == 0) {
                        // For default skin, update the tile textures too
                        if (j == 0) SDL_DestroyTexture(tileTextures[PLAYER]);
                        if (j == 1) SDL_DestroyTexture(tileTextures[PLAYER_ON_TARGET]);
                        
                        playerSkins[i][j] = newTexture;
                        if (j == 0) tileTextures[PLAYER] = newTexture;
                        if (j == 1) tileTextures[PLAYER_ON_TARGET] = newTexture;
                    } else {
                        // For other skins, just update the skin array
                        playerSkins[i][j] = newTexture;
                    }
                }
                SDL_FreeSurface(surface);
            } else {
                // If file doesn't exist, make sure we have a fallback
                std::cerr << "Warning: Could not load skin image " << skinBasePaths[i][j] << std::endl;
                std::cerr << "SDL_image Error: " << IMG_GetError() << std::endl;
                // Use the default texture as fallback
                playerSkins[i][j] = (j == 0) ? tileTextures[PLAYER] : tileTextures[PLAYER_ON_TARGET];
            }
        }
    }
    
    return true;
}

// Clean up and destroy all textures
void TextureManager::destroyTextures() {
    // First, destroy base tile textures
    for (int i = 0; i < 7; i++) {
        if (tileTextures[i]) {
            SDL_DestroyTexture(tileTextures[i]);
            tileTextures[i] = nullptr;
        }
    }
    
    // Destroy the wall textures
    for (int i = 0; i < WALL_TEXTURE_COUNT; i++) {
        if (wallTextures[i]) {
            SDL_DestroyTexture(wallTextures[i]);
            wallTextures[i] = nullptr;
        }
    }
    
    // Then destroy all player skin textures that aren't the default ones
    // We skip index 0 for each skin which points to the default base textures
    for (int i = 0; i < SKIN_COUNT; i++) {
        for (int j = 0; j < 2; j++) {
            // Make sure we don't double free textures that are also in tileTextures
            if (playerSkins[i][j] && 
                playerSkins[i][j] != tileTextures[PLAYER] && 
                playerSkins[i][j] != tileTextures[PLAYER_ON_TARGET]) {
                SDL_DestroyTexture(playerSkins[i][j]);
                playerSkins[i][j] = nullptr;
            }
        }
    }
}

// External variables for background (declared in main.cpp)
extern SDL_Texture* gameLevelBackgroundTexture;

// Render the level with textures
void renderLevel(SDL_Renderer* renderer, const Level& level, const PlayerInfo& player, TextureManager& textures) {
    const int TILE_SIZE = 40;  // Size of each tile in pixels
    
    // Random number generator for wall textures
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> distribution(0, WALL_TEXTURE_COUNT - 1);
    
    // Calculate viewport centering
    int windowWidth = 1280;  // Assumed window width
    int windowHeight = 720;  // Assumed window height
      // Note: The main background is now drawn in main.cpp before calling renderLevel
    
    // Calculate the level's pixel dimensions and position
    int levelPixelWidth = level.width * TILE_SIZE;
    int levelPixelHeight = level.height * TILE_SIZE;
    int offsetX = (windowWidth - levelPixelWidth) / 2;
    int offsetY = (windowHeight - levelPixelHeight) / 2;
      // The viewport offsets are now calculated above
    
    // Render each tile
    for (int y = 0; y < level.height; y++) {
        for (int x = 0; x < level.width; x++) {
            // Get tile type at this position
            TileType tile = level.currentMap[y][x];
            
            // Skip empty tiles (will show background)
            if (tile == EMPTY) {
                tile = EMPTY;  // Explicitly set to EMPTY for clarity
            }
            
            // Set the destination rectangle for rendering
            SDL_Rect destRect = {
                offsetX + x * TILE_SIZE,
                offsetY + y * TILE_SIZE,
                TILE_SIZE,
                TILE_SIZE
            };
              // Get the appropriate texture
            SDL_Texture* texture = nullptr;
            
            // Handle special case for player tiles (use the skin)
            if (tile == PLAYER) {
                texture = textures.playerSkins[game.settings.currentSkin][0];
            }
            else if (tile == PLAYER_ON_TARGET) {
                texture = textures.playerSkins[game.settings.currentSkin][1];
            }            // Handle special case for wall tiles to use random wall textures
            else if (tile == WALL) {
                // Create a deterministic random index based on wall position
                // This ensures walls don't change texture every frame but are
                // still randomly distributed based on position
                int seed = (y * level.width + x) * 12345; // Hash based on position
                int randomIndex = seed % WALL_TEXTURE_COUNT;
                texture = textures.wallTextures[randomIndex];
            }
            else {
                texture = textures.tileTextures[tile];
            }
            
            // Draw floor under everything except walls
            if (tile != WALL && tile != EMPTY) {
                SDL_RenderCopy(renderer, textures.tileTextures[EMPTY], nullptr, &destRect);
            }
            
            // Render the tile
            if (texture) {
                SDL_RenderCopy(renderer, texture, nullptr, &destRect);
            }
        }
    }
}

// Load audio files
bool MusicManager::loadAudio() {
    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }
    
    // Load BGM
    bgm = Mix_LoadMUS("assets/sounds/bgm/background.mp3");
    if (!bgm) {
        std::cerr << "Failed to load background music! SDL_mixer Error: " << Mix_GetError() << std::endl;
        // Continue without music
    }
    
    // Load sound effects
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

// Play BGM
void MusicManager::playBGM() const {
    if (bgm && game.settings.bgmEnabled && Mix_PlayingMusic() == 0) {
        Mix_PlayMusic(bgm, -1); // -1 means loop indefinitely
    }
}

// Stop BGM
void MusicManager::stopBGM() const {
    if (Mix_PlayingMusic()) {
        Mix_HaltMusic();
    }
}

// Play movement sound
void MusicManager::playMove() const {
    if (sfx[0] && game.settings.sfxEnabled) {
        Mix_PlayChannel(-1, sfx[0], 0);
    }
}

// Play push sound
void MusicManager::playPush() const {
    if (sfx[1] && game.settings.sfxEnabled) {
        Mix_PlayChannel(-1, sfx[1], 0);
    }
}

// Play level complete sound
void MusicManager::playComplete() const {
    if (sfx[2] && game.settings.sfxEnabled) {
        Mix_PlayChannel(-1, sfx[2], 0);
    }
}