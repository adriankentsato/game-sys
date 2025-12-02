#pragma once

// Silence OpenGL deprecation warnings on macOS
#define GL_SILENCE_DEPRECATION

#include <string>
#include <unordered_map>
#include <memory>

#if defined(__APPLE__)
    #include <OpenGL/gl3.h>
#else
    #include <SDL3/SDL_opengl.h>
#endif

class ShaderManager
{
public:
    // Get singleton instance
    static ShaderManager& getInstance();
    
    // Delete copy constructor and assignment operator
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    
    // Load or get cached shader program
    GLuint getShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);
    
    // Cleanup all shaders
    void cleanup();
    
private:
    ShaderManager() = default;
    ~ShaderManager();
    
    // Helper functions
    std::string readShaderFile(const char* filepath);
    GLuint compileShader(GLenum type, const char* source);
    GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath);
    
    // Cache: key is "vertexPath|fragmentPath", value is shader program ID
    std::unordered_map<std::string, GLuint> m_shaderCache;
    
    // Generate cache key from paths
    std::string makeCacheKey(const std::string& vertexPath, const std::string& fragmentPath);
};
