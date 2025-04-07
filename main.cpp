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

// Game states
enum GameState {
    MAIN_MENU,
    LEVEL_SELECT,
    SETTINGS,
    GAME_PLAYING
};

// Button class with text fallback support
class Button {
private:
    SDL_Rect rect;
    SDL_Texture* texture;
    SDL_Texture* hoverTexture;
    bool isHovered;
    std::function<void()> onClick;
    std::string buttonText;  // Text to display if images unavailable
    bool useTextFallback;   // Flag for using text instead of images

public:
    Button(SDL_Renderer* renderer, int x, int y, int w, int h, 
           const std::string& imagePath, const std::string& hoverImagePath, 
           std::function<void()> callback, const std::string& text = "") {
        rect = {x, y, w, h};
        isHovered = false;
        onClick = callback;
        buttonText = text;
        useTextFallback = false;
        
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
        
        // Load normal texture
        SDL_Surface* surface = IMG_Load(imagePath.c_str());
        if (surface == nullptr) {
            std::cout << "Failed to load image: " << imagePath << " - " << IMG_GetError() << std::endl;
            texture = nullptr;
            useTextFallback = true;
        } else {
            texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
        }
        
        // Load hover texture
        surface = IMG_Load(hoverImagePath.c_str());
        if (surface == nullptr) {
            std::cout << "Failed to load hover image: " << hoverImagePath << std::endl;
            hoverTexture = texture; // Use normal texture as fallback
            useTextFallback = useTextFallback || (hoverTexture == nullptr);
        } else {
            hoverTexture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
        }
    }
    
    ~Button() {
        if (texture) SDL_DestroyTexture(texture);
        if (hoverTexture && hoverTexture != texture) SDL_DestroyTexture(hoverTexture);
    }
    
    bool handleEvent(const SDL_Event& event) {
        if (event.type == SDL_MOUSEMOTION) {
            int mouseX = event.motion.x;
            int mouseY = event.motion.y;
            isHovered = (mouseX >= rect.x && mouseX < rect.x + rect.w &&
                         mouseY >= rect.y && mouseY < rect.y + rect.h);
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
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
            SDL_RenderCopy(renderer, isHovered ? hoverTexture : texture, nullptr, &rect);
        } else {
            // Draw a colored rectangle with text as fallback
            if (isHovered) {
                SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255);  // Light blue when hovered
            } else {
                SDL_SetRenderDrawColor(renderer, 70, 70, 200, 255);    // Darker blue normally
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
            }
        }
    }
    
    // Implement text rendering with SDL_TTF
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
        // Continue anyway, we have a fallback display method
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
        levelSelectButtons.push_back(Button(renderer, 300 + (i%3)*250, 200 + (i/3)*150, 200, 100, 
                                          "assets/level" + std::to_string(i+1) + "_btn.png", 
                                          "assets/level" + std::to_string(i+1) + "_btn_hover.png", 
                                          [i]() { std::cout << "Level " << i+1 << " selected\n"; }));
    }
    
    // Back button for level select
    levelSelectButtons.push_back(Button(renderer, 50, 50, 100, 50, "assets/back_btn.png", 
                                      "assets/back_btn_hover.png", 
                                      [&currentState]() { currentState = MAIN_MENU; }));
    
    // Create settings buttons
    std::vector<Button> settingsButtons;
    // Volume up/down buttons
    settingsButtons.push_back(Button(renderer, 700, 200, 50, 50, "assets/vol_up.png", 
                                   "assets/vol_up_hover.png", 
                                   [&musicVolume]() { 
                                       musicVolume = std::min(musicVolume + 10, MIX_MAX_VOLUME);
                                       Mix_VolumeMusic(musicVolume);
                                   }));
    
    settingsButtons.push_back(Button(renderer, 400, 200, 50, 50, "assets/vol_down.png", 
                                   "assets/vol_down_hover.png", 
                                   [&musicVolume]() { 
                                       musicVolume = std::max(musicVolume - 10, 0);
                                       Mix_VolumeMusic(musicVolume);
                                   }));
    
    // Language toggle
    settingsButtons.push_back(Button(renderer, 530, 350, 220, 60, "assets/lang_btn.png", 
                                   "assets/lang_btn_hover.png", 
                                   [&currentLanguage]() { 
                                       if (currentLanguage == "English") currentLanguage = "Vietnamese";
                                       else currentLanguage = "English";
                                   }));
    
    // Back button for settings
    settingsButtons.push_back(Button(renderer, 50, 50, 100, 50, "assets/back_btn.png", 
                                   "assets/back_btn_hover.png", 
                                   [&currentState]() { currentState = MAIN_MENU; }));

    // Biến kiểm tra vòng lặp chính
    bool running = true;

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
                    
                default:
                    break;
            }
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
