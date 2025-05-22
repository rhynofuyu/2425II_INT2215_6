#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "game_structures.h"  // Include this first to ensure Player and Level are defined
#include "texture_manager.h"

// Forward declarations if needed
struct Level;
struct Player;

void renderMenu(SDL_Renderer* renderer, TTF_Font* font);
void renderHUD(SDL_Renderer* renderer, TTF_Font* font, int levelNum, int moves, int pushes);
void renderLevelComplete(SDL_Renderer* renderer, TTF_Font* normalFont, TTF_Font* largeFont, int levelNum, int moves, int pushes);
void renderGameComplete(SDL_Renderer* renderer, TTF_Font* normalFont, TTF_Font* largeFont, int moves, int pushes);
void renderLevelSelect(SDL_Renderer* renderer, TTF_Font* font);
void renderSettings(SDL_Renderer* renderer, TTF_Font* font);
void renderSkinSelect(SDL_Renderer* renderer, TTF_Font* font);
void renderTutorial(SDL_Renderer* renderer);
void renderSolverStatus(SDL_Renderer* renderer, TTF_Font* font);
void renderText(SDL_Renderer* renderer, const char* text, int x, int y, TTF_Font* font, SDL_Color textColor);
void renderLevel(SDL_Renderer* renderer, Level& level, Player& player, TextureManager& textures);  // Fixed: use references
void renderGame(SDL_Renderer* renderer);

#endif // RENDERER_H
