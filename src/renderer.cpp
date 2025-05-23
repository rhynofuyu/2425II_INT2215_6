#include "include/renderer.h"
#include "include/game_structures.h"
#include "include/texture_manager.h"
#include "include/game_resources.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

const Uint32 SOLUTION_STEP_DELAY = 300;

extern int currentMenuSelection;
extern int currentSettingsSelection;
extern bool showingTutorial;
extern int currentSkinSelection;
extern SDL_Texture* menuBackgroundTexture;
extern SDL_Texture* levelSelectBackgroundTexture;
extern SDL_Texture* gameLevelBackgroundTexture;
extern SDL_Texture* tutorialTexture;
extern int totalLoadedLevels;
extern int currentLevelIndex;
extern bool solverActive;
extern bool solverRunning;
extern bool solverFoundSolution;
extern std::vector<char> solverSolution;
extern size_t currentSolutionStep;
extern Uint32 lastSolutionStepTime;
extern bool showSolverStats;
extern int solverNodesExplored;
extern int solverMaxQueueSize;
extern int solverExecutionTimeMs;
extern GameData game;
extern TextureManager gameTextures;
extern Mix_Chunk* soundEffects[];
extern const char* playerSkinNames[SKIN_COUNT][2];
extern void renderLevel(SDL_Renderer* renderer, Level& level, Player& player, TextureManager& textures);

void renderText(SDL_Renderer* renderer, const char* text, int x, int y, TTF_Font* font, SDL_Color textColor) {
    if (!text || strlen(text) == 0) {
        return;
    }
    
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, textColor);
    if (!textSurface) {
        std::cout << "Unable to render text surface! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return;
    }
    
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        std::cout << "Unable to create texture from rendered text! SDL Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(textSurface);
        return;
    }
    
    SDL_Rect renderQuad = {x, y, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);
    
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

void renderHUD(SDL_Renderer* renderer, TTF_Font* font, int levelNum, int moves, int pushes) {
    int screenWidth = 1280;
    SDL_Color textColor = {200, 200, 200, 255};
    SDL_Color numberColor = {255, 255, 255, 255};
    
    std::string levelText = "Sokoban " + std::to_string(levelNum);
    renderText(renderer, levelText.c_str(), 20, 20, font, textColor);
    
    std::string movesStr = std::to_string(moves);
    std::string movesLabel = " moves";
    
    int pushesWidth = (std::to_string(pushes).length() + 7) * 12;
    int movesTextX = screenWidth - 360 - pushesWidth;
    
    renderText(renderer, movesStr.c_str(), movesTextX, 20, font, numberColor);
    renderText(renderer, movesLabel.c_str(), movesTextX + movesStr.length() * 14, 20, font, textColor);
    
    std::string pushesStr = std::to_string(pushes);
    std::string pushesLabel = " pushes";
    
    renderText(renderer, pushesStr.c_str(), screenWidth - 200, 20, font, numberColor);
    renderText(renderer, pushesLabel.c_str(), screenWidth - 200 + pushesStr.length() * 14, 20, font, textColor);
    
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    for (int x = 10; x < screenWidth - 10; x += 4) {
        SDL_RenderDrawPoint(renderer, x, 45);
    }
}

void renderSolverStatus(SDL_Renderer* renderer, TTF_Font* font) {
    if (!solverActive && !showSolverStats) return;
    
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Color activeColor = {0, 255, 0, 255};
    SDL_Color errorColor = {255, 0, 0, 255};
    SDL_Color infoColor = {135, 206, 250, 255};

    TTF_Font* smallFont = TTF_OpenFont("assets/fonts/arial.ttf", 12);
    if (!smallFont) {
        std::cout << "Failed to load small font for solver stats" << std::endl;
        return;
    }
    
    int lineCount = 1;
    if (solverRunning) lineCount++;
    else if (solverActive) {
        lineCount++;
        if (solverFoundSolution && solverSolution.size() > 0) lineCount++;
    }
    
    if (showSolverStats) {
        if (solverNodesExplored > 0) lineCount++;
        if (solverMaxQueueSize > 0) lineCount++;
        if (solverExecutionTimeMs > 0) lineCount++;
        lineCount++;
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    int panelWidth = 550;
    int panelHeight = lineCount * 13 + 15;
    SDL_Rect solverInfoRect = {10, 50, panelWidth, panelHeight};
    SDL_RenderFillRect(renderer, &solverInfoRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    int yPos = 60;
    std::string solverText;

    renderText(renderer, "Solver", 20, yPos, smallFont, infoColor);
    yPos += 13;

    if (solverRunning) {
        solverText = "Solver is running...";
        renderText(renderer, solverText.c_str(), 20, yPos, smallFont, activeColor);
        yPos += 13;
    } else if (solverActive) {
        if (solverFoundSolution) {
            solverText = "Solution found! " + std::to_string(solverSolution.size()) + " moves";
            renderText(renderer, solverText.c_str(), 20, yPos, smallFont, activeColor);
            yPos += 13;
            
            Uint32 currentTime = SDL_GetTicks();
            if (currentSolutionStep < solverSolution.size() && currentTime - lastSolutionStepTime >= SOLUTION_STEP_DELAY) {
                char move = solverSolution[currentSolutionStep];
                SDL_Event moveEvent;
                moveEvent.type = SDL_KEYDOWN;
                moveEvent.key.keysym.scancode = SDL_SCANCODE_UNKNOWN;
                moveEvent.key.keysym.mod = KMOD_NONE;
                moveEvent.key.repeat = 0;

                switch (move) {
                    case 'U': moveEvent.key.keysym.sym = SDLK_UP; break;
                    case 'D': moveEvent.key.keysym.sym = SDLK_DOWN; break;
                    case 'L': moveEvent.key.keysym.sym = SDLK_LEFT; break;
                    case 'R': moveEvent.key.keysym.sym = SDLK_RIGHT; break;
                }

                SDL_PushEvent(&moveEvent);
                currentSolutionStep++;
                lastSolutionStepTime = currentTime;
            }
            
            if (solverSolution.size() > 0) {
                std::string progressText = "Progress: " + std::to_string(currentSolutionStep) + 
                                        " / " + std::to_string(solverSolution.size());
                renderText(renderer, progressText.c_str(), 20, yPos, smallFont, textColor);
                yPos += 13;
            }
        } else {
            solverText = "Solver failed to find a solution.";
            renderText(renderer, solverText.c_str(), 20, yPos, smallFont, errorColor);
            yPos += 13;
        }
    }

    if (showSolverStats) {
        if (solverNodesExplored > 0) {
            std::string nodesText = "Nodes explored: " + std::to_string(solverNodesExplored);
            renderText(renderer, nodesText.c_str(), 20, yPos, smallFont, textColor);
            yPos += 13;
        }
        
        if (solverMaxQueueSize > 0) {
            std::string queueText = "Max queue size: " + std::to_string(solverMaxQueueSize);
            renderText(renderer, queueText.c_str(), 20, yPos, smallFont, textColor);
            yPos += 13;
        }
        
        if (solverExecutionTimeMs > 0) {
            std::string timeText = "Execution time: " + std::to_string(solverExecutionTimeMs) + " ms";
            renderText(renderer, timeText.c_str(), 20, yPos, smallFont, textColor);
            yPos += 13;
        }
    }
    
    if (showSolverStats) {
        std::string helpText = "F1: Solve  F3: Reset  I: Toggle Info";
        renderText(renderer, helpText.c_str(), 20, yPos, smallFont, infoColor);
    }
    
    TTF_CloseFont(smallFont);
}

void renderMenu(SDL_Renderer* renderer, TTF_Font* font) {
    if (menuBackgroundTexture) {
        SDL_RenderCopy(renderer, menuBackgroundTexture, nullptr, nullptr);
    }

    const char* menuItems[] = {
        "Start Game",
        "Select Level",
        "Select Skin",
        "Settings",
        "Quit"
    };
    
    const int itemCount = 5;
    const int startY = 230;
    const int itemSpacing = 60;
    
    for (int i = 0; i < itemCount; i++) {
        SDL_Color textColor;
        std::string itemText;
        
        if (i == currentMenuSelection) {
            textColor = {0, 200, 0, 255};
            itemText = "> " + std::string(menuItems[i]) + " <";
        } else {
            textColor = {255, 255, 255, 255};
            itemText = menuItems[i];
        }
        
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, itemText.c_str(), textColor);
        int textWidth = textSurface ? textSurface->w : itemText.length() * 15;
        if (textSurface) {
            SDL_FreeSurface(textSurface);
        }
        
        int x = ((1280 - textWidth) / 2) - 275;
        
        renderText(renderer, itemText.c_str(), x, startY + i * itemSpacing, font, textColor);
    }
}

void renderLevelComplete(SDL_Renderer* renderer, TTF_Font* normalFont, TTF_Font* largeFont, int levelNum, int moves, int pushes) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    SDL_Rect overlayRect = {0, 0, 1280, 720};
    SDL_RenderFillRect(renderer, &overlayRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    int screenWidth = 1280;
    int screenHeight = 720;

    SDL_Color goldColor = {255, 215, 0, 255};
    const char* excellentText = "EXCELLENT!";
    SDL_Surface* excellentSurface = TTF_RenderText_Solid(largeFont, excellentText, goldColor);
    SDL_Texture* excellentTexture = SDL_CreateTextureFromSurface(renderer, excellentSurface);
    SDL_Rect excellentRect = {
        (screenWidth - excellentSurface->w) / 2,
        screenHeight / 4 - excellentSurface->h / 2,
        excellentSurface->w,
        excellentSurface->h
    };
    SDL_RenderCopy(renderer, excellentTexture, nullptr, &excellentRect);
    SDL_FreeSurface(excellentSurface);
    SDL_DestroyTexture(excellentTexture);

    SDL_Color whiteColor = {255, 255, 255, 255};
    std::string levelCompleteText = "Level " + std::to_string(levelNum) + " Complete!";
    SDL_Surface* levelCompleteSurface = TTF_RenderText_Solid(normalFont, levelCompleteText.c_str(), whiteColor);
    SDL_Texture* levelCompleteTexture = SDL_CreateTextureFromSurface(renderer, levelCompleteSurface);
    SDL_Rect levelCompleteRect = {
        (screenWidth - levelCompleteSurface->w) / 2,
        screenHeight / 2 - levelCompleteSurface->h / 2,
        levelCompleteSurface->w,
        levelCompleteSurface->h
    };
    SDL_RenderCopy(renderer, levelCompleteTexture, nullptr, &levelCompleteRect);
    SDL_FreeSurface(levelCompleteSurface);
    SDL_DestroyTexture(levelCompleteTexture);

    std::string statsText = "Moves: " + std::to_string(moves) + "  Pushes: " + std::to_string(pushes);
    SDL_Surface* statsSurface = TTF_RenderText_Solid(normalFont, statsText.c_str(), whiteColor);
    SDL_Texture* statsTexture = SDL_CreateTextureFromSurface(renderer, statsSurface);
    SDL_Rect statsRect = {
        (screenWidth - statsSurface->w) / 2,
        screenHeight / 2 + 50,
        statsSurface->w,
        statsSurface->h
    };
    SDL_RenderCopy(renderer, statsTexture, nullptr, &statsRect);
    SDL_FreeSurface(statsSurface);
    SDL_DestroyTexture(statsTexture);

    if (game.isNewRecord) {
        SDL_Color brightGreenColor = {0, 255, 128, 255};
        const char* newRecordText = "NEW HIGH SCORE!";
        SDL_Surface* newRecordSurface = TTF_RenderText_Solid(normalFont, newRecordText, brightGreenColor);
        SDL_Texture* newRecordTexture = SDL_CreateTextureFromSurface(renderer, newRecordSurface);
        SDL_Rect newRecordRect = {
            (screenWidth - newRecordSurface->w) / 2,
            screenHeight / 2 + 100,
            newRecordSurface->w,
            newRecordSurface->h
        };
        SDL_RenderCopy(renderer, newRecordTexture, nullptr, &newRecordRect);
        SDL_FreeSurface(newRecordSurface);
        SDL_DestroyTexture(newRecordTexture);
    }

    SDL_Color yellowColor = {255, 255, 0, 255};
    const char* continueText = "Press SPACE to Continue";
    SDL_Surface* continueSurface = TTF_RenderText_Solid(normalFont, continueText, yellowColor);
    SDL_Texture* continueTexture = SDL_CreateTextureFromSurface(renderer, continueSurface);
    SDL_Rect continueRect = {
        (screenWidth - continueSurface->w) / 2,
        screenHeight - 100,
        continueSurface->w,
        continueSurface->h
    };
    SDL_RenderCopy(renderer, continueTexture, nullptr, &continueRect);
    SDL_FreeSurface(continueSurface);
    SDL_DestroyTexture(continueTexture);

    if (game.settings.sfxEnabled && soundEffects[2]) {
        Mix_PlayChannel(-1, soundEffects[2], 0);
    }
}

void renderGameComplete(SDL_Renderer* renderer, TTF_Font* normalFont, TTF_Font* largeFont, int moves, int pushes) {
    SDL_Surface* doneBackgroundSurface = IMG_Load("assets/images/menu/done_background.png");
    if (doneBackgroundSurface) {
        SDL_Texture* doneBackgroundTexture = SDL_CreateTextureFromSurface(renderer, doneBackgroundSurface);
        SDL_FreeSurface(doneBackgroundSurface);
        
        if (doneBackgroundTexture) {
            SDL_RenderCopy(renderer, doneBackgroundTexture, nullptr, nullptr);
            SDL_DestroyTexture(doneBackgroundTexture);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 64, 255);
            SDL_RenderClear(renderer);
        }
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 0, 64, 255);
        SDL_RenderClear(renderer);
    }
    
    const int screenWidth = 1280;
    
    SDL_Color brightGreenColor = {0, 255, 128, 255};
    
    SDL_Surface* congratsSurface = TTF_RenderText_Solid(largeFont, "CONGRATULATIONS!", brightGreenColor);
    if (congratsSurface) {
        int congratsWidth = congratsSurface->w;
        int congratsX = (screenWidth - congratsWidth) / 2;
        SDL_FreeSurface(congratsSurface);
        
        renderText(renderer, "CONGRATULATIONS!", congratsX, 120, largeFont, brightGreenColor);
    }
    
    TTF_Font* smallerFont = TTF_OpenFont("assets/fonts/arial.ttf", 14);
    if (!smallerFont) {
        smallerFont = normalFont;
    }
    
    SDL_Color lightBlueColor = {135, 206, 250, 255};
    
    SDL_Surface* escSurface = TTF_RenderText_Solid(smallerFont, "Press ESC to return to Menu", lightBlueColor);
    if (escSurface) {
        int escWidth = escSurface->w;
        int escX = (screenWidth - escWidth) / 2;
        SDL_FreeSurface(escSurface);
        
        renderText(renderer, "Press ESC to return to Menu", escX, 190, smallerFont, lightBlueColor);
    }
    
    SDL_Surface* quitSurface = TTF_RenderText_Solid(smallerFont, "Press Q to Quit", lightBlueColor);
    if (quitSurface) {
        int quitWidth = quitSurface->w;
        int quitX = (screenWidth - quitWidth) / 2;
        SDL_FreeSurface(quitSurface);
        
        renderText(renderer, "Press Q to Quit", quitX, 220, smallerFont, lightBlueColor);
    }
    
    if (smallerFont != normalFont) {
        TTF_CloseFont(smallerFont);
    }
}

void renderLevelSelect(SDL_Renderer* renderer, TTF_Font* font) {
    if (levelSelectBackgroundTexture) {
        SDL_RenderCopy(renderer, levelSelectBackgroundTexture, nullptr, nullptr);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 46, 96, 95, 170);
    SDL_Rect backgroundRect = {100, 80, 1080, 560};
    SDL_RenderFillRect(renderer, &backgroundRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Color currentLevelColor = {0, 255, 128, 255};
    SDL_Color completedLevelColor = {135, 206, 250, 255};
    
    const int startY = 180;
    const int itemSpacing = 70;
    const int itemsPerRow = 4;
    const int itemWidth = 240;
    const int levelsPerPage = 16;
    
    TTF_Font* smallFont = TTF_OpenFont("assets/fonts/arial.ttf", 12);
    TTF_Font* scoreFont = TTF_OpenFont("assets/fonts/arial.ttf", 10);
    
    int currentPage = currentLevelIndex / levelsPerPage;
    int startLevel = currentPage * levelsPerPage;
    int endLevel = std::min(startLevel + levelsPerPage, totalLoadedLevels);
    
    std::string instructionsText = "Use arrow keys to navigate and ENTER to select a level";
    SDL_Surface* instrSurface = TTF_RenderText_Solid(smallFont, instructionsText.c_str(), textColor);
    int instrWidth = instrSurface ? instrSurface->w : instructionsText.length() * 7;
    if (instrSurface) {
        SDL_FreeSurface(instrSurface);
    }
    int instrX = (1280 - instrWidth) / 2;
    renderText(renderer, instructionsText.c_str(), instrX, 140, smallFont, textColor);
    
    if (totalLoadedLevels > levelsPerPage) {
        std::string pageText = "Page " + std::to_string(currentPage + 1) + 
                             " of " + std::to_string((totalLoadedLevels + levelsPerPage - 1) / levelsPerPage);
        SDL_Surface* pageSurface = TTF_RenderText_Solid(font, pageText.c_str(), textColor);
        int pageTextWidth = pageSurface ? pageSurface->w : pageText.length() * 14;
        if (pageSurface) {
            SDL_FreeSurface(pageSurface);
        }
        int pageX = (1280 - pageTextWidth) / 2;
        renderText(renderer, pageText.c_str(), pageX, 550, font, textColor);
    }
    
    for (int i = startLevel; i < endLevel; i++) {
        int localIndex = i - startLevel;
        int row = localIndex / itemsPerRow;
        int col = localIndex % itemsPerRow;
        
        int x = 150 + col * itemWidth;
        int y = startY + row * itemSpacing;
        
        if (i == currentLevelIndex) {
            SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 70, 0, 255);
        }
        SDL_Rect levelBox = {x - 10, y - 5, 220, 65};
        SDL_RenderFillRect(renderer, &levelBox);
        
        std::string levelText = "Level " + std::to_string(i + 1);
        
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, levelText.c_str(), textColor);
        int textWidth = textSurface ? textSurface->w : levelText.length() * 15;
        if (textSurface) {
            SDL_FreeSurface(textSurface);
        }
        
        int centeredX = x + (200 - textWidth) / 2;
        
        if (i == currentLevelIndex) {
            renderText(renderer, levelText.c_str(), centeredX, y, font, currentLevelColor);
        } else {
            renderText(renderer, levelText.c_str(), centeredX, y, font, textColor);
        }
        
        std::string scoreText;
        if (i < static_cast<int>(game.highScores.size()) && game.highScores[i].moves < INT_MAX) {
            scoreText = std::to_string(game.highScores[i].moves) + " moves, " + 
                       std::to_string(game.highScores[i].pushes) + " pushes";
        } else {
            scoreText = "Not completed";
        }
        
        SDL_Surface* scoreSurface = TTF_RenderText_Solid(scoreFont, scoreText.c_str(), textColor);
        int scoreWidth = scoreSurface ? scoreSurface->w : scoreText.length() * 5;
        if (scoreSurface) {
            SDL_FreeSurface(scoreSurface);
        }
        int centeredScoreX = x + (200 - scoreWidth) / 2;
        
        SDL_Color scoreColor = (i < static_cast<int>(game.highScores.size()) && game.highScores[i].moves < INT_MAX) ? 
                             completedLevelColor : textColor;
        renderText(renderer, scoreText.c_str(), centeredScoreX, y + 35, scoreFont, scoreColor);
    }
    
    if (totalLoadedLevels > levelsPerPage) {
        SDL_Color pageNavColor = {100, 255, 255, 255};
        std::string pageNavText = "Press PageUp/PageDown to change pages";
        
        SDL_Surface* navSurface = TTF_RenderText_Solid(smallFont, pageNavText.c_str(), pageNavColor);
        int navWidth = navSurface ? navSurface->w : pageNavText.length() * 7;
        if (navSurface) {
            SDL_FreeSurface(navSurface);
        }
        int navX = (1280 - navWidth) / 2;
        
        renderText(renderer, pageNavText.c_str(), navX, 470, smallFont, pageNavColor);
    }
    
    SDL_Color navColor = {255, 160, 0, 255};
    std::string backText = "Press ESC to return to menu";
    
    SDL_Surface* backSurface = TTF_RenderText_Solid(smallFont, backText.c_str(), navColor);
    int backWidth = backSurface ? backSurface->w : backText.length() * 7;
    if (backSurface) {
        SDL_FreeSurface(backSurface);
    }
    int backX = (1280 - backWidth) / 2;
    
    renderText(renderer, backText.c_str(), backX, 515, smallFont, navColor);
    
    if (smallFont) {
        TTF_CloseFont(smallFont);
    }
    if (scoreFont) {
        TTF_CloseFont(scoreFont);
    }
}

void renderSettings(SDL_Renderer* renderer, TTF_Font* font) {
    if (levelSelectBackgroundTexture) {
        SDL_RenderCopy(renderer, levelSelectBackgroundTexture, nullptr, nullptr);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 46, 96, 95, 190);
    SDL_Rect backgroundRect = {320, 120, 640, 480};
    SDL_RenderFillRect(renderer, &backgroundRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    SDL_Color titleColor = {255, 255, 100, 255};
    renderText(renderer, "Game Settings", 520, 140, font, titleColor);
    
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawLine(renderer, 350, 180, 930, 180);
    
    const char* settingsItems[] = {
        "Background Music",
        "Sound Effects",
        "Tutorials",
        "Back to Main Menu"
    };
    
    std::string bgmValue = game.settings.bgmEnabled ? "ON" : "OFF";
    std::string sfxValue = game.settings.sfxEnabled ? "ON" : "OFF";
    
    const std::string settingsValues[] = {
        bgmValue,
        sfxValue,
        "",
        ""
    };
    
    const int startY = 220;
    const int itemSpacing = 60;
    
    for (int i = 0; i < 4; i++) {
        SDL_Color textColor;
        SDL_Color valueColor = {255, 255, 100, 255};
        std::string itemText;
        
        if (i == currentSettingsSelection) {
            textColor = {0, 255, 0, 255};
            itemText = "> " + std::string(settingsItems[i]);
        } else {
            textColor = {255, 255, 255, 255};
            itemText = settingsItems[i];
        }
        
        renderText(renderer, itemText.c_str(), 370, startY + i * itemSpacing, font, textColor);
        
        if (i < 2 && !settingsValues[i].empty()) {
            renderText(renderer, settingsValues[i].c_str(), 825, startY + i * itemSpacing, font, valueColor);
        }
    }
    
    TTF_Font* smallFont = TTF_OpenFont("assets/fonts/arial.ttf", 15);
    
    SDL_Color instructionColor = {150, 220, 255, 255};
    
    std::string navInstructionLine1 = "Use UP/DOWN to navigate";
    std::string navInstructionLine2 = "LEFT/RIGHT to change settings";
    
    SDL_Surface* instrSurface1 = TTF_RenderText_Solid(smallFont, navInstructionLine1.c_str(), instructionColor);
    int instrWidth1 = instrSurface1 ? instrSurface1->w : navInstructionLine1.length() * 10;
    if (instrSurface1) {
        SDL_FreeSurface(instrSurface1);
    }
    
    int instrX1 = 320 + (640 - instrWidth1) / 2;
    
    SDL_Surface* instrSurface2 = TTF_RenderText_Solid(smallFont, navInstructionLine2.c_str(), instructionColor);
    int instrWidth2 = instrSurface2 ? instrSurface2->w : navInstructionLine2.length() * 10;
    if (instrSurface2) {
        SDL_FreeSurface(instrSurface2);
    }
    
    int instrX2 = 320 + (640 - instrWidth2) / 2;
    
    renderText(renderer, navInstructionLine1.c_str(), instrX1, 500, smallFont, instructionColor);
    renderText(renderer, navInstructionLine2.c_str(), instrX2, 530, smallFont, instructionColor);
    
    if (smallFont) {
        TTF_CloseFont(smallFont);
    }
}

void renderSkinSelect(SDL_Renderer* renderer, TTF_Font* font) {
    if (levelSelectBackgroundTexture) {
        SDL_RenderCopy(renderer, levelSelectBackgroundTexture, nullptr, nullptr);
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 46, 96, 95, 190);
    SDL_Rect backgroundRect = {320, 120, 640, 480};
    SDL_RenderFillRect(renderer, &backgroundRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    SDL_Color titleColor = {255, 255, 100, 255};
    
    std::string titleText = "Select Player Skin";
    SDL_Surface* titleSurface = TTF_RenderText_Solid(font, titleText.c_str(), titleColor);
    int titleWidth = titleSurface ? titleSurface->w : titleText.length() * 15;
    if (titleSurface) {
        SDL_FreeSurface(titleSurface);
    }
    
    int titleX = 320 + (640 - titleWidth) / 2;
    renderText(renderer, titleText.c_str(), titleX, 140, font, titleColor);
    
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawLine(renderer, 350, 180, 930, 180);
    
    const char* skinNames[] = {
        "Con meo da den",
        "Bombardino coccodrillo",
        "Capybara",
        "Tralalelo Tralala",
        "Tung tung tung sahur",
        "Kera Candy",
        "Win Sweet",
        "When Event"
    };
    
    int currentSkin = currentSkinSelection;
    
    if (currentSkin >= 8) {
        currentSkin = 0;
    }
    
    SDL_Color normalTextColor = {255, 255, 255, 255};
    SDL_Color arrowColor = {0, 255, 0, 255};
    
    int centerX = 320 + 640 / 2;
    int centerY = 280;
    
    std::string leftArrow = "<";
    renderText(renderer, leftArrow.c_str(), centerX - 150, centerY, font, arrowColor);
    
    std::string rightArrow = ">";
    renderText(renderer, rightArrow.c_str(), centerX + 120, centerY, font, arrowColor);
    
    SDL_Surface* playerSkinSurface = IMG_Load(playerSkinNames[currentSkin][0]);
    if (playerSkinSurface) {
        SDL_Texture* playerSkinTexture = SDL_CreateTextureFromSurface(renderer, playerSkinSurface);
        SDL_FreeSurface(playerSkinSurface);
        
        if (playerSkinTexture) {
            SDL_Rect destRect = {centerX - 40, centerY - 40, 80, 80};
            SDL_RenderCopy(renderer, playerSkinTexture, nullptr, &destRect);
            SDL_DestroyTexture(playerSkinTexture);
        }
    }
    
    std::string skinName = skinNames[currentSkin];
    
    SDL_Surface* nameSurface = TTF_RenderText_Solid(font, skinName.c_str(), normalTextColor);
    int nameWidth = nameSurface ? nameSurface->w : skinName.length() * 15;
    if (nameSurface) {
        SDL_FreeSurface(nameSurface);
    }
    
    int nameX = centerX - nameWidth / 2;
    renderText(renderer, skinName.c_str(), nameX, centerY + 100, font, normalTextColor);
    
    SDL_Color backColor = currentSkinSelection == 8 ? 
                         SDL_Color{0, 255, 0, 255} : SDL_Color{255, 255, 255, 255};
    std::string backText = currentSkinSelection == 8 ? 
                         "> Back to Main Menu <" : "Back to Main Menu";
                         
    SDL_Surface* backSurface = TTF_RenderText_Solid(font, backText.c_str(), backColor);
    int backWidth = backSurface ? backSurface->w : backText.length() * 15;
    if (backSurface) {
        SDL_FreeSurface(backSurface);
    }
    
    int backX = 320 + (640 - backWidth) / 2;
    renderText(renderer, backText.c_str(), backX, 450, font, backColor);
    
    TTF_Font* smallFont = TTF_OpenFont("assets/fonts/arial.ttf", 15);
    
    SDL_Color instructionColor = {150, 220, 255, 255};
    
    std::string navInstructionLine1 = "Use LEFT/RIGHT to navigate";
    std::string navInstructionLine2 = "UP/DOWN for Back option, ENTER to select";
    
    SDL_Surface* instrSurface1 = TTF_RenderText_Solid(smallFont, navInstructionLine1.c_str(), instructionColor);
    int instrWidth1 = instrSurface1 ? instrSurface1->w : navInstructionLine1.length() * 10;
    if (instrSurface1) {
        SDL_FreeSurface(instrSurface1);
    }
    
    int instrX1 = 320 + (640 - instrWidth1) / 2;
    
    SDL_Surface* instrSurface2 = TTF_RenderText_Solid(smallFont, navInstructionLine2.c_str(), instructionColor);
    int instrWidth2 = instrSurface2 ? instrSurface2->w : navInstructionLine2.length() * 10;
    if (instrSurface2) {
        SDL_FreeSurface(instrSurface2);
    }
    
    int instrX2 = 320 + (640 - instrWidth2) / 2;
    
    renderText(renderer, navInstructionLine1.c_str(), instrX1, 500, smallFont, instructionColor);
    renderText(renderer, navInstructionLine2.c_str(), instrX2, 530, smallFont, instructionColor);
    
    if (smallFont) {
        TTF_CloseFont(smallFont);
    }
}

void renderTutorial(SDL_Renderer* renderer) {
    if (!tutorialTexture) {
        return;
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect fullScreenRect = {0, 0, 1280, 720};
    SDL_RenderFillRect(renderer, &fullScreenRect);
    
    int imgWidth, imgHeight;
    SDL_QueryTexture(tutorialTexture, NULL, NULL, &imgWidth, &imgHeight);
    
    int x = (1280 - imgWidth) / 2;
    int y = (720 - imgHeight) / 2;
    
    SDL_Rect destRect = {x, y, imgWidth, imgHeight};
    
    SDL_RenderCopy(renderer, tutorialTexture, NULL, &destRect);
    
    SDL_Color textColor = {255, 255, 255, 255};
    const char* escText = "Press ESC to close tutorial";
    
    TTF_Font* font = TTF_OpenFont("assets/fonts/arial.ttf", 18);
    if (font) {
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, escText, textColor);
        if (textSurface) {
            int textX = (1280 - textSurface->w) / 2;
            int textY = y + imgHeight + 20;
            SDL_FreeSurface(textSurface);
            
            renderText(renderer, escText, textX, textY, font, textColor);
        }
        
        TTF_CloseFont(font);
    }
}

void renderLevel(SDL_Renderer* renderer, Level& level, Player& player, TextureManager& textures) {
}
