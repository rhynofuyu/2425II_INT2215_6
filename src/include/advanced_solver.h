#pragma once

#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <iostream>
#include <cstring>
#include "game_structures.h"

// Position structure for the solver
struct Position {
    int x, y;
    
    Position(int x = 0, int y = 0) : x(x), y(y) {}
    
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
    
    bool operator!=(const Position& other) const {
        return !(*this == other);
    }
    
    // Add comparison operator for priority queue
    bool operator<(const Position& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

// Hash function for Position
namespace std {
    template<>
    struct hash<Position> {
        size_t operator()(const Position& pos) const {
            return hash<int>()(pos.x) ^ (hash<int>()(pos.y) << 1);
        }
    };
}

// Game state representation for solver
struct SolverState {
    std::vector<std::vector<TileType>> board;
    Position playerPos;
    int g; // Cost so far
    int h; // Heuristic value
    std::string path; // Path taken to reach this state
    
    SolverState() : g(0), h(0) {}
    
    int f() const { return g + h; }
    
    bool operator==(const SolverState& other) const {
        return playerPos == other.playerPos && board == other.board;
    }
};

// Hash function for SolverState
namespace std {
    template<>
    struct hash<SolverState> {
        size_t operator()(const SolverState& state) const {
            size_t boardHash = 0;
            for (const auto& row : state.board) {
                for (TileType tile : row) {
                    boardHash = boardHash * 31 + static_cast<size_t>(tile);
                }
            }
            return boardHash ^ (hash<Position>()(state.playerPos) << 1);
        }
    };
}

// Comparator for priority queue
struct SolverStateComparator {
    bool operator()(const SolverState& a, const SolverState& b) const {
        // Lower f() values have higher priority
        return a.f() > b.f();
    }
};

class AdvancedSolver {
private:
    // Directions: Up, Right, Down, Left
    const int dx[4] = {0, 1, 0, -1};
    const int dy[4] = {-1, 0, 1, 0};
    const char dirChars[4] = {'U', 'R', 'D', 'L'};
    
    // Current game level
    Level level;
    
    // Statistics
    int nodesExplored;
    int maxQueueSize;
    long long executionTimeMs;
    
    // Convert level to solver state
    SolverState levelToState(const Level& level, int playerX, int playerY) {
        SolverState state;
        // Convert TileType** to vector<vector<TileType>>
        state.board.resize(level.height);
        for (int y = 0; y < level.height; y++) {
            state.board[y].resize(level.width);
            for (int x = 0; x < level.width; x++) {
                state.board[y][x] = level.currentMap[y][x];
            }
        }
        state.playerPos = Position(playerX, playerY);
        return state;
    }
    
    // Check if a position is a valid target for movement
    bool isValidPosition(const std::vector<std::vector<TileType>>& board, int x, int y) {
        if (x < 0 || y < 0 || y >= (int)board.size() || x >= (int)board[0].size()) 
            return false;
        return board[y][x] != WALL;
    }
    
    // Simple heuristic for distance to targets
    int calculateHeuristic(const SolverState& state) {
        int h = 0;
        for (size_t y = 0; y < state.board.size(); y++) {
            for (size_t x = 0; x < state.board[y].size(); x++) {
                // Boxes not on targets increase heuristic
                if (state.board[y][x] == BOX) {
                    h += 10;
                }
            }
        }
        return h;
    }
    
    // Check if all boxes are on targets
    bool checkWinCondition(const SolverState& state) {
        for (size_t y = 0; y < state.board.size(); y++) {
            for (size_t x = 0; x < state.board[y].size(); x++) {
                if (state.board[y][x] == BOX) {
                    return false; // Found a box not on target
                }
            }
        }
        return true;
    }
    
public:
    AdvancedSolver() : nodesExplored(0), maxQueueSize(0), executionTimeMs(0) {}
    
    // Solve the level using A* algorithm
    std::string solve(const Level& level, int playerX, int playerY) {
        nodesExplored = 0;
        maxQueueSize = 0;
        Uint32 startTime = SDL_GetTicks();
        
        SolverState initialState = levelToState(level, playerX, playerY);
        initialState.h = calculateHeuristic(initialState);
        
        // Priority queue for A* algorithm
        std::priority_queue<SolverState, std::vector<SolverState>, SolverStateComparator> openSet;
        openSet.push(initialState);
        
        // Track visited states
        std::unordered_set<std::string> closedSet;
        
        // A* search with simplified state hashing
        while (!openSet.empty() && nodesExplored < 100000) { // Limit search to prevent excessive runtime
            SolverState current = openSet.top();
            openSet.pop();
            
            nodesExplored++;
            maxQueueSize = std::max(maxQueueSize, (int)openSet.size());
            
            // Simple state hashing for visited check
            std::string stateHash = current.path;
            
            // Skip if we've seen this state
            if (closedSet.count(stateHash) > 0) {
                continue;
            }
            
            // Check if solved
            if (checkWinCondition(current)) {
                executionTimeMs = SDL_GetTicks() - startTime;
                return current.path;
            }
            
            closedSet.insert(stateHash);
            
            // Try all four directions
            for (int dir = 0; dir < 4; dir++) {
                int nx = current.playerPos.x + dx[dir];
                int ny = current.playerPos.y + dy[dir];
                
                // Check if position is valid
                if (!isValidPosition(current.board, nx, ny)) {
                    continue;
                }
                
                SolverState nextState = current;
                nextState.path += dirChars[dir];
                nextState.g += 1;
                
                // Move player
                if (current.board[ny][nx] == EMPTY || current.board[ny][nx] == TARGET) {
                    // Simple move
                    nextState.playerPos = Position(nx, ny);
                    nextState.h = calculateHeuristic(nextState);
                    openSet.push(nextState);
                }
                else if (current.board[ny][nx] == BOX || current.board[ny][nx] == BOX_ON_TARGET) {
                    // Try to push box
                    int boxNextX = nx + dx[dir];
                    int boxNextY = ny + dy[dir];
                    
                    if (isValidPosition(current.board, boxNextX, boxNextY) && 
                        (current.board[boxNextY][boxNextX] == EMPTY || 
                         current.board[boxNextY][boxNextX] == TARGET)) {
                        
                        // Update board with box pushed
                        nextState.board[boxNextY][boxNextX] = 
                            (current.board[boxNextY][boxNextX] == TARGET) ? BOX_ON_TARGET : BOX;
                        
                        // Update player position
                        nextState.playerPos = Position(nx, ny);
                        
                        // Update tile where player was
                        nextState.board[current.playerPos.y][current.playerPos.x] = EMPTY;
                        
                        // Update tile where box was
                        nextState.board[ny][nx] = 
                            (level.originalMap[ny][nx] == TARGET) ? PLAYER_ON_TARGET : PLAYER;
                        
                        nextState.h = calculateHeuristic(nextState);
                        openSet.push(nextState);
                    }
                }
            }
        }
        
        executionTimeMs = SDL_GetTicks() - startTime;
        return ""; // No solution found
    }
    
    // Get statistics
    int getNodesExplored() const { return nodesExplored; }
    int getMaxQueueSize() const { return maxQueueSize; }
    long long getExecutionTimeMs() const { return executionTimeMs; }
};

// Wrapper function to use the advanced solver
std::vector<char> solveWithAdvancedSolver(Level& level, int playerX, int playerY, int& nodesExplored, int& maxQueueSize);
