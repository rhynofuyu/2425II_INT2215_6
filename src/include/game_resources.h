#ifndef GAME_RESOURCES_H
#define GAME_RESOURCES_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "game_structures.h"

// Background music
extern Mix_Music* backgroundMusic;

// Sound effects
extern Mix_Chunk* soundEffects[3];

// Player skin file paths
extern const char* playerSkinNames[SKIN_COUNT][2];

// Make sure this function is properly declared
bool saveSettings(const char* filename);

#endif // GAME_RESOURCES_H
