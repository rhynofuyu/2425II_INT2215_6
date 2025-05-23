#ifndef GAME_RESOURCES_H
#define GAME_RESOURCES_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "game_structures.h"

extern Mix_Music* backgroundMusic;

extern Mix_Chunk* soundEffects[3];

extern const char* playerSkinNames[SKIN_COUNT][2];

bool saveSettings(const char* filename);

#endif // GAME_RESOURCES_H
