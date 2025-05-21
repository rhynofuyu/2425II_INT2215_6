#pragma once

#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <iostream>
#include <cstring>
#include <sstream>
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
    std::vector<Position> boxes;
    std::vector<Position> targets;
    int g; // Cost so far
    int h; // Heuristic value
    std::string path; // Path taken to reach this state
    
    SolverState() : g(0), h(0) {}
    
    int f() const { return g + h; }
    
    bool operator==(const SolverState& other) const {
        if (playerPos != other.playerPos) return false;
        
        // Compare boxes (order doesn't matter, so we need to sort them first)
        std::vector<Position> sortedBoxes = boxes;
        std::vector<Position> otherSortedBoxes = other.boxes;
        std::sort(sortedBoxes.begin(), sortedBoxes.end());
        std::sort(otherSortedBoxes.begin(), otherSortedBoxes.end());
        
        return sortedBoxes == otherSortedBoxes;
    }
};

// Hash function for SolverState
namespace std {
    template<>
    struct hash<SolverState> {
        size_t operator()(const SolverState& state) const {
            // Hash based on player position and box positions
            size_t h = hash<Position>()(state.playerPos);
            
            // Sort boxes to ensure consistent hash regardless of order
            std::vector<Position> sortedBoxes = state.boxes;
            std::sort(sortedBoxes.begin(), sortedBoxes.end());
            
            // Combine hashes of all box positions
            for (const Position& box : sortedBoxes) {
                h ^= hash<Position>()(box) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
            
            return h;
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
    
    // Find all targets in the level
    void findTargets(const Level& level, std::vector<Position>& targets) {
        targets.clear();
        for (int y = 0; y < level.height; y++) {
            for (int x = 0; x < level.width; x++) {
                if (level.originalMap[y][x] == TARGET) {
                    targets.push_back(Position(x, y));
                }
            }
        }
    }
    
    // Find all boxes in the current state
    void findBoxes(const std::vector<std::vector<TileType>>& board, std::vector<Position>& boxes) {
        boxes.clear();
        for (size_t y = 0; y < board.size(); y++) {
            for (size_t x = 0; x < board[y].size(); x++) {
                if (board[y][x] == BOX || board[y][x] == BOX_ON_TARGET) {
                    boxes.push_back(Position(x, y));
                }
            }
        }
    }
    
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
        
        // Find targets
        findTargets(level, state.targets);
        
        // Find boxes
        findBoxes(state.board, state.boxes);
        
        return state;
    }
    
    // Check if a position is a valid target for movement
    bool isValidPosition(const std::vector<std::vector<TileType>>& board, int x, int y) {
        if (x < 0 || y < 0 || y >= (int)board.size() || x >= (int)board[0].size()) {
            return false;
        }
        return board[y][x] != WALL;
    }
    
    // Check if a box is in a corner (can't be moved)
    bool isBoxInCorner(const std::vector<std::vector<TileType>>& board, int x, int y) {
        // If the box is already on a target, it's not a problem
        if (board[y][x] == BOX_ON_TARGET) return false;
        
        // Check if box is in corner (wall on two adjacent sides)
        bool wallUp = !isValidPosition(board, x, y-1) || board[y-1][x] == WALL;
        bool wallDown = !isValidPosition(board, x, y+1) || board[y+1][x] == WALL;
        bool wallLeft = !isValidPosition(board, x-1, y) || board[y][x-1] == WALL;
        bool wallRight = !isValidPosition(board, x+1, y) || board[y][x+1] == WALL;
        
        return (wallUp && wallLeft) || (wallUp && wallRight) || 
               (wallDown && wallLeft) || (wallDown && wallRight);
    }
    
    // Check if a box is against a wall with no target behind it
    bool isBoxStuckAgainstWall(const std::vector<std::vector<TileType>>& board, const std::vector<Position>& targets, int x, int y) {
        // If the box is already on a target, it's not a problem
        if (board[y][x] == BOX_ON_TARGET) return false;
        
        // Check if box is against a wall
        bool wallUp = !isValidPosition(board, x, y-1) || board[y-1][x] == WALL;
        bool wallDown = !isValidPosition(board, x, y+1) || board[y+1][x] == WALL;
        bool wallLeft = !isValidPosition(board, x-1, y) || board[y][x-1] == WALL;
        bool wallRight = !isValidPosition(board, x+1, y) || board[y][x+1] == WALL;
        
        // Check if there's a target in the row/column where the box is stuck
        if (wallUp || wallDown) {
            // Box is on a vertical wall, check if there's a target in this row
            for (const Position& target : targets) {
                if (target.y == y) return false;  // There's a target in this row
            }
            return true; // No target in this row, box is stuck
        }
        
        if (wallLeft || wallRight) {
            // Box is on a horizontal wall, check if there's a target in this column
            for (const Position& target : targets) {
                if (target.x == x) return false;  // There's a target in this column
            }
            return true; // No target in this column, box is stuck
        }
        
        return false; // Box is not against any wall
    }
    
    // Improved heuristic for distance to targets
    int calculateHeuristic(const SolverState& state) {
        int h = 0;
        
        if (state.boxes.empty() || state.targets.empty()) {
            return 1000; // Invalid state
        }
        
        // For each box, find the closest target and add the Manhattan distance
        for (const Position& box : state.boxes) {
            // Check if box is in an unsolvable position
            if (isBoxInCorner(state.board, box.x, box.y) || 
                isBoxStuckAgainstWall(state.board, state.targets, box.x, box.y)) {
                return 1000; // Very high heuristic for unsolvable states
            }
            
            int minDistance = INT_MAX;
            for (const Position& target : state.targets) {
                int distance = abs(box.x - target.x) + abs(box.y - target.y);
                minDistance = std::min(minDistance, distance);
            }
            h += minDistance;
        }
        
        // Add distance from player to nearest box
        int minPlayerBoxDist = INT_MAX;
        for (const Position& box : state.boxes) {
            if (state.board[box.y][box.x] != BOX_ON_TARGET) { // Only consider boxes not on targets
                int distance = abs(state.playerPos.x - box.x) + abs(state.playerPos.y - box.y);
                minPlayerBoxDist = std::min(minPlayerBoxDist, distance);
            }
        }
        
        if (minPlayerBoxDist < INT_MAX) {
            h += minPlayerBoxDist;
        }
        
        return h;
    }
    
    // Check if all boxes are on targets
    bool checkWinCondition(const SolverState& state) {
        for (const Position& box : state.boxes) {
            if (state.board[box.y][box.x] != BOX_ON_TARGET) {
                return false; // Found a box not on target
            }
        }
        return true;
    }
    
    // Create a string representation of the state for visited checking
    std::string createStateHash(const SolverState& state) {
        std::stringstream ss;
        
        // Add player position
        ss << state.playerPos.y << "," << state.playerPos.x << "|";
        
        // Sort boxes to ensure consistency
        std::vector<Position> sortedBoxes = state.boxes;
        std::sort(sortedBoxes.begin(), sortedBoxes.end());
        
        // Add box positions
        for (const Position& box : sortedBoxes) {
            ss << box.y << "," << box.x << ";";
        }
        
        return ss.str();
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
        
        // Track visited states using string hash
        std::unordered_set<std::string> closedSet;
        
        // Increase exploration limit based on board size
        int explorationLimit = std::min(1000000, 20000 * level.width * level.height);
        
        while (!openSet.empty() && nodesExplored < explorationLimit) {
            SolverState current = openSet.top();
            openSet.pop();
            
            nodesExplored++;
            maxQueueSize = std::max(maxQueueSize, (int)openSet.size());
            
            // Create state hash for visited check
            std::string stateHash = createStateHash(current);
            
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
                    
                    // Update board - first the new position
                    if (current.board[ny][nx] == TARGET) {
                        nextState.board[ny][nx] = PLAYER_ON_TARGET;
                    } else {
                        nextState.board[ny][nx] = PLAYER;
                    }
                    
                    // Then the old position
                    if (level.originalMap[current.playerPos.y][current.playerPos.x] == TARGET) {
                        nextState.board[current.playerPos.y][current.playerPos.x] = TARGET;
                    } else {
                        nextState.board[current.playerPos.y][current.playerPos.x] = EMPTY;
                    }
                    
                    nextState.h = calculateHeuristic(nextState);
                    if (nextState.h < 1000) { // Skip states with very high heuristic
                        openSet.push(nextState);
                    }
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
                        if (level.originalMap[current.playerPos.y][current.playerPos.x] == TARGET) {
                            nextState.board[current.playerPos.y][current.playerPos.x] = TARGET;
                        } else {
                            nextState.board[current.playerPos.y][current.playerPos.x] = EMPTY;
                        }
                        
                        // Update tile where player is now
                        if (level.originalMap[ny][nx] == TARGET) {
                            nextState.board[ny][nx] = PLAYER_ON_TARGET;
                        } else {
                            nextState.board[ny][nx] = PLAYER;
                        }
                        
                        // Update box positions
                        nextState.boxes.clear();
                        findBoxes(nextState.board, nextState.boxes);
                        
                        nextState.h = calculateHeuristic(nextState);
                        if (nextState.h < 1000) { // Skip states with very high heuristic
                            openSet.push(nextState);
                        }
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
