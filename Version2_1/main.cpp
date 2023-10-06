#include "sdl_window_manager.h"
#include <SDL_timer.h>
#include <SDL_video.h>

SDLWindowManager* _manager = nullptr;
const int width  = 600;
const int height = 600;

int main(int argc, char** argv){
    
    _manager = new SDLWindowManager(width, height);

    _manager->init("SoftRenderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, false);

    while (_manager->running()) 
    {
        auto start = SDL_GetTicks();
        _manager->handleEvent();
        _manager->update();
        _manager->render();
        auto time = SDL_GetTicks() - start;
        _manager->refreshTitle(time);
    }

    _manager->clear();

    return 0;
}
