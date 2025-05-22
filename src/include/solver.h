#pragma once

#include <vector>
#include <string>
#include <chrono>
#include "game_structures.h"
#include "advanced_solver.h"

extern int solverNodesExplored;
extern int solverMaxQueueSize;
extern int solverExecutionTimeMs;

std::vector<char> solveSokoban(Level& level, int playerX, int playerY, int& nodesExplored, int& maxQueueSize);

bool solveLevel(Level& level, std::vector<char>& solution, int& nodesExplored, int& maxQueueSize);
