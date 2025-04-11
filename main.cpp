#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>  // Add TTF library
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <cmath>
#include <map>  // For mapping button filenames to display text
#include <unistd.h> // For getcwd
#include <random>   // For fish random generation

// Function prototype for loadTexture
SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path);

// Game states
enum GameState {
    MAIN_MENU,
    LEVEL_SELECT,
    SETTINGS,
    GAME_PLAYING
};

// Player direction enum
enum Direction {
    UP, DOWN, LEFT, RIGHT, IDLE
};

// Fishing state enum
enum FishingState {
    NOT_FISHING,
    CASTING,
    WAITING_FOR_FISH,
    FISH_BITING,
    REELING,
    CAUGHT_FISH
};

// Particle class for visual effects (bubbles, ripples)
class Particle {
private:
    float x, y;
    float vx, vy;
    int size;
    int lifetime;
    int maxLifetime;
    SDL_Color color;
    int alpha;
    
public:
    Particle(float startX, float startY, float velocityX, float velocityY, 
             int particleSize, int maxLife, SDL_Color particleColor)
        : x(startX), y(startY), vx(velocityX), vy(velocityY), 
          size(particleSize), lifetime(0), maxLifetime(maxLife), color(particleColor), alpha(255) {
    }
    
    bool update() {
        // Update position
        x += vx;
        y += vy;
        
        // Apply subtle random movement
        x += (std::rand() % 3 - 1) * 0.5f;
        y += (std::rand() % 3 - 1) * 0.5f;
        
        // Update lifetime
        lifetime++;
        
        // Fade out near end of life
        if (lifetime > maxLifetime * 0.7f) {
            float fadeRatio = 1.0f - ((float)lifetime - maxLifetime * 0.7f) / (maxLifetime * 0.3f);
            alpha = static_cast<int>(fadeRatio * 255);
            if (alpha < 0) alpha = 0;
        }
        
        // Return true if particle is still alive
        return lifetime < maxLifetime;
    }
    
    void render(SDL_Renderer* renderer, SDL_Texture* texture = nullptr) {
        if (texture) {
            // If texture is provided, render textured particle
            SDL_Rect destRect = {static_cast<int>(x - size/2), static_cast<int>(y - size/2), size, size};
            SDL_SetTextureAlphaMod(texture, alpha);
            SDL_RenderCopy(renderer, texture, nullptr, &destRect);
        } else {
            // Otherwise render simple circle (approximated with points)
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
            
            // Simple approximation of a circle using points
            for (int i = 0; i < 10; i++) {
                float angle = i * M_PI / 5.0f;
                int px = static_cast<int>(x + std::cos(angle) * size/2);
                int py = static_cast<int>(y + std::sin(angle) * size/2);
                SDL_RenderDrawPoint(renderer, px, py);
            }
        }
    }
};

// ParticleSystem class to manage multiple particles
class ParticleSystem {
private:
    std::vector<Particle> particles;
    SDL_Texture* particleTexture;
    std::mt19937 rng;
    
public:
    ParticleSystem(SDL_Renderer* renderer, const std::string& texturePath = "") : particleTexture(nullptr) {
        // Initialize random generator
        std::random_device rd;
        rng = std::mt19937(rd());
        
        // Load texture if path provided
        if (!texturePath.empty()) {
            particleTexture = loadTexture(renderer, texturePath);
        }
    }
    
    ~ParticleSystem() {
        if (particleTexture) SDL_DestroyTexture(particleTexture);
    }
    
    void addBubbleParticles(float x, float y, int count) {
        std::uniform_real_distribution<float> velDist(-0.5f, 0.5f);
        std::uniform_int_distribution<int> sizeDist(8, 16);
        std::uniform_int_distribution<int> lifeDist(40, 100);
        
        for (int i = 0; i < count; i++) {
            float offsetX = (std::rand() % 20) - 10;
            float offsetY = (std::rand() % 20) - 10;
            
            SDL_Color bubbleColor = {200, 240, 255, 255}; // Light blue
            
            particles.emplace_back(
                x + offsetX, y + offsetY,
                velDist(rng), -0.5f - velDist(rng), // Upward movement
                sizeDist(rng), lifeDist(rng),
                bubbleColor
            );
        }
    }
    
    void addSplashParticles(float x, float y, int count, float force = 1.0f) {
        std::uniform_real_distribution<float> angleDist(0, 2 * M_PI);
        std::uniform_real_distribution<float> speedDist(1.0f, 3.0f);
        std::uniform_int_distribution<int> sizeDist(3, 7);
        std::uniform_int_distribution<int> lifeDist(20, 40);
        
        for (int i = 0; i < count; i++) {
            float angle = angleDist(rng);
            float speed = speedDist(rng) * force;
            
            SDL_Color splashColor = {180, 220, 255, 255}; // Water color
            
            particles.emplace_back(
                x, y,
                std::cos(angle) * speed, std::sin(angle) * speed,
                sizeDist(rng), lifeDist(rng),
                splashColor
            );
        }
    }
    
    void update() {
        // Using erase-remove idiom to efficiently remove dead particles
        particles.erase(std::remove_if(particles.begin(), particles.end(),
            [](Particle& p) { return !p.update(); }), particles.end());
    }
    
    void render(SDL_Renderer* renderer) {
        for (auto& particle : particles) {
            particle.render(renderer, particleTexture);
        }
    }
    
    int getParticleCount() const {
        return particles.size();
    }
};

// Player class for the fishing game
class Player {
private:
    float x, y;
    int width, height;
    float speed;
    Direction direction;
    SDL_Texture* texture;
    SDL_Rect currentFrame;
    SDL_Rect collisionBox;
    bool isMoving;
    
    // Animation variables
    int frameCount;
    int currentFrameIndex;
    int animationDelay;
    int animationTimer;

public:
    Player(SDL_Renderer* renderer, float startX, float startY) {
        x = startX;
        y = startY;
        width = 64;
        height = 64;
        speed = 5.0f;
        direction = IDLE;
        isMoving = false;
        
        // Animation setup
        frameCount = 4;  // Frames per direction
        currentFrameIndex = 0;
        animationDelay = 8;  // Frames to wait before changing animation frame
        animationTimer = 0;
        
        // Default collision box
        collisionBox = {(int)x + 16, (int)y + 32, width - 32, height - 32};
        
        // Load player texture
        texture = loadTexture(renderer, "assets/player_spritesheet.png");
        if (!texture) {
            std::cout << "Failed to load player texture. Using fallback." << std::endl;
        }
        
        // Initialize frame rectangle
        currentFrame = {0, 0, width, height};
    }
    
    ~Player() {
        if (texture) SDL_DestroyTexture(texture);
    }
    
    void handleInput(const Uint8* keystate) {
        // Reset movement flag
        isMoving = false;
        
        // Process movement keys
        if (keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP]) {
            direction = UP;
            y -= speed;
            isMoving = true;
        } 
        else if (keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN]) {
            direction = DOWN;
            y += speed;
            isMoving = true;
        }
        
        if (keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_LEFT]) {
            direction = LEFT;
            x -= speed;
            isMoving = true;
        } 
        else if (keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_RIGHT]) {
            direction = RIGHT;
            x += speed;
            isMoving = true;
        }
        
        // Update collision box based on new position
        collisionBox.x = (int)x + 16;
        collisionBox.y = (int)y + 32;
    }
    
    void update() {
        // Update animation if moving
        if (isMoving) {
            animationTimer++;
            if (animationTimer >= animationDelay) {
                currentFrameIndex = (currentFrameIndex + 1) % frameCount;
                animationTimer = 0;
            }
        } else {
            // Reset to standing frame when not moving
            currentFrameIndex = 0;
        }
        
        // Update current frame rect based on direction and animation
        currentFrame.x = currentFrameIndex * width;
        currentFrame.y = (int)direction * height;
    }
    
    void render(SDL_Renderer* renderer) {
        SDL_Rect destRect = {(int)x, (int)y, width, height};
        
        if (texture) {
            SDL_RenderCopy(renderer, texture, &currentFrame, &destRect);
        } else {
            // Fallback rendering
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &destRect);
            
            // Draw direction indicator
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            int centerX = (int)x + width / 2;
            int centerY = (int)y + height / 2;
            int lineEndX = centerX;
            int lineEndY = centerY;
            
            switch(direction) {
                case UP: lineEndY = centerY - 20; break;
                case DOWN: lineEndY = centerY + 20; break;
                case LEFT: lineEndX = centerX - 20; break;
                case RIGHT: lineEndX = centerX + 20; break;
                default: break;
            }
            
            SDL_RenderDrawLine(renderer, centerX, centerY, lineEndX, lineEndY);
        }
        
        // Debug: draw collision box
        // SDL_SetRenderDrawColor(renderer, 0, 255, 0, 128);
        // SDL_RenderDrawRect(renderer, &collisionBox);
    }
    
    // Getters
    float getX() const { return x; }
    float getY() const { return y; }
    SDL_Rect getCollisionBox() const { return collisionBox; }
    
    // Teleport player to position
    void setPosition(float newX, float newY) {
        x = newX;
        y = newY;
        collisionBox.x = (int)x + 16;
        collisionBox.y = (int)y + 32;
    }
};

// Fishing game class
class FishingGame {
private:
    FishingState state;
    SDL_Texture* rodTexture;
    SDL_Texture* fishTexture;
    SDL_Texture* bubbleTexture;
    int castTimer;
    int waitingTimer;
    int maxWaitTime;
    SDL_Rect waterArea;
    SDL_Rect bobberPosition;
    SDL_Point fishPosition;
    bool fishCaught;
    int fishType;
    int reelProgress;
    int requiredProgress;
    
    std::mt19937 rng;
    ParticleSystem particleSystem;
    
public:
    FishingGame(SDL_Renderer* renderer) : particleSystem(renderer, "assets/fishing/bubble.png") {
        state = NOT_FISHING;
        castTimer = 0;
        waitingTimer = 0;
        maxWaitTime = 0;
        fishCaught = false;
        fishType = 0;
        reelProgress = 0;
        requiredProgress = 100;
        
        // Initialize random generator
        std::random_device rd;
        rng = std::mt19937(rd());
        
        // Define water area (lake position on screen)
        waterArea = {300, 200, 680, 320};
        
        // Load textures
        rodTexture = loadTexture(renderer, "assets/fishing_rod.png");
        fishTexture = loadTexture(renderer, "assets/fishing/fish_types.png");
        bubbleTexture = loadTexture(renderer, "assets/fishing/bubble.png");
        
        if (!rodTexture) {
            std::cout << "Failed to load fishing rod texture" << std::endl;
        }
        if (!fishTexture) {
            std::cout << "Failed to load fish texture" << std::endl;
        }
        if (!bubbleTexture) {
            std::cout << "Failed to load bubble texture" << std::endl;
        }
    }
    
    ~FishingGame() {
        if (rodTexture) SDL_DestroyTexture(rodTexture);
        if (fishTexture) SDL_DestroyTexture(fishTexture);
        if (bubbleTexture) SDL_DestroyTexture(bubbleTexture);
    }
    
    bool isPlayerNearWater(const Player& player) {
        SDL_Rect playerBox = player.getCollisionBox();
        int extendedRange = 20;
        
        SDL_Rect expandedWaterArea = {
            waterArea.x - extendedRange,
            waterArea.y - extendedRange,
            waterArea.w + (2 * extendedRange),
            waterArea.h + (2 * extendedRange)
        };
        
        return SDL_HasIntersection(&playerBox, &expandedWaterArea);
    }
    
    void startFishing(const Player& player) {
        if (state == NOT_FISHING && isPlayerNearWater(player)) {
            state = CASTING;
            castTimer = 0;
            
            // Calculate bobber position based on player's position
            bobberPosition.x = (waterArea.x + waterArea.w / 2) - 16;
            bobberPosition.y = (waterArea.y + waterArea.h / 2) - 16;
            bobberPosition.w = 32;
            bobberPosition.h = 32;
            
            std::cout << "Casting fishing rod..." << std::endl;
        }
    }
    
    void update(const Player& player) {
        switch (state) {
            case CASTING:
                castTimer++;
                if (castTimer >= 60) { // 1 second at 60 FPS
                    state = WAITING_FOR_FISH;
                    waitingTimer = 0;
                    
                    // Random wait time between 1 and 5 seconds
                    std::uniform_int_distribution<int> dist(60, 300);
                    maxWaitTime = dist(rng);
                    
                    std::cout << "Waiting for fish to bite..." << std::endl;
                }
                break;
                
            case WAITING_FOR_FISH:
                waitingTimer++;
                if (waitingTimer >= maxWaitTime) {
                    state = FISH_BITING;
                    
                    // Generate random fish type (0-3)
                    std::uniform_int_distribution<int> fishDist(0, 3);
                    fishType = fishDist(rng);
                    
                    // Set fishing difficulty based on fish type
                    requiredProgress = 60 + (fishType * 20); // 60, 80, 100, 120
                    
                    std::cout << "Fish is biting! Press E to catch!" << std::endl;
                }
                break;
                
            case FISH_BITING:
                // Visual indicator - wait for player input
                particleSystem.addBubbleParticles(bobberPosition.x + 16, bobberPosition.y + 16, 5);
                break;
                
            case REELING:
                // This is handled by key presses
                break;
                
            case CAUGHT_FISH:
                // Display caught fish for 2 seconds
                castTimer++;
                if (castTimer >= 120) {
                    state = NOT_FISHING;
                    fishCaught = false;
                }
                break;
                
            default:
                break;
        }
        
        // Update particle system
        particleSystem.update();
    }
    
    void handleInput(const Uint8* keystate, SDL_Event& event) {
        // Handle specific key presses for fishing
        if (state == FISH_BITING && (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_e)) {
            state = REELING;
            reelProgress = 0;
            std::cout << "Reeling in the fish! Keep pressing E!" << std::endl;
        }
        
        // Reel in fish with repeated key presses
        if (state == REELING && (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_e)) {
            std::uniform_int_distribution<int> progressDist(5, 15);
            reelProgress += progressDist(rng);
            
            if (reelProgress >= requiredProgress) {
                state = CAUGHT_FISH;
                fishCaught = true;
                castTimer = 0;
                std::cout << "You caught a fish (type " << fishType << ")!" << std::endl;
            }
        }
        
        // Cancel fishing with X
        if ((state == CASTING || state == WAITING_FOR_FISH || state == FISH_BITING) && 
            (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_x)) {
            state = NOT_FISHING;
            std::cout << "Fishing canceled" << std::endl;
        }
    }
    
    void render(SDL_Renderer* renderer, const Player& player) {
        // Draw water area
        SDL_SetRenderDrawColor(renderer, 64, 164, 223, 160);
        SDL_RenderFillRect(renderer, &waterArea);
        
        SDL_SetRenderDrawColor(renderer, 32, 128, 192, 255);
        SDL_RenderDrawRect(renderer, &waterArea);
        
        // Render based on fishing state
        switch (state) {
            case CASTING: {
                // Draw casting line
                int playerX = player.getX() + 32;
                int playerY = player.getY() + 32;
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawLine(renderer, playerX, playerY, 
                                  bobberPosition.x + 16, bobberPosition.y + 16);
                
                // Draw bobber
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &bobberPosition);
                break;
            }
            
            case WAITING_FOR_FISH:
            case FISH_BITING: {
                // Draw fishing line
                int playerX = player.getX() + 32;
                int playerY = player.getY() + 32;
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawLine(renderer, playerX, playerY, 
                                  bobberPosition.x + 16, bobberPosition.y + 16);
                
                // Draw bobber
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &bobberPosition);
                
                // Draw bubbles/ripples for FISH_BITING
                if (state == FISH_BITING) {
                    particleSystem.render(renderer);
                }
                break;
            }
            
            case REELING: {
                // Draw fishing line
                int playerX = player.getX() + 32;
                int playerY = player.getY() + 32;
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawLine(renderer, playerX, playerY, 
                                  bobberPosition.x + 16, bobberPosition.y + 16);
                
                // Draw bobber
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &bobberPosition);
                
                // Draw progress bar
                SDL_Rect progressBarBg = {540, 650, 200, 20};
                SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                SDL_RenderFillRect(renderer, &progressBarBg);
                
                SDL_Rect progressBar = {540, 650, (reelProgress * 200) / requiredProgress, 20};
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                SDL_RenderFillRect(renderer, &progressBar);
                
                // Draw instruction text
                // (This would require rendering text - for now, we'll use cout in the console)
                break;
            }
            
            case CAUGHT_FISH: {
                // Display the caught fish
                SDL_Rect fishRect = {540, 360, 64, 64};
                
                if (fishTexture) {
                    SDL_Rect fishSrc = {fishType * 64, 0, 64, 64};
                    SDL_RenderCopy(renderer, fishTexture, &fishSrc, &fishRect);
                } else {
                    // Fallback
                    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
                    SDL_RenderFillRect(renderer, &fishRect);
                }
                
                // Draw text would go here
                break;
            }
            
            default:
                break;
        }
        
        // Render particle system
        particleSystem.render(renderer);
    }
    
    bool canStartFishing(const Player& player) {
        return state == NOT_FISHING && isPlayerNearWater(player);
    }
    
    FishingState getState() const {
        return state;
    }
};

// Button class with hover functionality
class Button {
private:
    SDL_Rect rect;
    SDL_Texture* texture;
    std::function<void()> onClick;
    std::string buttonText;  // Text to display if images unavailable
    bool useTextFallback;   // Flag for using text instead of images
    bool isHovered;         // Track hover state for fallback rendering

public:
    Button(SDL_Renderer* renderer, int x, int y, int w, int h, 
           const std::string& imagePath, const std::string& hoverImagePath, 
           std::function<void()> callback, const std::string& text = "") {
        rect = {x, y, w, h};
        onClick = callback;
        buttonText = text;
        useTextFallback = false;
        isHovered = false;
        
        // Extract button text from filename if not provided
        if (buttonText.empty()) {
            // Extract filename from path
            size_t lastSlash = imagePath.find_last_of("/\\");
            std::string filename = imagePath.substr(lastSlash + 1);
            
            // Remove extension and "_btn" suffix
            size_t dotPos = filename.find_last_of(".");
            std::string baseName = filename.substr(0, dotPos);
            
            // Handle different button types
            if (baseName == "play_btn") buttonText = "PLAY";
            else if (baseName == "settings_btn") buttonText = "SETTINGS";
            else if (baseName == "credit_btn") buttonText = "CREDITS";
            else if (baseName == "back_btn") buttonText = "BACK";
            else if (baseName == "vol_up") buttonText = "+";
            else if (baseName == "vol_down") buttonText = "-";
            else if (baseName == "lang_btn") buttonText = "LANGUAGE";
            else if (baseName.find("level") != std::string::npos) {
                // For level buttons, extract the level number
                for (char c : baseName) {
                    if (isdigit(c)) {
                        buttonText = "LEVEL " + std::string(1, c);
                        break;
                    }
                }
            }
        }
        
        // Check if the assets directory exists
        SDL_RWops* file = SDL_RWFromFile(imagePath.c_str(), "rb");
        if (!file) {
            std::cout << "WARNING: Assets file not found: " << imagePath << std::endl;
            std::cout << "Current working directory may be incorrect." << std::endl;
            useTextFallback = true;
            texture = nullptr;
            return;
        }
        SDL_RWclose(file);
        
        // Load normal texture
        SDL_Surface* surface = IMG_Load(imagePath.c_str());
        if (surface == nullptr) {
            std::cout << "Failed to load image: " << imagePath << " - " << IMG_GetError() << std::endl;
            texture = nullptr;
            useTextFallback = true;
        } else {
            texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (!texture) {
                std::cout << "Failed to create texture from surface: " << SDL_GetError() << std::endl;
                useTextFallback = true;
            }
            SDL_FreeSurface(surface);
        }
    }
    
    ~Button() {
        if (texture) SDL_DestroyTexture(texture);
    }
    
    bool handleEvent(const SDL_Event& event) {
        if (event.type == SDL_MOUSEMOTION) {
            int mouseX = event.motion.x;
            int mouseY = event.motion.y;
            isHovered = (mouseX >= rect.x && mouseX < rect.x + rect.w &&
                         mouseY >= rect.y && mouseY < rect.y + rect.h);
        }
        
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            int mouseX = event.button.x;
            int mouseY = event.button.y;
            if (mouseX >= rect.x && mouseX < rect.x + rect.w &&
                mouseY >= rect.y && mouseY < rect.y + rect.h) {
                onClick();
                return true;
            }
        }
        return false;
    }
    
    void render(SDL_Renderer* renderer, TTF_Font* font = nullptr) {
        if (!useTextFallback && (texture != nullptr)) {
            // Use the image if available
            SDL_RenderCopy(renderer, texture, nullptr, &rect);
        } else {
            // Draw a colored rectangle with text as fallback
            // Use a brighter color when hovered
            if (isHovered) {
                SDL_SetRenderDrawColor(renderer, 100, 100, 220, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 70, 70, 200, 255);
            }
            
            // Draw button background
            SDL_RenderFillRect(renderer, &rect);
            
            // Draw button border
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderDrawRect(renderer, &rect);
            
            // Render text if font is provided
            if (font != nullptr) {
                renderTextOnButton(renderer, font);
            } else {
                // Fallback if no font: draw a simple visual indicator (horizontal line)
                int lineWidth = buttonText.length() * 8;
                int lineX = rect.x + (rect.w - lineWidth) / 2;
                int lineY = rect.y + rect.h / 2;
                
                SDL_Rect textIndicator = {lineX, lineY, lineWidth, 2};
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &textIndicator);
                
                // Also draw the text directly as debug info
                std::cout << "Button at (" << rect.x << "," << rect.y << ") with text: " << buttonText << std::endl;
            }
        }
    }
    
    void renderTextOnButton(SDL_Renderer* renderer, TTF_Font* font) {
        if (!font) return;
        
        SDL_Color textColor = {255, 255, 255, 255}; // White text
        SDL_Surface* textSurface = TTF_RenderText_Blended(font, buttonText.c_str(), textColor);
        if (textSurface) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            
            // Calculate position to center text in button
            SDL_Rect textRect;
            textRect.w = textSurface->w;
            textRect.h = textSurface->h;
            textRect.x = rect.x + (rect.w - textRect.w) / 2;
            textRect.y = rect.y + (rect.h - textRect.h) / 2;
            
            // Render text
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            
            // Clean up
            SDL_FreeSurface(textSurface);
            SDL_DestroyTexture(textTexture);
        }
    }
};

// Load texture function
SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (surface == nullptr) {
        std::cout << "Failed to load image: " << path << " - " << IMG_GetError() << std::endl;
        return nullptr;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

int main(int argc, char* argv[]) {
    // Khởi tạo SDL
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    Mix_Init(MIX_INIT_MP3);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    
    // Initialize TTF
    if (TTF_Init() == -1) {
        std::cout << "Failed to initialize SDL_ttf: " << TTF_GetError() << std::endl;
        return -1;
    }

    // Print current working directory to help diagnose path issues
    char currentPath[FILENAME_MAX];
    #ifdef _WIN32
        _getcwd(currentPath, sizeof(currentPath));
    #else
        getcwd(currentPath, sizeof(currentPath));
    #endif
    std::cout << "Current working directory: " << currentPath << std::endl;
    
    // Check if the assets directory exists
    SDL_RWops* file = SDL_RWFromFile("assets/fonts/arial.ttf", "rb");
    if (!file) {
        std::cout << "WARNING: Font file not found. Make sure the assets directory exists." << std::endl;
        std::cout << "Try running the program from the project's root directory." << std::endl;
    } else {
        SDL_RWclose(file);
    }

    // Tạo cửa sổ
    SDL_Window* window = SDL_CreateWindow(
        "Game Menu",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 720,
        0
    );

    // Tạo renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );
    
    // Load font
    TTF_Font* buttonFont = TTF_OpenFont("assets/fonts/arial.ttf", 24);
    if (!buttonFont) {
        std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
        // Try different font paths if the original path doesn't work
        std::cout << "Trying alternative font path..." << std::endl;
        buttonFont = TTF_OpenFont("./assets/fonts/arial.ttf", 24);
        
        if (!buttonFont) {
            // If Arial is not available, try any system font
            #ifdef _WIN32
            buttonFont = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 24);
            #elif defined(__APPLE__)
            buttonFont = TTF_OpenFont("/Library/Fonts/Arial.ttf", 24);
            #else
            buttonFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24);
            #endif
        }
    }

    // Game state
    GameState currentState = MAIN_MENU;
    
    // Volume settings
    int musicVolume = MIX_MAX_VOLUME;
    int sfxVolume = MIX_MAX_VOLUME;
    std::string currentLanguage = "English";
    
    // Load background textures
    SDL_Texture* mainMenuBg = loadTexture(renderer, "assets/main_menu_bg.png");
    SDL_Texture* levelSelectBg = loadTexture(renderer, "assets/level_select_bg.png"); 
    SDL_Texture* settingsBg = loadTexture(renderer, "assets/settings_bg.png");
    
    // Create main menu buttons
    std::vector<Button> mainMenuButtons;
    mainMenuButtons.push_back(Button(renderer, 500, 300, 280, 80, "assets/play_btn.png", 
                                   "assets/play_btn_hover.png", 
                                   [&currentState]() { currentState = LEVEL_SELECT; }));
    
    mainMenuButtons.push_back(Button(renderer, 500, 400, 280, 80, "assets/settings_btn.png", 
                                   "assets/settings_btn_hover.png", 
                                   [&currentState]() { currentState = SETTINGS; }));
    
    // Add Credits button at the bottom-right corner
    mainMenuButtons.push_back(Button(renderer, 1100, 640, 150, 60, "assets/credit_btn.png", 
                                   "assets/credit_btn_hover.png", 
                                   []() { 
                                       std::cout << "Credits - Game developed by [Your Name]\n"; 
                                       // You could also create a new CREDITS state and screen instead
                                   }));
    
    // Create level select buttons
    std::vector<Button> levelSelectButtons;
    for (int i = 0; i < 5; i++) {
        std::function<void()> callback;
        
        if (i == 0) {
            // Level 1 - Fishing Game
            callback = [&currentState]() { 
                std::cout << "Starting fishing game (Level 1)\n"; 
                currentState = GAME_PLAYING; 
            };
        } else {
            // Other levels - just print message
            callback = [i]() { std::cout << "Level " << i+1 << " selected\n"; };
        }
        
        levelSelectButtons.push_back(Button(renderer, 500 + (i%3)*250, 300 + (i/3)*150, 200, 100, 
                                           "assets/level" + std::to_string(i+1) + "_btn.png", 
                                           "assets/level" + std::to_string(i+1) + "_btn_hover.png", 
                                           callback));
    }
    
    // Back button for level select
    levelSelectButtons.push_back(Button(renderer, 50, 50, 100, 50, "assets/back_btn.png", 
                                      "assets/back_btn_hover.png", 
                                      [&currentState]() { currentState = MAIN_MENU; }));
    
    // Create settings buttons
    std::vector<Button> settingsButtons;
    // Volume up/down buttons
    settingsButtons.push_back(Button(renderer, 700, 300, 50, 50, "assets/vol_up.png", 
                                   "assets/vol_up_hover.png", 
                                   [&musicVolume]() { 
                                       musicVolume = std::min(musicVolume + 10, MIX_MAX_VOLUME);
                                       Mix_VolumeMusic(musicVolume);
                                   }));
    
    settingsButtons.push_back(Button(renderer, 400, 300, 50, 50, "assets/vol_down.png", 
                                   "assets/vol_down_hover.png", 
                                   [&musicVolume]() { 
                                       musicVolume = std::max(musicVolume - 10, 0);
                                       Mix_VolumeMusic(musicVolume);
                                   }));
    
    // Language toggle
    settingsButtons.push_back(Button(renderer, 530, 450, 220, 60, "assets/lang_btn.png", 
                                   "assets/lang_btn_hover.png", 
                                   [&currentLanguage]() { 
                                       if (currentLanguage == "English") currentLanguage = "Vietnamese";
                                       else currentLanguage = "English";
                                   }));
    
    // Back button for settings
    settingsButtons.push_back(Button(renderer, 50, 50, 100, 50, "assets/back_btn.png", 
                                   "assets/back_btn_hover.png", 
                                   [&currentState]() { currentState = MAIN_MENU; }));

    // Create player and fishing game
    Player player(renderer, 640, 500);
    FishingGame fishingGame(renderer);
    
    // Load fishing level background
    SDL_Texture* fishingLevelBg = loadTexture(renderer, "assets/fishing_level_bg.png");

    // Game loop variables
    bool running = true;
    const Uint8* keyboardState = SDL_GetKeyboardState(NULL);
    
    // Vòng lặp chính
    while (running) {
        // Xử lý sự kiện
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            
            // Handle button events based on current state
            switch (currentState) {
                case MAIN_MENU:
                    for (auto& button : mainMenuButtons) {
                        button.handleEvent(event);
                    }
                    break;
                    
                case LEVEL_SELECT:
                    for (auto& button : levelSelectButtons) {
                        button.handleEvent(event);
                    }
                    break;
                    
                case SETTINGS:
                    for (auto& button : settingsButtons) {
                        button.handleEvent(event);
                    }
                    break;
                    
                case GAME_PLAYING:
                    // Check for fishing action key (E)
                    if (event.type == SDL_KEYDOWN) {
                        if (event.key.keysym.sym == SDLK_e && 
                            fishingGame.canStartFishing(player)) {
                            fishingGame.startFishing(player);
                        }
                        // Handle fishing input
                        fishingGame.handleInput(keyboardState, event);
                        
                        // Back to level select on ESC
                        if (event.key.keysym.sym == SDLK_ESCAPE) {
                            currentState = LEVEL_SELECT;
                        }
                    }
                    break;
                    
                default:
                    break;
            }
        }
        
        // Update game state
        if (currentState == GAME_PLAYING) {
            // Update player only if not fishing or just casting
            if (fishingGame.getState() == NOT_FISHING || 
                fishingGame.getState() == CASTING) {
                player.handleInput(keyboardState);
                player.update();
            }
            
            // Update fishing game
            fishingGame.update(player);
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Render based on current state
        switch (currentState) {
            case MAIN_MENU:
                if (mainMenuBg) {
                    SDL_RenderCopy(renderer, mainMenuBg, nullptr, nullptr);
                }
                for (auto& button : mainMenuButtons) {
                    button.render(renderer, buttonFont);
                }
                break;
                
            case LEVEL_SELECT:
                if (levelSelectBg) {
                    SDL_RenderCopy(renderer, levelSelectBg, nullptr, nullptr);
                }
                for (auto& button : levelSelectButtons) {
                    button.render(renderer, buttonFont);
                }
                break;
                
            case SETTINGS:
                if (settingsBg) {
                    SDL_RenderCopy(renderer, settingsBg, nullptr, nullptr);
                }
                for (auto& button : settingsButtons) {
                    button.render(renderer, buttonFont);
                }
                break;
                
            case GAME_PLAYING:
                // Draw level background
                if (fishingLevelBg) {
                    SDL_RenderCopy(renderer, fishingLevelBg, nullptr, nullptr);
                } else {
                    // Fallback background with grass and sky
                    SDL_Rect skyRect = {0, 0, 1280, 400};
                    SDL_Rect grassRect = {0, 400, 1280, 320};
                    
                    SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255); // Sky blue
                    SDL_RenderFillRect(renderer, &skyRect);
                    
                    SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255); // Forest green
                    SDL_RenderFillRect(renderer, &grassRect);
                }
                
                // Render fishing game (water, fish, rod, etc.)
                fishingGame.render(renderer, player);
                
                // Render player character
                player.render(renderer);
                
                // Render UI text for fishing
                if (fishingGame.canStartFishing(player)) {
                    // Display "Press E to fish" text
                    // This would require text rendering
                }
                break;
                
            default:
                break;
        }
        
        // Hiển thị màn hình
        SDL_RenderPresent(renderer);
        
        // Giới hạn FPS
        SDL_Delay(16);
    }

    // Giải phóng bộ nhớ
    if (mainMenuBg) SDL_DestroyTexture(mainMenuBg);
    if (levelSelectBg) SDL_DestroyTexture(levelSelectBg);
    if (settingsBg) SDL_DestroyTexture(settingsBg);
    if (fishingLevelBg) SDL_DestroyTexture(fishingLevelBg);
    
    // Clean up font
    if (buttonFont) TTF_CloseFont(buttonFont);
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    Mix_Quit();
    TTF_Quit();  // Quit TTF
    IMG_Quit();
    SDL_Quit();

    return 0;
}
