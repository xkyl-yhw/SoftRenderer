#include "sdl_window_manager.h"

#define PI 3.14 

SDLWindowManager::SDLWindowManager(int width, int height) : _width(width), _height(height)
{
    _layer = new RendererInitialLayer(width, height);
}

SDLWindowManager::~SDLWindowManager()
{
}

void SDLWindowManager::init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen)
{
    int flags = 0;
    if(fullscreen)
    {
        flags = SDL_WINDOW_FULLSCREEN;
    }

    if(SDL_Init(SDL_INIT_EVERYTHING) == 0)
    {
        std::cout << "SDL initialized." << std::endl;

        window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
        _title = title;
        if(window)
        {
            std::cout << "SDL Window Created." << std::endl;
        }

        renderer = SDL_CreateRenderer(window, -1, 0);
        if(renderer)
        {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            std::cout << "SDL Renderer Created." << std::endl;
        }

        isRunning = true;
    }

    _layer->initial();

}

void SDLWindowManager::handleEvent()
{
    SDL_Event _event;
    SDL_PollEvent(&_event);
    switch (_event.type) {
        case SDL_QUIT:
            isRunning = false;
            break;
        default: break;
    }
}

void SDLWindowManager::update()
{
    _angle += 10;
    if(_angle >= 360) _angle -= 360;
    _model = mat<4, 4>::identity();
    _model[0][0] = std::cos(_angle * PI / 180);
    _model[0][2] = std::sin(_angle * PI / 180);
    _model[2][0] = -std::sin(_angle * PI / 180);
    _model[2][2] = std::cos(_angle * PI / 180);
}

void SDLWindowManager::render()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    _layer->drawScene(renderer);
    SDL_RenderPresent(renderer);
}

void SDLWindowManager::clear()
{
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    std::cout << "SDL Clear." << std::endl;
}

void SDLWindowManager::refreshTitle(int ms)
{
    int fps = 1000 / ms; 
    SDL_SetWindowTitle(window, (_title + " fps-" + std::to_string(fps)).c_str());
}