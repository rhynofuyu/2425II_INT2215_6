#include "include/solver.h"
#include "include/advanced_solver.h"
#include <iostream>

// Global solver statistics
int solverNodesExplored = 0;
int solverMaxQueueSize = 0;
int solverExecutionTimeMs = 0;
// Removed useAdvancedSolver variable since we're only using the advanced solver now

// Implementation of solveWithAdvancedSolver
std::vector<char> solveWithAdvancedSolver(Level& level, int playerX, int playerY, int& nodesExplored, int& maxQueueSize) {
    AdvancedSolver solver;
    std::string solution = solver.solve(level, playerX, playerY);
    
    // Store solver statistics
    nodesExplored = solver.getNodesExplored();
    maxQueueSize = solver.getMaxQueueSize();
    
    // Convert to vector of chars
    std::vector<char> solutionMoves;
    for (char c : solution) {
        solutionMoves.push_back(c);
    }
    
    return solutionMoves;
}

// Direct implementation using advanced solver
std::vector<char> solveSokoban(Level& level, int playerX, int playerY, int& nodesExplored, int& maxQueueSize) {
    return solveWithAdvancedSolver(level, playerX, playerY, nodesExplored, maxQueueSize);
}

// Solve level wrapper function
bool solveLevel(Level& level, std::vector<char>& solution, int& nodesExplored, int& maxQueueSize) {
    solution = solveSokoban(level, level.playerStartX, level.playerStartY, nodesExplored, maxQueueSize);
    return !solution.empty();
}
