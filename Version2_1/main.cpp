#include "sdl_window_manager.h"

SDLWindowManager* _manager = nullptr;
const int width  = 400;
const int height = 400;

int main(int argc, char** argv){
    
    _manager = new SDLWindowManager(width, height);

    _manager->init("SoftRenderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, false);

    while (_manager->running()) 
    {
        _manager->handleEvent();
        _manager->update();
        _manager->render();

    }

    _manager->clear();

    return 0;
}
