#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "libs/maths/fast_inv.sqrt.h"
#include "voxel.h"
#include "donut.h"
#include "shader_manager.h"

#include <stdio.h>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>

// Silence OpenGL deprecation warnings on macOS
#define GL_SILENCE_DEPRECATION

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#if defined(__APPLE__)
    #include <OpenGL/gl3.h>
#else
    // On Windows/Linux, imgui's OpenGL3 backend handles function loading
    #include <SDL3/SDL_opengl.h>
#endif

#define ONE_SECOND_MS 1000

static SDL_Window *window = NULL;
static SDL_GLContext gl_context = NULL;
static char debug_string[32];
static char delta_string[32];
static std::atomic<bool> running(true);
static std::atomic<bool> isFullscreen(false);
static std::atomic<int> windowWidth(800);
static std::atomic<int> windowHeight(450);
static std::mutex renderMutex;

// OpenGL objects
static GLuint shaderProgram = 0;

// Camera variables
static float cameraDistance = 5.0f;
static float cameraYaw = 45.0f;
static float cameraPitch = 30.0f;

// Mouse control variables
static bool mousePressed = false;
static float lastMouseX = 0.0f;
static float lastMouseY = 0.0f;
static float mouseSensitivity = 0.2f;

// Object picking variables
static Voxel* selectedVoxel = nullptr;
static Donut* selectedDonut = nullptr;

// Event watcher callback - called for every event
static bool eventWatcher(void* userdata, SDL_Event* event)
{
    // Trigger a redraw on window events
    if (event->type == SDL_EVENT_WINDOW_RESIZED || 
        event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED ||
        event->type == SDL_EVENT_WINDOW_EXPOSED)
    {
        // Update window dimensions
        int w, h;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        windowWidth.store(w);
        windowHeight.store(h);
    }
    return true;  // Return true to continue processing
}

// Helper function to read shader file
std::string readShaderFile(const char* filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        char errorMsg[256];
        SDL_snprintf(errorMsg, sizeof(errorMsg), "Failed to open shader file: %s", filepath);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Shader Load Error", errorMsg, NULL);
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper function to compile shaders
GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Shader Compilation Failed", infoLog, NULL);
    }
    
    return shader;
}

// Helper function to create shader program
GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath)
{
    std::string vertexCode = readShaderFile(vertexPath);
    std::string fragmentCode = readShaderFile(fragmentPath);
    
    if (vertexCode.empty() || fragmentCode.empty())
    {
        return 0;
    }
    
    const char* vertexShaderSource = vertexCode.c_str();
    const char* fragmentShaderSource = fragmentCode.c_str();
    
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Shader Linking Failed", infoLog, NULL);
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

// Matrix multiplication helper (4x4 matrices)
void multiplyMatrix(float* result, const float* a, const float* b)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            result[i * 4 + j] = 0;
            for (int k = 0; k < 4; k++)
            {
                result[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Couldn't initialize SDL!", SDL_GetError(), NULL);
        return 1;
    }

    // Set OpenGL attributes for modern core profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // 800x450 is 16:9
    window = SDL_CreateWindow("OpenGL Triangle Demo", windowWidth, windowHeight, 
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Couldn't create window!", SDL_GetError(), NULL);
        return 1;
    }

    // Create OpenGL context
    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Couldn't create OpenGL context!", SDL_GetError(), NULL);
        return 1;
    }

    // Enable VSync
    SDL_GL_SetSwapInterval(1);
    
    // Set initial OpenGL viewport
    glViewport(0, 0, windowWidth, windowHeight);

    debug_string[0] = 0;
    delta_string[0] = 0;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    // Get executable directory for shader paths
    const char* basePath = SDL_GetBasePath();
    if (!basePath)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Path Error", "Failed to get base path!", NULL);
        return 1;
    }
    
    // Build shader paths (basePath is managed by SDL, don't free it)
    std::string vertexShaderPath = std::string(basePath) + "shaders/vertex.glsl";
    std::string fragmentShaderPath = std::string(basePath) + "shaders/fragment.glsl";
    
    // Create shader program
    shaderProgram = createShaderProgram(vertexShaderPath.c_str(), fragmentShaderPath.c_str());
    if (shaderProgram == 0)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Shader Error", "Failed to create shader program!", NULL);
        return 1;
    }
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Enable back-face culling (don't render faces pointing away from camera)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // Counter-clockwise winding is front-facing
    
    // Create multiple voxels with their own shaders
    // All voxels use the same shader files, but ShaderManager caches them
    Voxel voxel1("Voxel 1", 0.0f, 0.0f, 0.0f, 1.0f,
                 vertexShaderPath, fragmentShaderPath);
    Voxel voxel2("Voxel 2", 2.5f, 0.0f, 0.0f, 0.75f,
                 vertexShaderPath, fragmentShaderPath);
    Voxel voxel3("Voxel 3", -2.5f, 0.0f, 0.0f, 0.5f,
                 vertexShaderPath, fragmentShaderPath);
    
    // Create donuts
    Donut donut1("Donut 1", 0.0f, 2.0f, 0.0f, 1.0f, 0.4f,
                 vertexShaderPath, fragmentShaderPath);
    Donut donut2("Donut 2", 0.0f, -2.0f, 0.0f, 0.8f, 0.3f,
                 vertexShaderPath, fragmentShaderPath);
    
    // Store voxels in an array for easier picking
    Voxel* voxels[] = {&voxel1, &voxel2, &voxel3};
    const int voxelCount = 3;
    
    // Store donuts in an array
    Donut* donuts[] = {&donut1, &donut2};
    const int donutCount = 2;

    // Make context current on main thread initially
    SDL_GL_MakeCurrent(window, gl_context);
    
    // Add event watcher to catch resize events
    SDL_AddEventWatch(eventWatcher, NULL);
    
    Uint64 accu = 0;
    Uint64 last = 0;
    Uint64 lastFrameTime = SDL_GetTicks();
    float deltaTime = 0.0f;
    
    while(running.load())
    {
        Uint64 now = SDL_GetTicks(); // Get current time in milliseconds
        deltaTime = (float)(now - lastFrameTime) / 1000.0f;
        lastFrameTime = now;
        
        SDL_Event event;

        // Check for events (don't block, use timeout)
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    running.store(false);
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE)
                    {
                        running.store(false);
                    }
                    else if (event.key.key == SDLK_F11)
                    {
                        // Toggle fullscreen
                        bool currentFullscreen = isFullscreen.load();
                        isFullscreen.store(!currentFullscreen);
                        if (!currentFullscreen)
                        {
                            SDL_SetWindowFullscreen(window, true);
                        }
                        else
                        {
                            SDL_SetWindowFullscreen(window, false);
                        }
                    }
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                case SDL_EVENT_WINDOW_EXPOSED:
                    {
                        // Update viewport when window is resized or needs redraw
                        // Use pixel size for actual drawable area (important for high-DPI)
                        int w, h;
                        SDL_GetWindowSizeInPixels(window, &w, &h);
                        windowWidth.store(w);
                        windowHeight.store(h);
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        // Check if mouse is not over ImGui window
                        if (!io.WantCaptureMouse)
                        {
                            mousePressed = true;
                            lastMouseX = event.button.x;
                            lastMouseY = event.button.y;
                            
                            // Perform ray casting to check if we clicked on a voxel
                            // Calculate camera position
                            float camYawRad = cameraYaw * 3.14159265359f / 180.0f;
                            float camPitchRad = cameraPitch * 3.14159265359f / 180.0f;
                            float camX = cameraDistance * std::cos(camPitchRad) * std::cos(camYawRad);
                            float camY = cameraDistance * std::sin(camPitchRad);
                            float camZ = cameraDistance * std::cos(camPitchRad) * std::sin(camYawRad);
                            
                            // Convert mouse position to normalized device coordinates
                            int w = windowWidth.load();
                            int h = windowHeight.load();
                            float ndcX = (2.0f * event.button.x) / w - 1.0f;
                            float ndcY = 1.0f - (2.0f * event.button.y) / h;
                            
                            // Calculate ray direction in world space
                            float aspect = (float)w / (float)h;
                            float fov = 45.0f * 3.14159265359f / 180.0f;
                            float tanHalfFov = std::tan(fov / 2.0f);
                            
                            // Ray in camera space
                            float rayX_cam = ndcX * aspect * tanHalfFov;
                            float rayY_cam = ndcY * tanHalfFov;
                            float rayZ_cam = -1.0f;
                            
                            // Camera basis vectors (from view matrix calculation)
                            float target[3] = {0.0f, 0.0f, 0.0f};
                            float up[3] = {0.0f, 1.0f, 0.0f};
                            
                            float zaxis[3] = {camX - target[0], camY - target[1], camZ - target[2]};
                            float zlen = std::sqrt(zaxis[0]*zaxis[0] + zaxis[1]*zaxis[1] + zaxis[2]*zaxis[2]);
                            zaxis[0] /= zlen; zaxis[1] /= zlen; zaxis[2] /= zlen;
                            
                            float xaxis[3] = {
                                up[1]*zaxis[2] - up[2]*zaxis[1],
                                up[2]*zaxis[0] - up[0]*zaxis[2],
                                up[0]*zaxis[1] - up[1]*zaxis[0]
                            };
                            float xlen = std::sqrt(xaxis[0]*xaxis[0] + xaxis[1]*xaxis[1] + xaxis[2]*xaxis[2]);
                            xaxis[0] /= xlen; xaxis[1] /= xlen; xaxis[2] /= xlen;
                            
                            float yaxis[3] = {
                                zaxis[1]*xaxis[2] - zaxis[2]*xaxis[1],
                                zaxis[2]*xaxis[0] - zaxis[0]*xaxis[2],
                                zaxis[0]*xaxis[1] - zaxis[1]*xaxis[0]
                            };
                            
                            // Transform ray to world space
                            float rayDir[3] = {
                                xaxis[0] * rayX_cam + yaxis[0] * rayY_cam + zaxis[0] * rayZ_cam,
                                xaxis[1] * rayX_cam + yaxis[1] * rayY_cam + zaxis[1] * rayZ_cam,
                                xaxis[2] * rayX_cam + yaxis[2] * rayY_cam + zaxis[2] * rayZ_cam
                            };
                            
                            // Normalize ray direction
                            float rayLen = std::sqrt(rayDir[0]*rayDir[0] + rayDir[1]*rayDir[1] + rayDir[2]*rayDir[2]);
                            rayDir[0] /= rayLen; rayDir[1] /= rayLen; rayDir[2] /= rayLen;
                            
                            float rayOrigin[3] = {camX, camY, camZ};
                            
                            // Test intersection with all objects
                            selectedVoxel = nullptr;
                            selectedDonut = nullptr;
                            float closestDistance = 1e30f;
                            
                            // Test voxels
                            for (int i = 0; i < voxelCount; i++)
                            {
                                float distance;
                                if (voxels[i]->intersectsRay(rayOrigin, rayDir, distance))
                                {
                                    if (distance < closestDistance)
                                    {
                                        closestDistance = distance;
                                        selectedVoxel = voxels[i];
                                        selectedDonut = nullptr;
                                    }
                                }
                            }
                            
                            // Test donuts
                            for (int i = 0; i < donutCount; i++)
                            {
                                float distance;
                                if (donuts[i]->intersectsRay(rayOrigin, rayDir, distance))
                                {
                                    if (distance < closestDistance)
                                    {
                                        closestDistance = distance;
                                        selectedDonut = donuts[i];
                                        selectedVoxel = nullptr;
                                    }
                                }
                            }
                            
                            // Show the selected object's control window if clicked
                            if (selectedVoxel)
                            {
                                selectedVoxel->setWindowVisible(true);
                            }
                            else if (selectedDonut)
                            {
                                selectedDonut->setWindowVisible(true);
                            }
                        }
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        mousePressed = false;
                        selectedVoxel = nullptr;
                        selectedDonut = nullptr;
                    }
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    if (mousePressed && !io.WantCaptureMouse)
                    {
                        float deltaX = event.motion.x - lastMouseX;
                        float deltaY = event.motion.y - lastMouseY;
                        
                        if (selectedVoxel || selectedDonut)
                        {
                            // Calculate camera basis vectors for screen-space rotation
                            float camYawRad = cameraYaw * 3.14159265359f / 180.0f;
                            float camPitchRad = cameraPitch * 3.14159265359f / 180.0f;
                            float camX = cameraDistance * std::cos(camPitchRad) * std::cos(camYawRad);
                            float camY = cameraDistance * std::sin(camPitchRad);
                            float camZ = cameraDistance * std::cos(camPitchRad) * std::sin(camYawRad);
                            
                            float cameraPos[3] = {camX, camY, camZ};
                            float target[3] = {0.0f, 0.0f, 0.0f};
                            float up[3] = {0.0f, 1.0f, 0.0f};
                            
                            // Calculate camera right and up vectors
                            float zaxis[3] = {
                                cameraPos[0] - target[0],
                                cameraPos[1] - target[1],
                                cameraPos[2] - target[2]
                            };
                            float zlen = std::sqrt(zaxis[0]*zaxis[0] + zaxis[1]*zaxis[1] + zaxis[2]*zaxis[2]);
                            zaxis[0] /= zlen; zaxis[1] /= zlen; zaxis[2] /= zlen;
                            
                            float xaxis[3] = {
                                up[1]*zaxis[2] - up[2]*zaxis[1],
                                up[2]*zaxis[0] - up[0]*zaxis[2],
                                up[0]*zaxis[1] - up[1]*zaxis[0]
                            };
                            float xlen = std::sqrt(xaxis[0]*xaxis[0] + xaxis[1]*xaxis[1] + xaxis[2]*xaxis[2]);
                            xaxis[0] /= xlen; xaxis[1] /= xlen; xaxis[2] /= xlen;
                            
                            float yaxis[3] = {
                                zaxis[1]*xaxis[2] - zaxis[2]*xaxis[1],
                                zaxis[2]*xaxis[0] - zaxis[0]*xaxis[2],
                                zaxis[0]*xaxis[1] - zaxis[1]*xaxis[0]
                            };
                            
                            // Apply screen-space rotation
                            float horizontalDelta = deltaX * mouseSensitivity * 2.0f;
                            float verticalDelta = deltaY * mouseSensitivity * 2.0f;
                            
                            if (selectedVoxel)
                                selectedVoxel->rotateScreenSpace(horizontalDelta, verticalDelta, xaxis, yaxis);
                            else if (selectedDonut)
                                selectedDonut->rotateScreenSpace(horizontalDelta, verticalDelta, xaxis, yaxis);
                            
                            // Update last mouse position for object rotation
                            lastMouseX = event.motion.x;
                            lastMouseY = event.motion.y;
                        }
                        else
                        {
                            // Move camera
                            cameraYaw += deltaX * mouseSensitivity;
                            cameraPitch += deltaY * mouseSensitivity;
                            
                            // Clamp pitch to prevent gimbal lock
                            if (cameraPitch > 89.0f) cameraPitch = 89.0f;
                            if (cameraPitch < -89.0f) cameraPitch = -89.0f;
                            
                            // Wrap yaw around
                            if (cameraYaw > 360.0f) cameraYaw -= 360.0f;
                            if (cameraYaw < 0.0f) cameraYaw += 360.0f;
                            
                            lastMouseX = event.motion.x;
                            lastMouseY = event.motion.y;
                        }
                    }
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    if (!io.WantCaptureMouse)
                    {
                        // Zoom in/out with mouse wheel
                        cameraDistance -= event.wheel.y * 0.5f;
                        
                        // Clamp distance
                        if (cameraDistance < 2.0f) cameraDistance = 2.0f;
                        if (cameraDistance > 10.0f) cameraDistance = 10.0f;
                    }
                    break;
                default:
                    break;
            }
        }
        // Events checker
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    
        // Get current window size in pixels every frame
        int currentWidth = windowWidth.load();
        int currentHeight = windowHeight.load();
        
        // Update viewport every frame to ensure it's correct
        glViewport(0, 0, currentWidth, currentHeight);
        
        // Clear screen with OpenGL
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // FPS counter overlay at top left
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        ImGuiWindowFlags fps_window_flags = ImGuiWindowFlags_NoDecoration | 
                                             ImGuiWindowFlags_AlwaysAutoResize | 
                                             ImGuiWindowFlags_NoSavedSettings | 
                                             ImGuiWindowFlags_NoFocusOnAppearing | 
                                             ImGuiWindowFlags_NoNav;
        ImGui::Begin("FPS Counter", nullptr, fps_window_flags);
        ImGui::Text("%.1f FPS", io.Framerate);
        ImGui::Text("%.2f ms", 1000.0f / io.Framerate);
        ImGui::End();
        
        // Camera controls window
        bool showCameraControls = true;
        ImGui::Begin("Camera Controls", &showCameraControls);
        ImGui::Text("Mouse Controls:");
        ImGui::BulletText("Left-click + drag to rotate camera");
        ImGui::BulletText("Scroll wheel to zoom in/out");
        ImGui::Separator();
        
        ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity, 0.05f, 1.0f);
        ImGui::SliderFloat("Camera Distance", &cameraDistance, 2.0f, 10.0f);
        ImGui::SliderFloat("Camera Yaw", &cameraYaw, 0.0f, 360.0f);
        ImGui::SliderFloat("Camera Pitch", &cameraPitch, -89.0f, 89.0f);
        
        ImGui::Separator();
        ImGui::Text("Window Size (pixels): %dx%d", currentWidth, currentHeight);
        
        int logicalW, logicalH;
        SDL_GetWindowSize(window, &logicalW, &logicalH);
        ImGui::Text("Window Size (logical): %dx%d", logicalW, logicalH);
        ImGui::End();
        
        // Update voxels (for auto-rotation)
        voxel1.update(deltaTime);
        voxel2.update(deltaTime);
        voxel3.update(deltaTime);
        
        // Update donuts (for auto-rotation)
        donut1.update(deltaTime);
        donut2.update(deltaTime);
        
        // Show individual voxel control windows
        voxel1.showControls();
        voxel2.showControls();
        voxel3.showControls();
        
        // Show individual donut control windows
        donut1.showControls();
        donut2.showControls();
        
        // Calculate camera position
        float camYawRad = cameraYaw * 3.14159265359f / 180.0f;
        float camPitchRad = cameraPitch * 3.14159265359f / 180.0f;
        
        float camX = cameraDistance * std::cos(camPitchRad) * std::cos(camYawRad);
        float camY = cameraDistance * std::sin(camPitchRad);
        float camZ = cameraDistance * std::cos(camPitchRad) * std::sin(camYawRad);
        
        // Create view matrix (lookAt)
        float cameraPos[3] = {camX, camY, camZ};
        float target[3] = {0.0f, 0.0f, 0.0f};
        float up[3] = {0.0f, 1.0f, 0.0f};
        
        // Calculate camera basis vectors
        float zaxis[3] = {
            cameraPos[0] - target[0],
            cameraPos[1] - target[1],
            cameraPos[2] - target[2]
        };
        float zlen = std::sqrt(zaxis[0]*zaxis[0] + zaxis[1]*zaxis[1] + zaxis[2]*zaxis[2]);
        zaxis[0] /= zlen; zaxis[1] /= zlen; zaxis[2] /= zlen;
        
        float xaxis[3] = {
            up[1]*zaxis[2] - up[2]*zaxis[1],
            up[2]*zaxis[0] - up[0]*zaxis[2],
            up[0]*zaxis[1] - up[1]*zaxis[0]
        };
        float xlen = std::sqrt(xaxis[0]*xaxis[0] + xaxis[1]*xaxis[1] + xaxis[2]*xaxis[2]);
        xaxis[0] /= xlen; xaxis[1] /= xlen; xaxis[2] /= xlen;
        
        float yaxis[3] = {
            zaxis[1]*xaxis[2] - zaxis[2]*xaxis[1],
            zaxis[2]*xaxis[0] - zaxis[0]*xaxis[2],
            zaxis[0]*xaxis[1] - zaxis[1]*xaxis[0]
        };
        
        float view[16] = {
            xaxis[0], yaxis[0], zaxis[0], 0.0f,
            xaxis[1], yaxis[1], zaxis[1], 0.0f,
            xaxis[2], yaxis[2], zaxis[2], 0.0f,
            -(xaxis[0]*cameraPos[0] + xaxis[1]*cameraPos[1] + xaxis[2]*cameraPos[2]),
            -(yaxis[0]*cameraPos[0] + yaxis[1]*cameraPos[1] + yaxis[2]*cameraPos[2]),
            -(zaxis[0]*cameraPos[0] + zaxis[1]*cameraPos[1] + zaxis[2]*cameraPos[2]),
            1.0f
        };
        
        // Create perspective projection matrix
        float aspect = (float)currentWidth / (float)currentHeight;
        float fov = 45.0f * 3.14159265359f / 180.0f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;
        
        float f = 1.0f / std::tan(fov / 2.0f);
        float projection[16] = {
            f / aspect, 0.0f, 0.0f, 0.0f,
            0.0f, f, 0.0f, 0.0f,
            0.0f, 0.0f, (farPlane + nearPlane) / (nearPlane - farPlane), -1.0f,
            0.0f, 0.0f, (2.0f * farPlane * nearPlane) / (nearPlane - farPlane), 0.0f
        };
        
        // Set lighting uniforms for each object's shader
        float lightPos[3] = {5.0f, 5.0f, 5.0f};
        
        // Render all voxels with their own shaders
        for (int i = 0; i < voxelCount; i++)
        {
            GLuint voxelShader = voxels[i]->getShaderProgram();
            if (voxelShader != 0)
            {
                glUseProgram(voxelShader);
                GLint lightPosLoc = glGetUniformLocation(voxelShader, "lightPos");
                GLint viewPosLoc = glGetUniformLocation(voxelShader, "viewPos");
                glUniform3fv(lightPosLoc, 1, lightPos);
                glUniform3fv(viewPosLoc, 1, cameraPos);
            }
            voxels[i]->render(view, projection);
        }
        
        // Render all donuts with their own shaders
        for (int i = 0; i < donutCount; i++)
        {
            GLuint donutShader = donuts[i]->getShaderProgram();
            if (donutShader != 0)
            {
                glUseProgram(donutShader);
                GLint lightPosLoc = glGetUniformLocation(donutShader, "lightPos");
                GLint viewPosLoc = glGetUniformLocation(donutShader, "viewPos");
                glUniform3fv(lightPosLoc, 1, lightPos);
                glUniform3fv(viewPosLoc, 1, cameraPos);
            }
            donuts[i]->render(view, projection);
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        SDL_GL_SwapWindow(window);

        // Reset FPS counter, one second passed
        if (now - last > ONE_SECOND_MS)
        {
            last = now;
            SDL_snprintf(debug_string, sizeof(debug_string), "%" SDL_PRIu64 " fps", accu);
            accu = 0;
        }
        // Reset FPS counter

        accu += 1; // increment FPS counter
    }

    // Remove event watcher
    SDL_RemoveEventWatch(eventWatcher, NULL);
    
    // Cleanup OpenGL objects
    glDeleteProgram(shaderProgram);
    
    // Cleanup shader manager cache
    ShaderManager::getInstance().cleanup();
    
    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
