#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define SCREEN_FPS 60
#define ONE_SECOND_MS 1000

static const int SCREEN_TICKS_PER_FRAME_MS = ONE_SECOND_MS / SCREEN_FPS;
static const float FRAME_TIME_S = 1.0f / SCREEN_FPS;

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static char debug_string[32];
static char delta_string[32];
static bool running = true;

int main(int argc, char *argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Couldn't initialize SDL!", SDL_GetError(), NULL);
        return 1;
    }

    // 800x450 is 16:9
    if (!SDL_CreateWindowAndRenderer("hello SDL3 with cmake", 800, 450, 0, &window, &renderer))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Couldn't create window/renderer!", SDL_GetError(), NULL);
        return 1;
    }

    // Enable VSync
    // if(!SDL_SetRenderVSync(renderer, 1))
    // {
    //     SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not enable VSync!", SDL_GetError(), NULL);
    //     return 1;
    // }

    debug_string[0] = 0;
    delta_string[0] = 0;

    while(running)
    {
        Uint64 now = SDL_GetTicks(); // Get current time in milliseconds
        
        static Uint64 accu = 0;
        static Uint64 last = 0;
        static float deltaTime = 0.0f;

        SDL_Event event;

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
        // FPS counter

        // Render delta time
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(renderer, 10, 20, delta_string);
        // Render delta time

        // Show the renderer into the screen
        SDL_RenderPresent(renderer);

        // Reset FPS counter, one second passed
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

        // Check if the current elapsed time is within the frame time
        if (elapsed < SCREEN_TICKS_PER_FRAME_MS)
        {
            SDL_Delay(SCREEN_TICKS_PER_FRAME_MS - elapsed);
            deltaTime = FRAME_TIME_S;
        }
        else
        {
            deltaTime = (float)elapsed / 1000.0f;
        }
        // Wait for next frame

        SDL_snprintf(delta_string, sizeof(delta_string), "%f s", deltaTime);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
