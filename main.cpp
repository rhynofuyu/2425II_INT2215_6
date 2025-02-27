#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <cmath>

enum GameState {
    MENU,
    PLAYING
};

struct GameObject {
    SDL_Rect rect;
    int speed_y;
    float speed_x, speed_y_ball;
};

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    Mix_Init(MIX_INIT_MP3);

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        SDL_Log("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        return false;
    }

    SDL_Window* window = SDL_CreateWindow("Game Menu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Surface* bgSurface = IMG_Load("./3.jpg");
    if (!bgSurface) {
        SDL_Log("failed menu bg %s", IMG_GetError());
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
    buttonRect.y = (720 - buttonRect.h) / 2 + 100;
    SDL_FreeSurface(buttonSurface);

    SDL_Surface* paddleSurface = IMG_Load("./dpq.png");
    if (!paddleSurface) {
        SDL_Log("fail paddle texture %s", IMG_GetError());
        return 1;
    }
    SDL_Texture* paddleTexture = SDL_CreateTextureFromSurface(renderer, paddleSurface);
    SDL_FreeSurface(paddleSurface);

    SDL_Surface* gameBgSurface = IMG_Load("./topview.jpeg");
    if (!gameBgSurface) {
        SDL_Log("failed game bg %s", IMG_GetError());
        return 1;
    }
    SDL_Texture* gameBgTexture = SDL_CreateTextureFromSurface(renderer, gameBgSurface);
    SDL_FreeSurface(gameBgSurface);

    Mix_Music *music = Mix_LoadMUS("./bgm.mp3");
    if (!music) {
        SDL_Log("Mix_LoadMUS(\"bgm.mp3\"): %s\n", Mix_GetError());

    }


    GameState state = MENU;
    bool running = true;
    GameObject playerPaddle, enemyPaddle, ball;

    playerPaddle.rect.w = 20;
    playerPaddle.rect.h = 100;
    playerPaddle.rect.x = 50;
    playerPaddle.rect.y = (720 - playerPaddle.rect.h) / 2;
    playerPaddle.speed_y = 0;

    
    enemyPaddle.rect.w = 90;
    enemyPaddle.rect.h = 90; 
    enemyPaddle.rect.x = 1280 - 50 - enemyPaddle.rect.w;
    enemyPaddle.rect.y = (720 - enemyPaddle.rect.h) / 2;
    enemyPaddle.speed_y = 5.0f;

    ball.rect.w = 20;
    ball.rect.h = 20;
    ball.rect.x = (1280 - ball.rect.w) / 2;
    ball.rect.y = (720 - ball.rect.h) / 2;
    ball.speed_x = 5.0f;
    ball.speed_y_ball = 5.0f;


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
                    if (music) {
                        Mix_PlayMusic(music, -1);
                    }
                }
            }
            if (state == PLAYING) {
                if (event.type == SDL_KEYDOWN) {
                    switch (event.key.keysym.sym) {
                        case SDLK_UP: playerPaddle.speed_y = -5; break;
                        case SDLK_DOWN: playerPaddle.speed_y = 5; break;
                    }
                }
                if (event.type == SDL_KEYUP) {
                    switch (event.key.keysym.sym) {
                        case SDLK_UP:
                        case SDLK_DOWN: playerPaddle.speed_y = 0; break;
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (state == MENU) {
            SDL_RenderCopy(renderer, bgTexture, NULL, NULL);
            SDL_RenderCopy(renderer, buttonTexture, NULL, &buttonRect);
        } else if (state == PLAYING) {

            SDL_RenderCopy(renderer, gameBgTexture, NULL, NULL);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

            SDL_RenderFillRect(renderer, &playerPaddle.rect);

            SDL_RenderCopy(renderer, paddleTexture, NULL, &enemyPaddle.rect);

            SDL_RenderFillRect(renderer, &ball.rect);

            playerPaddle.rect.y += playerPaddle.speed_y;
            if (playerPaddle.rect.y < 0) playerPaddle.rect.y = 0;
            if (playerPaddle.rect.y + playerPaddle.rect.h > 720) playerPaddle.rect.y = 720 - playerPaddle.rect.h;

            
            if (ball.rect.y < enemyPaddle.rect.y + enemyPaddle.rect.h / 2) {
                enemyPaddle.speed_y = -3;
            } else if (ball.rect.y > enemyPaddle.rect.y + enemyPaddle.rect.h / 2) {
                enemyPaddle.speed_y = 3;
            } else {
                enemyPaddle.speed_y = 0;
            }
            enemyPaddle.rect.y += enemyPaddle.speed_y;
            if (enemyPaddle.rect.y < 0) enemyPaddle.rect.y = 0;
            if (enemyPaddle.rect.y + enemyPaddle.rect.h > 720) enemyPaddle.rect.y = 720 - enemyPaddle.rect.h;

            ball.rect.x += ball.speed_x;
            ball.rect.y += ball.speed_y_ball;

            if (ball.rect.y < 0 || ball.rect.y + ball.rect.h > 720) {
                ball.speed_y_ball = -ball.speed_y_ball;
            }

            if (SDL_HasIntersection(&ball.rect, &playerPaddle.rect)) {
                ball.speed_x = -ball.speed_x;
            }
            if (SDL_HasIntersection(&ball.rect, &enemyPaddle.rect)) {
                ball.speed_x = -ball.speed_x;
            }

            if (ball.rect.x < 0 || ball.rect.x + ball.rect.w > 1280) {
                ball.rect.x = (1280 - ball.rect.w) / 2;
                ball.rect.y = (720 - ball.rect.h) / 2;
                ball.speed_x = (ball.rect.x < 0) ? 5.0f : -5.0f;
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(bgTexture);
    SDL_DestroyTexture(buttonTexture);
    SDL_DestroyTexture(paddleTexture);
    SDL_DestroyTexture(gameBgTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    if (music) {
        Mix_HaltMusic();
        Mix_FreeMusic(music);
    }
    Mix_CloseAudio();
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}