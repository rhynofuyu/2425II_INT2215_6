#include <SDL2/SDL.h>
#include <iostream>
#include <SDL2/SDL_image.h>

const int WIDTH = 1280;
const int HEIGHT = 720;

int main(int argv, char* args[])
{
    SDL_Surface* windowSurface = nullptr;
    SDL_Surface* imageSurface = nullptr;
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        std::cout << "couldnt init sdl error: " << SDL_GetError() << std::endl;
        return 1;
    }

    std::string windowTitle = "setup sdl2 with images";
    SDL_Window* window = SDL_CreateWindow(windowTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI);
    windowSurface = SDL_GetWindowSurface(window);
    if (window == nullptr)
    {
        std::cout << "window could not be initialized: ERROR: " << SDL_GetError() << std::endl;
        return 1;
    }

    if(!(IMG_Init(IMG_INIT_JPG) & IMG_INIT_JPG))
    {
        std::cout << "could not initialize sdl image jpeg Error: " << IMG_GetError() << std::endl;
        return 1;
    }
    
    SDL_Event windowEvent;

    imageSurface = IMG_Load("rsz_1visionboard.jpg");

    std::cout << "image width " << imageSurface->w << " image height " << imageSurface->h;

    while(true)
    {
        if(SDL_PollEvent(&windowEvent))
        {
            if(windowEvent.type == SDL_QUIT)
            {
                break;
            }
        }
        SDL_BlitSurface(imageSurface, nullptr, windowSurface, nullptr);
        SDL_UpdateWindowSurface(window);

    }
    
    SDL_DestroyWindow(window);

    return 0;
}