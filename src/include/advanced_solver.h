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

struct Position {
    int x, y;
    
    Position(int x = 0, int y = 0) : x(x), y(y) {}
    
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
    
    bool operator!=(const Position& other) const {
        return !(*this == other);
    }
    
    bool operator<(const Position& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

namespace std {
    template<>
    struct hash<Position> {
        size_t operator()(const Position& pos) const {
            return hash<int>()(pos.x) ^ (hash<int>()(pos.y) << 1);
        }
    };
}

struct SolverState {
    std::vector<std::vector<TileType>> board;
    Position playerPos;
    std::vector<Position> boxes;
    std::vector<Position> targets;
    int g;
    int h;
    std::string path;
    
    SolverState() : g(0), h(0) {}
    
    int f() const { return g + h; }
    
    bool operator==(const SolverState& other) const {
        if (playerPos != other.playerPos) return false;
        
        std::vector<Position> sortedBoxes = boxes;
        std::vector<Position> otherSortedBoxes = other.boxes;
        std::sort(sortedBoxes.begin(), sortedBoxes.end());
        std::sort(otherSortedBoxes.begin(), otherSortedBoxes.end());
        
        return sortedBoxes == otherSortedBoxes;
    }
};

namespace std {
    template<>
    struct hash<SolverState> {
        size_t operator()(const SolverState& state) const {
            size_t h = hash<Position>()(state.playerPos);
            
            std::vector<Position> sortedBoxes = state.boxes;
            std::sort(sortedBoxes.begin(), sortedBoxes.end());
            
            for (const Position& box : sortedBoxes) {
                h ^= hash<Position>()(box) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
            
            return h;
        }
    };
}

struct SolverStateComparator {
    bool operator()(const SolverState& a, const SolverState& b) const {
        return a.f() > b.f();
    }
};

class AdvancedSolver {
private:
    const int dx[4] = {0, 1, 0, -1};
    const int dy[4] = {-1, 0, 1, 0};
    const char dirChars[4] = {'U', 'R', 'D', 'L'};
    
    Level level;
    
    int nodesExplored;
    int maxQueueSize;
    long long executionTimeMs;
    
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
    
    SolverState levelToState(const Level& level, int playerX, int playerY) {
        SolverState state;
        
        state.board.resize(level.height);
        for (int y = 0; y < level.height; y++) {
            state.board[y].resize(level.width);
            for (int x = 0; x < level.width; x++) {
                state.board[y][x] = level.currentMap[y][x];
            }
        }
        
        state.playerPos = Position(playerX, playerY);
        
        findTargets(level, state.targets);
        
        findBoxes(state.board, state.boxes);
        
        return state;
    }
    
    bool isValidPosition(const std::vector<std::vector<TileType>>& board, int x, int y) {
        if (x < 0 || y < 0 || y >= (int)board.size() || x >= (int)board[0].size()) {
            return false;
        }
        return board[y][x] != WALL;
    }
    
    bool isBoxInCorner(const std::vector<std::vector<TileType>>& board, int x, int y) {
        if (board[y][x] == BOX_ON_TARGET) return false;
        
        bool wallUp = !isValidPosition(board, x, y-1) || board[y-1][x] == WALL;
        bool wallDown = !isValidPosition(board, x, y+1) || board[y+1][x] == WALL;
        bool wallLeft = !isValidPosition(board, x-1, y) || board[y][x-1] == WALL;
        bool wallRight = !isValidPosition(board, x+1, y) || board[y][x+1] == WALL;
        
        return (wallUp && wallLeft) || (wallUp && wallRight) || 
               (wallDown && wallLeft) || (wallDown && wallRight);
    }
    
    bool isBoxStuckAgainstWall(const std::vector<std::vector<TileType>>& board, const std::vector<Position>& targets, int x, int y) {
        if (board[y][x] == BOX_ON_TARGET) return false;
        
        bool wallUp = !isValidPosition(board, x, y-1) || board[y-1][x] == WALL;
        bool wallDown = !isValidPosition(board, x, y+1) || board[y+1][x] == WALL;
        bool wallLeft = !isValidPosition(board, x-1, y) || board[y][x-1] == WALL;
        bool wallRight = !isValidPosition(board, x+1, y) || board[y][x+1] == WALL;
        
        if (wallUp || wallDown) {
            for (const Position& target : targets) {
                if (target.y == y) return false;
            }
            return true;
        }
        
        if (wallLeft || wallRight) {
            for (const Position& target : targets) {
                if (target.x == x) return false;
            }
            return true;
        }
        
        return false;
    }
    
    int calculateHeuristic(const SolverState& state) {
        int h = 0;
        
        if (state.boxes.empty() || state.targets.empty()) {
            return 1000;
        }
        
        for (const Position& box : state.boxes) {
            if (isBoxInCorner(state.board, box.x, box.y) || 
                isBoxStuckAgainstWall(state.board, state.targets, box.x, box.y)) {
                return 1000;
            }
            
            int minDistance = INT_MAX;
            for (const Position& target : state.targets) {
                int distance = abs(box.x - target.x) + abs(box.y - target.y);
                minDistance = std::min(minDistance, distance);
            }
            h += minDistance;
        }
        
        int minPlayerBoxDist = INT_MAX;
        for (const Position& box : state.boxes) {
            if (state.board[box.y][box.x] != BOX_ON_TARGET) {
                int distance = abs(state.playerPos.x - box.x) + abs(state.playerPos.y - box.y);
                minPlayerBoxDist = std::min(minPlayerBoxDist, distance);
            }
        }
        
        if (minPlayerBoxDist < INT_MAX) {
            h += minPlayerBoxDist;
        }
        
        return h;
    }
    
    bool checkWinCondition(const SolverState& state) {
        for (const Position& box : state.boxes) {
            if (state.board[box.y][box.x] != BOX_ON_TARGET) {
                return false;
            }
        }
        return true;
    }
    
    std::string createStateHash(const SolverState& state) {
        std::stringstream ss;
        
        ss << state.playerPos.y << "," << state.playerPos.x << "|";
        
        std::vector<Position> sortedBoxes = state.boxes;
        std::sort(sortedBoxes.begin(), sortedBoxes.end());
        
        for (const Position& box : sortedBoxes) {
            ss << box.y << "," << box.x << ";";
        }
        
        return ss.str();
    }

public:
    AdvancedSolver() : nodesExplored(0), maxQueueSize(0), executionTimeMs(0) {}
    
    std::string solve(const Level& level, int playerX, int playerY) {
        nodesExplored = 0;
        maxQueueSize = 0;
        Uint32 startTime = SDL_GetTicks();
        
        SolverState initialState = levelToState(level, playerX, playerY);
        initialState.h = calculateHeuristic(initialState);
        
        std::priority_queue<SolverState, std::vector<SolverState>, SolverStateComparator> openSet;
        openSet.push(initialState);
        
        std::unordered_set<std::string> closedSet;
        
        int explorationLimit = std::min(1000000, 20000 * level.width * level.height);
        
        while (!openSet.empty() && nodesExplored < explorationLimit) {
            SolverState current = openSet.top();
            openSet.pop();
            
            nodesExplored++;
            maxQueueSize = std::max(maxQueueSize, (int)openSet.size());
            
            std::string stateHash = createStateHash(current);
            
            if (closedSet.count(stateHash) > 0) {
                continue;
            }
            
            if (checkWinCondition(current)) {
                executionTimeMs = SDL_GetTicks() - startTime;
                return current.path;
            }
            
            closedSet.insert(stateHash);
            
            for (int dir = 0; dir < 4; dir++) {
                int nx = current.playerPos.x + dx[dir];
                int ny = current.playerPos.y + dy[dir];
                
                if (!isValidPosition(current.board, nx, ny)) {
                    continue;
                }
                
                SolverState nextState = current;
                nextState.path += dirChars[dir];
                nextState.g += 1;
                
                if (current.board[ny][nx] == EMPTY || current.board[ny][nx] == TARGET) {
                    nextState.playerPos = Position(nx, ny);
                    
                    if (current.board[ny][nx] == TARGET) {
                        nextState.board[ny][nx] = PLAYER_ON_TARGET;
                    } else {
                        nextState.board[ny][nx] = PLAYER;
                    }
                    
                    if (level.originalMap[current.playerPos.y][current.playerPos.x] == TARGET) {
                        nextState.board[current.playerPos.y][current.playerPos.x] = TARGET;
                    } else {
                        nextState.board[current.playerPos.y][current.playerPos.x] = EMPTY;
                    }
                    
                    nextState.h = calculateHeuristic(nextState);
                    if (nextState.h < 1000) {
                        openSet.push(nextState);
                    }
                }
                else if (current.board[ny][nx] == BOX || current.board[ny][nx] == BOX_ON_TARGET) {
                    int boxNextX = nx + dx[dir];
                    int boxNextY = ny + dy[dir];
                    
                    if (isValidPosition(current.board, boxNextX, boxNextY) && 
                        (current.board[boxNextY][boxNextX] == EMPTY || 
                         current.board[boxNextY][boxNextX] == TARGET)) {
                        
                        nextState.board[boxNextY][boxNextX] = 
                            (current.board[boxNextY][boxNextX] == TARGET) ? BOX_ON_TARGET : BOX;
                        
                        nextState.playerPos = Position(nx, ny);
                        
                        if (level.originalMap[current.playerPos.y][current.playerPos.x] == TARGET) {
                            nextState.board[current.playerPos.y][current.playerPos.x] = TARGET;
                        } else {
                            nextState.board[current.playerPos.y][current.playerPos.x] = EMPTY;
                        }
                        
                        if (level.originalMap[ny][nx] == TARGET) {
                            nextState.board[ny][nx] = PLAYER_ON_TARGET;
                        } else {
                            nextState.board[ny][nx] = PLAYER;
                        }
                        
                        nextState.boxes.clear();
                        findBoxes(nextState.board, nextState.boxes);
                        
                        nextState.h = calculateHeuristic(nextState);
                        if (nextState.h < 1000) {
                            openSet.push(nextState);
                        }
                    }
                }
            }
        }
        
        executionTimeMs = SDL_GetTicks() - startTime;
        return "";
    }
    
    int getNodesExplored() const { return nodesExplored; }
    int getMaxQueueSize() const { return maxQueueSize; }
    long long getExecutionTimeMs() const { return executionTimeMs; }
};

std::vector<char> solveWithAdvancedSolver(Level& level, int playerX, int playerY, int& nodesExplored, int& maxQueueSize);
