#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
#include <iostream>

namespace {
    /// The window we'll open to show our rendering inside.
    SDL_Window* window {nullptr};
}

extern "C" {

int SDL_AppInit(void **appstate, int argc, char **argv)
{
    std::cerr << "SDL_AppInit" << std::endl;

    int result = 0;
    if(result = SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
    {
        std::cerr << "SDL_InitSubSystem failed with code " << result << std::endl;
        goto error_exit;
    }
    SDL_SetEventEnabled(SDL_EVENT_TERMINATING, SDL_TRUE);
    SDL_SetEventEnabled(SDL_EVENT_DID_ENTER_BACKGROUND, SDL_TRUE);
    SDL_SetEventEnabled(SDL_EVENT_DID_ENTER_FOREGROUND, SDL_TRUE);
    SDL_SetEventEnabled(SDL_EVENT_KEY_DOWN, SDL_TRUE);
    SDL_SetEventEnabled(SDL_EVENT_KEY_UP, SDL_TRUE);
    SDL_SetEventEnabled(SDL_EVENT_MOUSE_MOTION, SDL_TRUE);

    window = SDL_CreateWindow( "SDL3 Window", 960, 540, 0 /* | SDL_WINDOW_VULKAN*/ );
    if( window == NULL )
    {
        std::cerr << "SDL_CreateWindow failed" << std::endl;
        goto error_exit;
    }

    return 0;

    error_exit:
    std::cerr << "Last SDL error: " << SDL_GetError() << std::endl;
    return -1;
}

int SDL_AppIterate(void *appstate)
{
    static thread_local int i = 0;
    ++i;
    return 0;
}

int SDL_AppEvent(void *appstate, const SDL_Event *event)
{
    std::cerr << "SDL_AppEvent";
    if(event)
    {
        std::cerr << ": type = " << event->type << ", timestamp = " << event->common.timestamp << std::endl;
    }
    std::cerr << std::endl;
    if(event->type == SDL_EVENT_QUIT)
    {
        std::cerr << "SDL_EVENT_QUIT" << std::endl;
        return 1;
    }
    return 0;
}

void SDL_AppQuit(void *appstate)
{
    std::cerr << "SDL_AppQuit" << std::endl;
    return;
}

}