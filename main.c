#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define SCREEN_FPS 60
#define ONE_SECOND_MS 1000ULL

static Uint64 SCREEN_TICKS_PER_FRAME_MS = ONE_SECOND_MS / SCREEN_FPS;
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static char debug_string[32];
static bool running = true;

int main(int argc, char **argv)
{

    if (SDL_Init(SDL_INIT_VIDEO) == false)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Couldn't initialize SDL!", SDL_GetError(), NULL);
        return 1;
    }

    // 800x450 is 16:9
    if (SDL_CreateWindowAndRenderer("hello SDL3 with cmake", 800, 450, 0, &window, &renderer) == false)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Couldn't create window/renderer!", SDL_GetError(), NULL);
        return 1;
    }

    //Enable VSync
    if( SDL_SetRenderVSync( renderer, 1 ) == false )
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not enable VSync!", SDL_GetError(), NULL);
        return 1;
    }

    debug_string[0] = 0;

    SDL_SetRenderVSync(renderer, 1);

    while(running) {
        Uint64 now = SDL_GetTicks(); // Get current time in milliseconds

        SDL_Event event;

        static Uint64 accu = 0;
        static Uint64 last = 0;

        // Check for events
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE)
                    {
                        running = false;
                    }
                default:
                    break;
            }
        }
        // Events checker
    
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        // Clear screen

        // Render FPS counter
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(renderer, 10, 10, debug_string);
        SDL_RenderPresent(renderer);
        // FPS counter

        // Reset FPS counter
        if (now - last > ONE_SECOND_MS)
        {
            last = now;
            SDL_snprintf(debug_string, sizeof(debug_string), "%" SDL_PRIu64 " fps", accu);
            accu = 0;
        }
        // Reset FPS counter

        accu += 1; // increment FPS counter

        // Wait for next frame
        Uint64 elapsed = SDL_GetTicks() - now;
        // Wait for next frame

        if (elapsed < SCREEN_TICKS_PER_FRAME_MS)
        {
            SDL_Delay(SCREEN_TICKS_PER_FRAME_MS - elapsed);
        }
        // Wait for next frame
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
