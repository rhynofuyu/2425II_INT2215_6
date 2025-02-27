#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

enum GameState {
    MENU,
    PLAYING
};

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window* window = SDL_CreateWindow("Game Menu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Surface* bgSurface = IMG_Load("./3.jpg"); 
    if (!bgSurface) {
        SDL_Log("failed bg %s", IMG_GetError());
        return 1;
    }
    SDL_Texture* bgTexture = SDL_CreateTextureFromSurface(renderer, bgSurface);
    SDL_FreeSurface(bgSurface);

    SDL_Surface* buttonSurface = IMG_Load("./4.jpg"); 
    if (!buttonSurface) {
        SDL_Log("fail button %s", IMG_GetError());
        return 1;
    }
    SDL_Texture* buttonTexture = SDL_CreateTextureFromSurface(renderer, buttonSurface);

    SDL_Rect buttonRect;
    buttonRect.w = buttonSurface->w;
    buttonRect.h = buttonSurface->h; 
    buttonRect.x = (1280 - buttonRect.w) / 2;
    buttonRect.y = (720 - buttonRect.h) / 2; 
    SDL_FreeSurface(buttonSurface);

    GameState state = MENU;
    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (state == MENU && event.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                if (mouseX >= buttonRect.x && mouseX <= buttonRect.x + buttonRect.w &&
                    mouseY >= buttonRect.y && mouseY <= buttonRect.y + buttonRect.h) {
                    state = PLAYING; 
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (state == MENU) {
            SDL_RenderCopy(renderer, bgTexture, NULL, NULL);
            SDL_RenderCopy(renderer, buttonTexture, NULL, &buttonRect);
        } else if (state == PLAYING) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_Rect placeholder = {350, 250, 100, 100};
            SDL_RenderFillRect(renderer, &placeholder);
        }

        SDL_RenderPresent(renderer); 
        SDL_Delay(16); 
    }

    SDL_DestroyTexture(bgTexture);
    SDL_DestroyTexture(buttonTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}