#include "include/game_init.h"
#include "include/renderer.h"
#include "include/game_resources.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>

void renderGame(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
    SDL_RenderClear(renderer);
    
    TTF_Font* font = TTF_OpenFont("assets/fonts/arial.ttf", 24);
    TTF_Font* largeFont = TTF_OpenFont("assets/fonts/arial.ttf", 48);
    
    if (!font || !largeFont) {
        std::cerr << "Failed to load fonts for rendering" << std::endl;
        return;
    }
    
    if (game.currentState == MENU) {
        renderMenu(renderer, font);
    } else if (game.currentState == PLAYING) {
        if (gameLevelBackgroundTexture) {
            SDL_RenderCopy(renderer, gameLevelBackgroundTexture, nullptr, nullptr);
        }
        
        renderLevel(renderer, game.activeLevel, game.player, gameTextures);
        
        renderHUD(renderer, font, currentLevelIndex + 1, game.player.moves, game.player.pushes);
        
        renderSolverStatus(renderer, font);
    } else if (game.currentState == LEVEL_COMPLETE) {
        if (gameLevelBackgroundTexture) {
            SDL_RenderCopy(renderer, gameLevelBackgroundTexture, nullptr, nullptr);
        }
        
        renderLevel(renderer, game.activeLevel, game.player, gameTextures);
        
        renderLevelComplete(renderer, font, largeFont, currentLevelIndex + 1, game.player.moves, game.player.pushes);
    } else if (game.currentState == GAME_OVER) {
        renderGameComplete(renderer, font, largeFont, game.player.moves, game.player.pushes);
    } else if (game.currentState == LEVEL_SELECT) {
        renderLevelSelect(renderer, font);
    } else if (game.currentState == SETTINGS) {
        renderSettings(renderer, font);
        
        if (showingTutorial) {
            renderTutorial(renderer);
        }
    } else if (game.currentState == SKIN_SELECT) {
        renderSkinSelect(renderer, font);
    }
    
    SDL_RenderPresent(renderer);
    
    TTF_CloseFont(font);
    TTF_CloseFont(largeFont);
}
