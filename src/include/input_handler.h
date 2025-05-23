#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <SDL2/SDL.h>
#include "../include/game_structures.h"

void handleInput(SDL_Event& event);
bool checkWinCondition(Level* level);

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
extern int solverNodesExplored;
extern int solverMaxQueueSize;
extern int solverExecutionTimeMs;
extern int totalLoadedLevels;
extern int currentLevelIndex;
extern std::vector<std::string> dynamicLevelFiles;

#endif
