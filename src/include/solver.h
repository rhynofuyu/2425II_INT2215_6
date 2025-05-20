#pragma once

#include <vector>
#include <string>
#include <chrono>
#include "game_structures.h"
#include "advanced_solver.h"

// Solver statistics
extern int solverNodesExplored;
extern int solverMaxQueueSize;
extern int solverExecutionTimeMs;

// Main solver function using advanced solver only
std::vector<char> solveSokoban(Level& level, int playerX, int playerY, int& nodesExplored, int& maxQueueSize);

// Process a level for solving
bool solveLevel(Level& level, std::vector<char>& solution, int& nodesExplored, int& maxQueueSize);
