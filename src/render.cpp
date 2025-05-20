#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>  // Add this for sin function
#include "include/render.h"
#include "include/game_structures.h"
#include "include/game_init.h"
#include "include/input_handler.h"
#include "include/game_logic.h"

// Global constants
const int TILE_SIZE = 40;  // Size of each tile in pixels

// Level complete animation variables
Uint32 levelCompleteTime = 0;
bool showLevelCompleteAnim = false;

// Render menu
void renderMenu(SDL_Renderer* renderer, TTF_Font* font) {
    // First draw the menu background
    if (menuBackgroundTexture) {
        SDL_RenderCopy(renderer, menuBackgroundTexture, nullptr, nullptr);
    }
    
    // Define menu option colors
    SDL_Color selectedColor = {255, 255, 0, 255}; // Yellow for selected item
    SDL_Color normalColor = {255, 255, 255, 255}; // White for unselected items
    
    // Render menu options
    const int menuStartY = 300; // Starting Y position for the first menu item
    const int menuItemSpacing = 50; // Vertical spacing between menu items
    
    // Render title
    renderText(renderer, "SOKOBAN", 640, 100, font, {255, 255, 255, 255});
    
    // Render menu options
    renderText(renderer, "Start Game", 640, menuStartY, font, 
               (currentMenuSelection == MENU_START_GAME) ? selectedColor : normalColor);
    
    // Only show level select option if levels are available
    if (totalLoadedLevels > 0) {
        renderText(renderer, "Select Level", 640, menuStartY + menuItemSpacing, font, 
                   (currentMenuSelection == MENU_SELECT_LEVEL) ? selectedColor : normalColor);
    }
    
    renderText(renderer, "Select Skin", 640, menuStartY + (2 * menuItemSpacing), font, 
               (currentMenuSelection == MENU_SELECT_SKIN) ? selectedColor : normalColor);
    
    renderText(renderer, "Settings", 640, menuStartY + (3 * menuItemSpacing), font, 
               (currentMenuSelection == MENU_SETTINGS) ? selectedColor : normalColor);
    
    renderText(renderer, "Quit", 640, menuStartY + (4 * menuItemSpacing), font, 
               (currentMenuSelection == MENU_QUIT) ? selectedColor : normalColor);
}

// Render settings menu
void renderSettings(SDL_Renderer* renderer, TTF_Font* font) {
    // First draw the menu background
    if (menuBackgroundTexture) {
        SDL_RenderCopy(renderer, menuBackgroundTexture, nullptr, nullptr);
    }
    
    // Define menu option colors
    SDL_Color selectedColor = {255, 255, 0, 255}; // Yellow for selected item
    SDL_Color normalColor = {255, 255, 255, 255}; // White for unselected items
    SDL_Color enabledColor = {0, 255, 0, 255};    // Green for enabled options
    SDL_Color disabledColor = {255, 0, 0, 255};   // Red for disabled options
    
    // Render title
    renderText(renderer, "SETTINGS", 640, 100, font, normalColor);
    
    // Render settings options
    const int menuStartY = 300; // Starting Y position for the first menu item
    const int menuItemSpacing = 50; // Vertical spacing between menu items
    
    // Background Music option
    renderText(renderer, "Background Music:", 500, menuStartY, font, 
               (currentSettingsSelection == SETTINGS_BACKGROUND_MUSIC) ? selectedColor : normalColor);
    renderText(renderer, game.settings.bgmEnabled ? "ON" : "OFF", 800, menuStartY, font,
               game.settings.bgmEnabled ? enabledColor : disabledColor);
    
    // Sound Effects option
    renderText(renderer, "Sound Effects:", 500, menuStartY + menuItemSpacing, font,
               (currentSettingsSelection == SETTINGS_SOUND_EFFECTS) ? selectedColor : normalColor);
    renderText(renderer, game.settings.sfxEnabled ? "ON" : "OFF", 800, menuStartY + menuItemSpacing, font,
               game.settings.sfxEnabled ? enabledColor : disabledColor);
    
    // Back option
    renderText(renderer, "Back", 640, menuStartY + (2 * menuItemSpacing), font,
               (currentSettingsSelection == SETTINGS_BACK) ? selectedColor : normalColor);
    
    // Instructions
    renderText(renderer, "Use UP/DOWN to navigate, ENTER to select, ESC to return", 640, 600, font, normalColor);
}

// Render skin selection screen
void renderSkinSelect(SDL_Renderer* renderer, TTF_Font* font) {
    // First draw the menu background
    if (menuBackgroundTexture) {
        SDL_RenderCopy(renderer, menuBackgroundTexture, nullptr, nullptr);
    }
    
    // Define colors
    SDL_Color normalColor = {255, 255, 255, 255}; // White text
    SDL_Color selectedColor = {255, 255, 0, 255}; // Yellow for selected skin
    
    // Render title
    renderText(renderer, "SELECT PLAYER SKIN", 640, 100, font, normalColor);
    
    // Render current skin name
    std::string skinNames[SKIN_COUNT] = {
        "Default", "Blue", "Red", "Green", "Purple"
    };
    
    renderText(renderer, skinNames[currentSkinSelection].c_str(), 640, 300, font, selectedColor);
    
    // Load and render the current skin preview
    SDL_Surface* skinSurface = IMG_Load(playerSkinNames[currentSkinSelection][0]);
    if (skinSurface) {
        // Create a scaled surface 3x the size of the original
        SDL_Surface* scaledSurface = SDL_CreateRGBSurface(
            0, skinSurface->w * 3, skinSurface->h * 3, 32, 
            0, 0, 0, 0);
        
        SDL_Rect destRect = {0, 0, skinSurface->w * 3, skinSurface->h * 3};
        SDL_BlitScaled(skinSurface, NULL, scaledSurface, &destRect);
        
        SDL_Texture* skinTexture = SDL_CreateTextureFromSurface(renderer, scaledSurface);
        SDL_FreeSurface(skinSurface);
        SDL_FreeSurface(scaledSurface);
        
        if (skinTexture) {
            SDL_Rect skinRect = {
                640 - (skinSurface->w * 3 / 2),  // Center horizontally
                400 - (skinSurface->h * 3 / 2),  // Position vertically
                skinSurface->w * 3,
                skinSurface->h * 3
            };
            SDL_RenderCopy(renderer, skinTexture, nullptr, &skinRect);
            SDL_DestroyTexture(skinTexture);
        }
    }
    
    // Instructions
    renderText(renderer, "Use LEFT/RIGHT to select, ENTER to confirm, ESC to return", 640, 600, font, normalColor);
}

// Render level select screen
void renderLevelSelect(SDL_Renderer* renderer, TTF_Font* font) {
    // First draw the level selection background
    if (levelSelectBackgroundTexture) {
        SDL_RenderCopy(renderer, levelSelectBackgroundTexture, nullptr, nullptr);
    }
    
    // Define colors
    SDL_Color normalColor = {255, 255, 255, 255}; // White text
    SDL_Color selectedColor = {255, 255, 0, 255}; // Yellow for selected level
    SDL_Color completedColor = {0, 255, 0, 255};  // Green for completed levels
    
    // Render title
    renderText(renderer, "SELECT LEVEL", 640, 50, font, normalColor);
    
    // Render the level grid (4 columns)
    const int levelsPerRow = 4;
    const int startX = 320;
    const int startY = 150;
    const int spacingX = 200;
    const int spacingY = 100;
    
    for (int i = 0; i < totalLoadedLevels; i++) {
        int row = i / levelsPerRow;
        int col = i % levelsPerRow;
        int x = startX + (col * spacingX);
        int y = startY + (row * spacingY);
        
        // Prepare level number text
        std::stringstream ss;
        ss << "Level " << (i + 1);
        std::string levelText = ss.str();
        
        // Choose color - selected, completed, or normal
        SDL_Color textColor = normalColor;
        if (i == currentLevelIndex) {
            textColor = selectedColor;        } else if (static_cast<size_t>(i) < game.highScores.size() && game.highScores[i].moves < INT_MAX) {
            // This level has been completed (has a high score)
            textColor = completedColor;
        }
        
        renderText(renderer, levelText.c_str(), x, y, font, textColor);
          // If this level has a high score, show it
        if (static_cast<size_t>(i) < game.highScores.size() && game.highScores[i].moves < INT_MAX) {
            std::stringstream hsText;
            hsText << "Moves: " << game.highScores[i].moves;
            renderText(renderer, hsText.str().c_str(), x, y + 30, font, normalColor);
            
            hsText.str("");
            hsText << "Pushes: " << game.highScores[i].pushes;
            renderText(renderer, hsText.str().c_str(), x, y + 55, font, normalColor);
        }
    }
    
    // Instructions
    renderText(renderer, "Use arrow keys to navigate, ENTER to select, ESC to return", 640, 650, font, normalColor);
}

// Render HUD (heads-up display) showing level info and stats
void renderHUD(SDL_Renderer* renderer, TTF_Font* font, int levelNum, int moves, int pushes) {
    // Background bar for HUD
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 192); // Semi-transparent black
    SDL_Rect hudRect = {0, 0, 1280, 40};
    SDL_RenderFillRect(renderer, &hudRect);
    
    // Text color
    SDL_Color textColor = {255, 255, 255, 255}; // White
    
    // Prepare level text
    std::stringstream levelText;
    levelText << "Level: " << levelNum;
    renderText(renderer, levelText.str().c_str(), 100, 20, font, textColor);
    
    // Prepare moves text
    std::stringstream movesText;
    movesText << "Moves: " << moves;
    renderText(renderer, movesText.str().c_str(), 300, 20, font, textColor);
    
    // Prepare pushes text
    std::stringstream pushesText;
    pushesText << "Pushes: " << pushes;
    renderText(renderer, pushesText.str().c_str(), 500, 20, font, textColor);
    
    // Help text for controls
    std::string helpText = "Arrow keys: Move  Z: Undo  R: Reset  ESC: Menu";
    renderText(renderer, helpText.c_str(), 900, 20, font, textColor);
}

// Render level completion screen
void renderLevelComplete(SDL_Renderer* renderer, TTF_Font* normalFont, TTF_Font* largeFont, 
                        int levelNum, int moves, int pushes) {
    // If this is the first frame of the completion animation, record the time
    if (!showLevelCompleteAnim) {
        levelCompleteTime = SDL_GetTicks();
        showLevelCompleteAnim = true;
    }
    
    // Calculate animation progress
    Uint32 currentTime = SDL_GetTicks();
    Uint32 elapsedTime = currentTime - levelCompleteTime;
    float animProgress = std::min(1.0f, elapsedTime / 2000.0f); // 2 second animation
    
    // Create a semi-transparent overlay that gradually appears
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, static_cast<Uint8>(192 * animProgress));
    SDL_Rect overlayRect = {0, 0, 1280, 720};
    SDL_RenderFillRect(renderer, &overlayRect);
    
    // Text appears after a small delay
    if (animProgress > 0.3f) {
        float textAlpha = std::min(1.0f, (animProgress - 0.3f) / 0.7f);
        SDL_Color textColor = {255, 255, 0, static_cast<Uint8>(255 * textAlpha)}; // Yellow with fade in
        
        // Render level complete message
        std::stringstream titleText;
        titleText << "Level " << levelNum << " Complete!";
        renderText(renderer, titleText.str().c_str(), 640, 200, largeFont, textColor);
        
        // Game stats
        SDL_Color statsColor = {255, 255, 255, static_cast<Uint8>(255 * textAlpha)}; // White with fade in
        
        std::stringstream movesText;
        movesText << "Moves: " << moves;
        renderText(renderer, movesText.str().c_str(), 540, 300, normalFont, statsColor);
        
        std::stringstream pushesText; 
        pushesText << "Pushes: " << pushes;
        renderText(renderer, pushesText.str().c_str(), 740, 300, normalFont, statsColor);
        
        // Display if this is a new record
        if (game.isNewRecord) {
            SDL_Color newRecordColor = {0, 255, 0, static_cast<Uint8>(255 * textAlpha)}; // Green
            renderText(renderer, "NEW RECORD!", 640, 350, normalFont, newRecordColor);
        }
        
        // Display controls to continue
        SDL_Color controlsColor = {200, 200, 200, static_cast<Uint8>(255 * textAlpha)}; // Light gray
        renderText(renderer, "Press ENTER to continue to next level", 640, 500, normalFont, controlsColor);
        renderText(renderer, "Press ESC to return to menu", 640, 540, normalFont, controlsColor);
    }
}

// Render game completion screen
void renderGameComplete(SDL_Renderer* renderer, TTF_Font* normalFont, TTF_Font* largeFont, 
                        int totalMoves, int totalPushes) {
    // Draw a color-shifting background
    Uint32 currentTime = SDL_GetTicks();
    Uint8 r = static_cast<Uint8>(128 + 127 * sin(currentTime / 1000.0));
    Uint8 g = static_cast<Uint8>(128 + 127 * sin(currentTime / 1500.0));
    Uint8 b = static_cast<Uint8>(128 + 127 * sin(currentTime / 2000.0));
    
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_Rect bgRect = {0, 0, 1280, 720};
    SDL_RenderFillRect(renderer, &bgRect);
    
    // Add a dark overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    SDL_RenderFillRect(renderer, &bgRect);
    
    // Text colors
    SDL_Color titleColor = {255, 255, 0, 255}; // Yellow
    SDL_Color statsColor = {255, 255, 255, 255}; // White
    
    // Render completion message
    renderText(renderer, "CONGRATULATIONS!", 640, 150, largeFont, titleColor);
    renderText(renderer, "You've completed all levels!", 640, 220, normalFont, statsColor);
    
    // Display total stats
    std::stringstream movesText;
    movesText << "Total Moves: " << totalMoves;
    renderText(renderer, movesText.str().c_str(), 640, 300, normalFont, statsColor);
    
    std::stringstream pushesText;
    pushesText << "Total Pushes: " << totalPushes;
    renderText(renderer, pushesText.str().c_str(), 640, 340, normalFont, statsColor);
    
    // Display prompt to continue
    SDL_Color promptColor = {200, 200, 200, 255}; // Light gray
    renderText(renderer, "Press ENTER or ESC to return to main menu", 640, 500, normalFont, promptColor);
}

// Render the game level
void renderLevel(SDL_Renderer* renderer, const Level& level, const PlayerInfo& player, const TextureManager& textures) {
    // Calculate the offset to center the level on the screen
    int offsetX = (1280 - (level.width * TILE_SIZE)) / 2;
    int offsetY = (720 - (level.height * TILE_SIZE)) / 2;
    
    // Draw the level tiles
    for (int y = 0; y < level.height; y++) {
        for (int x = 0; x < level.width; x++) {
            // Calculate the destination rectangle for this tile
            SDL_Rect dstRect = {
                offsetX + (x * TILE_SIZE),
                offsetY + (y * TILE_SIZE),
                TILE_SIZE,
                TILE_SIZE
            };
            
            // Draw the floor tile first (always)
            SDL_RenderCopy(renderer, textures.tileTextures[EMPTY], nullptr, &dstRect);
            
            // Then draw the appropriate tile based on the type
            switch (level.currentMap[y][x]) {
                case WALL:
                    // Use the first wall texture or a random wall texture
                    SDL_RenderCopy(renderer, textures.wallTextures[0], nullptr, &dstRect);
                    break;
                case BOX:
                    SDL_RenderCopy(renderer, textures.tileTextures[BOX], nullptr, &dstRect);
                    break;
                case TARGET:
                    SDL_RenderCopy(renderer, textures.tileTextures[TARGET], nullptr, &dstRect);
                    break;
                case PLAYER:
                    // Use the current skin for player
                    SDL_RenderCopy(renderer, textures.playerSkins[game.settings.currentSkin][0], nullptr, &dstRect);
                    break;
                case BOX_ON_TARGET:
                    SDL_RenderCopy(renderer, textures.tileTextures[BOX_ON_TARGET], nullptr, &dstRect);
                    break;
                case PLAYER_ON_TARGET:
                    // Use the current skin for player on target
                    SDL_RenderCopy(renderer, textures.playerSkins[game.settings.currentSkin][1], nullptr, &dstRect);
                    break;
                default:
                    break;
            }
        }
    }
}

// Render text with horizontal centering
void renderText(SDL_Renderer* renderer, const char* text, int x, int y, TTF_Font* font, SDL_Color textColor) {
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text, textColor);
    if (!textSurface) {
        std::cout << "Failed to render text: " << TTF_GetError() << std::endl;
        return;
    }
    
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        std::cout << "Failed to create texture from rendered text: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(textSurface);
        return;
    }
    
    // Setup the destination rectangle to be centered horizontally on the given x
    SDL_Rect dstRect = {
        x - (textSurface->w / 2), // Center horizontally
        y - (textSurface->h / 2), // Center vertically
        textSurface->w,
        textSurface->h
    };
    
    // Render the text
    SDL_RenderCopy(renderer, textTexture, nullptr, &dstRect);
    
    // Clean up
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}
