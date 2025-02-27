#include <SDL2/SDL.h>

struct Paddle {
    SDL_Rect rect;
    int speed;
};

struct Ball {
    SDL_Rect rect;
    int speedX;
    int speedY;
};

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("Không thể khởi tạo SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Ping Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
    if (!window) {
        SDL_Log("Không thể tạo cửa sổ: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Không thể tạo renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Paddle playerPaddle = { {100, 250, 20, 100}, 5 };
    Paddle aiPaddle = { {680, 250, 20, 100}, 5 };
    Ball ball = { {390, 290, 20, 20}, 3, 3 };

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        playerPaddle.rect.y -= playerPaddle.speed;
                        break;
                    case SDLK_DOWN:
                        playerPaddle.rect.y += playerPaddle.speed;
                        break;
                }
            }
        }

        // Cập nhật bóng
        ball.rect.x += ball.speedX;
        ball.rect.y += ball.speedY;

        // Va chạm với paddle
        if (SDL_HasIntersection(&ball.rect, &playerPaddle.rect) || SDL_HasIntersection(&ball.rect, &aiPaddle.rect)) {
            ball.speedX = -ball.speedX;
        }

        // Va chạm với biên
        if (ball.rect.y <= 0 || ball.rect.y + ball.rect.h >= 600) {
            ball.speedY = -ball.speedY;
        }
        if (ball.rect.x <= 0 || ball.rect.x + ball.rect.w >= 800) {
            ball.rect.x = 390;
            ball.rect.y = 290;
        }

        // AI cho paddle
        if (ball.rect.y < aiPaddle.rect.y + aiPaddle.rect.h / 2) {
            aiPaddle.rect.y -= aiPaddle.speed;
        } else if (ball.rect.y > aiPaddle.rect.y + aiPaddle.rect.h / 2) {
            aiPaddle.rect.y += aiPaddle.speed;
        }

        // Vẽ màn hình
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &playerPaddle.rect);
        SDL_RenderFillRect(renderer, &aiPaddle.rect);
        SDL_RenderFillRect(renderer, &ball.rect);
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}