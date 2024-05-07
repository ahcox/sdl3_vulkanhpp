#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
#include <iostream>

namespace {
    struct AppState
    {
        /// The window we'll open to show our rendering inside.
        SDL_Window *window{nullptr};
        /// Count of the number of times the main loop has been run.
        long long iterations{0};
    };
}

extern "C" {

int SDL_AppInit(void **appstate, int argc, char **argv)
{
    std::cerr << "SDL_AppInit" << std::endl;

    *appstate = new AppState;
    AppState& state = *static_cast<AppState*>(*appstate);

    int result = 0;
    if(result = SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
    {
        std::cerr << "SDL_InitSubSystem failed with code " << result << std::endl;
        goto error_exit;
    }

    state.window = SDL_CreateWindow( "SDL3 Window", 960, 540, 0 /* | SDL_WINDOW_VULKAN*/ );
    if( state.window == NULL )
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
    AppState& state = *static_cast<AppState*>(appstate);
    ++state.iterations;
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
    AppState* state = static_cast<AppState*>(appstate);
    std::cerr << "SDL_AppQuit after " << state->iterations << " iterations of the main loop." << std::endl;
    delete state;
    return;
}

}