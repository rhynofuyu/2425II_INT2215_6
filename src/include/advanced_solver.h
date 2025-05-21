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
    int g; // Cost so far
    int h; // Heuristic value
    std::string path; // Path taken to reach this state
    
    SolverState() : g(0), h(0) {}
    
    int f() const { return g + h; }
    
    bool operator==(const SolverState& other) const {
        return playerPos == other.playerPos && board == other.board;
};  }
};
// Hash function for SolverState
namespace std {n for SolverState
    template<>{
    struct hash<SolverState> {
        size_t operator()(const SolverState& state) const {
            size_t boardHash = 0;olverState& state) const {
            for (const auto& row : state.board) {
                for (TileType tile : row) {ard) {
                    boardHash = boardHash * 31 + static_cast<size_t>(tile);
                }   boardHash = boardHash * 31 + static_cast<size_t>(tile);
            }   }
            return boardHash ^ (hash<Position>()(state.playerPos) << 1);
        }   return boardHash ^ (hash<Position>()(state.playerPos) << 1);
    };  }
}   };
}
// Comparator for priority queue
struct SolverStateComparator {ue
    bool operator()(const SolverState& a, const SolverState& b) const {
        // Lower f() values have higher prioritySolverState& b) const {
        return a.f() > b.f();ave higher priority
    }   return a.f() > b.f();
};  }
};
class AdvancedSolver {
private:vancedSolver {
    // Directions: Up, Right, Down, Left
    const int dx[4] = {0, 1, 0, -1};Left
    const int dy[4] = {-1, 0, 1, 0};
    const char dirChars[4] = {'U', 'R', 'D', 'L'};
    const char dirChars[4] = {'U', 'R', 'D', 'L'};
    // Current game level
    Level level;ame level
    Level level;
    // Statistics
    int nodesExplored;
    int maxQueueSize;;
    long long executionTimeMs;
    long long executionTimeMs;
    // Convert level to solver state
    SolverState levelToState(const Level& level, int playerX, int playerY) {
        SolverState state;te(const Level& level, int playerX, int playerY) {
        // Convert TileType** to vector<vector<TileType>>
        state.board.resize(level.height);ector<TileType>>
        for (int y = 0; y < level.height; y++) {
            state.board[y].resize(level.width);{
            for (int x = 0; x < level.width; x++) {
                state.board[y][x] = level.currentMap[y][x];
            }   state.board[y][x] = level.currentMap[y][x];
        }   }
        state.playerPos = Position(playerX, playerY);
        return state;os = Position(playerX, playerY);
    }   return state;
    }
    // Check if a position is a valid target for movement
    bool isValidPosition(const std::vector<std::vector<TileType>>& board, int x, int y) {
        if (x < 0 || y < 0 || y >= (int)board.size() || x >= (int)board[0].size()) t y) {
            return false;0 || y >= (int)board.size() || x >= (int)board[0].size()) 
        return board[y][x] != WALL;
    }   return board[y][x] != WALL;
    }
    // Improved heuristic for distance to targets
    int calculateHeuristic(const SolverState& state) {
        int h = 0;euristic(const SolverState& state) {
        std::vector<Position> boxes;
        std::vector<Position> targets;
        x++) {
        // Find all boxes and targetse heuristic
        for (size_t y = 0; y < state.board.size(); y++) {ard[y][x] == BOX) {
            for (size_t x = 0; x < state.board[y].size(); x++) {   h += 10;
                if (state.board[y][x] == BOX) {   }
                    boxes.push_back(Position(x, y));   }
                }
                if (state.originalBoard[y][x] == TARGET) {   return h;
                    targets.push_back(Position(x, y));}
                }
            }
        }
        
        // Calculate Manhattan distance from each box to nearest targety].size(); x++) {
        for (const Position& box : boxes) {
            int minDist = INT_MAX;   return false; // Found a box not on target
            for (const Position& target : targets) {   }
                int dist = abs(box.x - target.x) + abs(box.y - target.y);   }
                minDist = std::min(minDist, dist);
            }   return true;
            h += minDist;}
        }
        
        return h;std::string createStateHash(const std::vector<std::vector<TileType>>& board, const Position& playerPos) {
    }
    
    // Check if all boxes are on targetsow : board) {
    bool checkWinCondition(const SolverState& state) { tile : row) {
        for (size_t y = 0; y < state.board.size(); y++) {e);
            }or (size_t x = 0; x < state.board[y].size(); x++) {    }
        }       if (state.board[y][x] == BOX) {
        return ss.str();rn false; // Found a box not on target
    }           }
            }
public: }
    AdvancedSolver() : nodesExplored(0), maxQueueSize(0), executionTimeMs(0) {}
    }
    // Solve the level using A* algorithm
    std::string solve(const Level& level, int playerX, int playerY) {
        nodesExplored = 0;esExplored(0), maxQueueSize(0), executionTimeMs(0) {}nodesExplored = 0;
        maxQueueSize = 0;
        Uint32 startTime = SDL_GetTicks();
        :string solve(const Level& level, int playerX, int playerY) {
        SolverState initialState = levelToState(level, playerX, playerY);
        initialState.h = calculateHeuristic(initialState);nitialState);
        Uint32 startTime = SDL_GetTicks();
        // Priority queue for A* algorithm
        std::priority_queue<SolverState, std::vector<SolverState>, SolverStateComparator> openSet;te, std::vector<SolverState>, SolverStateComparator> openSet;
        openSet.push(initialState);euristic(initialState);
        
        // Track visited states* algorithmrack visited states
        std::unordered_set<std::string> closedSet;or<SolverState>, SolverStateComparator> openSet;edSet;
        openSet.push(initialState);
        // Increase exploration limit based on board size
        int explorationLimit = std::min(500000, 10000 * level.width * level.height); excessive runtime
        while (!openSet.empty() && nodesExplored < explorationLimit) {t;SolverState current = openSet.top();
            SolverState current = openSet.top();
            openSet.pop();* search with simplified state hashing
            y() && nodesExplored < 100000) { // Limit search to prevent excessive runtime
            nodesExplored++;Set.size());
            maxQueueSize = std::max(maxQueueSize, (int)openSet.size());openSet.pop();
            
            // Simple state hashing for visited check
            std::string stateHash = createStateHash(current.board, current.playerPos);maxQueueSize = std::max(maxQueueSize, (int)openSet.size());
            
            // Skip if we've seen this stateheck {
            if (closedSet.count(stateHash) > 0) {tateHash = current.path;
                continue;
            }// Skip if we've seen this state
            t(stateHash) > 0) {
            // Check if solved
            if (checkWinCondition(current)) {
                executionTimeMs = SDL_GetTicks() - startTime;
                return current.path;/ Check if solved
            }if (checkWinCondition(current)) {
            tTicks() - startTime;
            closedSet.insert(stateHash);    return current.path;
            
            // Try all four directions
            for (int dir = 0; dir < 4; dir++) {
                int nx = current.playerPos.x + dx[dir];
                int ny = current.playerPos.y + dy[dir];ry all four directions
                 {
                // Check if position is valid nx, ny)) {
                if (!isValidPosition(current.board, nx, ny)) {ent.playerPos.y + dy[dir];   continue;
                    continue;
                }// Check if position is valid
                rd, nx, ny)) {
                SolverState nextState = current;
                nextState.path += dirChars[dir];
                nextState.g += 1;
                xtState = current;
                // Move player
                if (current.board[ny][nx] == EMPTY || current.board[ny][nx] == TARGET) {mple move
                    // Simple move
                    nextState.playerPos = Position(nx, ny);
                    nextState.h = calculateHeuristic(nextState); EMPTY || current.board[ny][nx] == TARGET) {
                    openSet.push(nextState);   // Simple move
                }_TARGET) {
                else if (current.board[ny][nx] == BOX || current.board[ny][nx] == BOX_ON_TARGET) {ulateHeuristic(nextState);
                    // Try to push boxextX = nx + dx[dir];
                    int boxNextX = nx + dx[dir];
                    int boxNextY = ny + dy[dir]; if (current.board[ny][nx] == BOX || current.board[ny][nx] == BOX_ON_TARGET) {
                     boxNextY) && 
                    if (isValidPosition(current.board, boxNextX, boxNextY) && 
                        (current.board[boxNextY][boxNextX] == EMPTY || 
                         current.board[boxNextY][boxNextX] == TARGET)) {
                        boxNextX, boxNextY) && // Update board with box pushed
                        // Update board with box pushedEMPTY || 
                        nextState.board[boxNextY][boxNextX] = ) ? BOX_ON_TARGET : BOX;
                            (current.board[boxNextY][boxNextX] == TARGET) ? BOX_ON_TARGET : BOX;   
                        pushed       // Update player position
                        // Update player position          nextState.playerPos = Position(nx, ny);
                        nextState.playerPos = Position(nx, ny);    (current.board[boxNextY][boxNextX] == TARGET) ? BOX_ON_TARGET : BOX;               
                        where player was
                        // Update tile where player was
                        nextState.board[current.playerPos.y][current.playerPos.x] = EMPTY;nextState.playerPos = Position(nx, ny);
                         tile where box was
                        // Update tile where box wasr was                    nextState.board[ny][nx] = 
                        nextState.board[ny][nx] =    (level.originalMap[ny][nx] == TARGET) ? PLAYER_ON_TARGET : PLAYER;
                            (level.originalMap[ny][nx] == TARGET) ? PLAYER_ON_TARGET : PLAYER;
                        
                        nextState.h = calculateHeuristic(nextState);= 
                        openSet.push(nextState);       (level.originalMap[ny][nx] == TARGET) ? PLAYER_ON_TARGET : PLAYER;                  }
                    }                       }
                }           nextState.h = calculateHeuristic(nextState);
            }               openSet.push(nextState);
        }            }        
        
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





std::vector<char> solveWithAdvancedSolver(Level& level, int playerX, int playerY, int& nodesExplored, int& maxQueueSize);// Wrapper function to use the advanced solver};    long long getExecutionTimeMs() const { return executionTimeMs; }    int getMaxQueueSize() const { return maxQueueSize; }    int getNodesExplored() const { return nodesExplored; }    // Get statistics        }        return ""; // No solution found        executionTimeMs = SDL_GetTicks() - startTime;                }            }                }        executionTimeMs = SDL_GetTicks() - startTime;
        return ""; // No solution found
    }
    
    // Get statistics
    int getNodesExplored() const { return nodesExplored; }
    int getMaxQueueSize() const { return maxQueueSize; }
    long long getExecutionTimeMs() const { return executionTimeMs; }
};

// Wrapper function to use the advanced solver
std::vector<char> solveWithAdvancedSolver(Level& level, int playerX, int playerY, int& nodesExplored, int& maxQueueSize);
