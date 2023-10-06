#ifndef __SDL_WINDOW_MANAGER_H__
#define __SDL_WINDOW_MANAGER_H__

#include "SDL2/SDL.h"
#include "renderer_initial_layer.h"
#include <iostream>

class SDLWindowManager
{
public:
    SDLWindowManager(int width, int height);
    ~SDLWindowManager();

    void init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);

    void handleEvent();
    void update();
    void render();
    void clear();

    bool running() { return isRunning; };

    void refreshTitle(int ms);

private:
    bool isRunning = false;
    SDL_Window* window;
    SDL_Renderer* renderer;
    RendererInitialLayer* _layer;
    std::string _title;
    int _width, _height;
    double _angle = 0;
};

#endif