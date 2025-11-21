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

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
    
    // Triangle control variables
    float triangle_scale = 1.0f;
    float triangle_rotation = 0.0f;
    float color_intensity = 1.0f;
    float color_top[3] = {1.0f, 0.0f, 0.0f};      // Red
    float color_bottom_left[3] = {0.0f, 1.0f, 0.0f};  // Green
    float color_bottom_right[3] = {0.0f, 0.0f, 1.0f}; // Blue

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
            ImGui_ImplSDL3_ProcessEvent(&event);
            
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
        
        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        // Clear screen

        // ImGui window for triangle controls
        ImGui::Begin("Triangle Controls");
        ImGui::SliderFloat("Scale", &triangle_scale, 0.1f, 2.0f);
        ImGui::SliderFloat("Rotation", &triangle_rotation, 0.0f, 360.0f);
        ImGui::SliderFloat("Color Intensity", &color_intensity, 0.0f, 1.0f);
        
        ImGui::Separator();
        ImGui::Text("Corner Colors:");
        ImGui::ColorEdit3("Top (Red)", color_top);
        ImGui::ColorEdit3("Bottom Left (Green)", color_bottom_left);
        ImGui::ColorEdit3("Bottom Right (Blue)", color_bottom_right);
        
        ImGui::Separator();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
        
        // Render triangle with RGB colors
        SDL_Vertex vertices[3];
        
        // Calculate center point for rotation
        float centerX = 400.0f;
        float centerY = 250.0f;
        float rad = triangle_rotation * 3.14159f / 180.0f;
        float cosR = SDL_cosf(rad);
        float sinR = SDL_sinf(rad);
        
        // Top vertex
        float x0 = 0.0f * triangle_scale;
        float y0 = -150.0f * triangle_scale;
        vertices[0].position.x = centerX + (x0 * cosR - y0 * sinR);
        vertices[0].position.y = centerY + (x0 * sinR + y0 * cosR);
        vertices[0].color.r = color_top[0] * color_intensity;
        vertices[0].color.g = color_top[1] * color_intensity;
        vertices[0].color.b = color_top[2] * color_intensity;
        vertices[0].color.a = 1.0f;
        vertices[0].tex_coord.x = 0.0f;
        vertices[0].tex_coord.y = 0.0f;
        
        // Bottom left vertex
        float x1 = -200.0f * triangle_scale;
        float y1 = 100.0f * triangle_scale;
        vertices[1].position.x = centerX + (x1 * cosR - y1 * sinR);
        vertices[1].position.y = centerY + (x1 * sinR + y1 * cosR);
        vertices[1].color.r = color_bottom_left[0] * color_intensity;
        vertices[1].color.g = color_bottom_left[1] * color_intensity;
        vertices[1].color.b = color_bottom_left[2] * color_intensity;
        vertices[1].color.a = 1.0f;
        vertices[1].tex_coord.x = 0.0f;
        vertices[1].tex_coord.y = 1.0f;
        
        // Bottom right vertex
        float x2 = 200.0f * triangle_scale;
        float y2 = 100.0f * triangle_scale;
        vertices[2].position.x = centerX + (x2 * cosR - y2 * sinR);
        vertices[2].position.y = centerY + (x2 * sinR + y2 * cosR);
        vertices[2].color.r = color_bottom_right[0] * color_intensity;
        vertices[2].color.g = color_bottom_right[1] * color_intensity;
        vertices[2].color.b = color_bottom_right[2] * color_intensity;
        vertices[2].color.a = 1.0f;
        vertices[2].tex_coord.x = 1.0f;
        vertices[2].tex_coord.y = 1.0f;
        
        SDL_RenderGeometry(renderer, NULL, vertices, 3, NULL, 0);
        // Render triangle

        // Render FPS counter
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(renderer, 10, 10, debug_string);
        // FPS counter

        // Render delta time
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(renderer, 10, 20, delta_string);
        // Render delta time

        // Render ImGui
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

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

    // Cleanup ImGui
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
