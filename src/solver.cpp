#include "include/solver.h"
#include "include/advanced_solver.h"
#include <iostream>
#include <chrono>

int solverNodesExplored = 0;
int solverMaxQueueSize = 0;
int solverExecutionTimeMs = 0;

std::vector<char> solveWithAdvancedSolver(Level& level, int playerX, int playerY, int& nodesExplored, int& maxQueueSize) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    AdvancedSolver solver;
    std::string solution = solver.solve(level, playerX, playerY);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    nodesExplored = solver.getNodesExplored();
    maxQueueSize = solver.getMaxQueueSize();
    solverExecutionTimeMs = duration.count();
    
    std::vector<char> solutionMoves;
    for (char c : solution) {
        solutionMoves.push_back(c);
    }
    
    std::cout << "Solver stats - Nodes explored: " << nodesExplored 
              << ", Max queue size: " << maxQueueSize 
              << ", Time: " << solverExecutionTimeMs << "ms" << std::endl;
    std::cout << "Solution length: " << solutionMoves.size() << std::endl;
    
    return solutionMoves;
}

std::vector<char> solveSokoban(Level& level, int playerX, int playerY, int& nodesExplored, int& maxQueueSize) {
    return solveWithAdvancedSolver(level, playerX, playerY, nodesExplored, maxQueueSize);
}

bool solveLevel(Level& level, std::vector<char>& solution, int& nodesExplored, int& maxQueueSize) {
    solution = solveSokoban(level, level.playerStartX, level.playerStartY, nodesExplored, maxQueueSize);
    return !solution.empty();
}
