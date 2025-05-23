#ifndef GAME_INIT_H
#define GAME_INIT_H

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include "game_structures.h"

bool initSDL();
void cleanupSDL();
bool createWindowAndRenderer(SDL_Window** window, SDL_Renderer** renderer);
void destroyWindowAndRenderer(SDL_Window* window, SDL_Renderer* renderer);

void initGame();
bool initGameResources(SDL_Renderer* renderer);
void cleanupGameResources();
bool initMenuBackground(SDL_Renderer* renderer);
bool initTutorialImage(SDL_Renderer* renderer);
void cleanupMenuResources();
void scanLevelsDirectory(const std::string& path);

void updateGame();
void renderGame(SDL_Renderer* renderer);

extern int totalLoadedLevels;
extern int currentLevelIndex;
extern int solverNodesExplored;
extern int solverMaxQueueSize;
extern int solverExecutionTimeMs;
extern int currentMenuSelection;
extern int currentSettingsSelection;
extern bool showingTutorial;
extern int currentSkinSelection;
extern Uint32 levelCompleteTime;
extern bool showLevelCompleteAnim;

extern bool solverActive;
extern bool solverRunning;
extern bool solverFoundSolution;
extern std::vector<char> solverSolution;
extern size_t currentSolutionStep;
extern Uint32 lastSolutionStepTime;
extern const Uint32 SOLUTION_STEP_DELAY;
extern bool showSolverStats;

extern SDL_Window* window;
extern SDL_Texture* menuBackgroundTexture;
extern SDL_Texture* levelSelectBackgroundTexture;
extern SDL_Texture* gameLevelBackgroundTexture;
extern SDL_Texture* tutorialTexture;
extern std::vector<std::string> dynamicLevelFiles;

#endif
