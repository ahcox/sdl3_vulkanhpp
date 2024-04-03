#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <iostream>

extern "C" {

int SDL_AppInit(void **appstate, int argc, char **argv)
{
    std::cout << "SDL_AppInit" << std::endl;
    return 0;
}

int SDL_AppIterate(void *appstate)
{
    static thread_local int i = 0;
    std::cout << "SDL_AppIterate " << i++ << std::endl;
    if(i > 60)
    {
        return 1;
    }
    return 0;
}

int SDL_AppEvent(void *appstate, const SDL_Event *event)
{
    std::cout << "SDL_AppEvent" << std::endl;
    return 0;
}

void SDL_AppQuit(void *appstate)
{
    std::cout << "SDL_AppQuit" << std::endl;
    return;
}

}