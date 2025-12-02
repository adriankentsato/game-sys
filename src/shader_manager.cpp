// Silence OpenGL deprecation warnings on macOS
#define GL_SILENCE_DEPRECATION

#include "shader_manager.h"
#include <fstream>
#include <sstream>
#include <SDL3/SDL.h>

ShaderManager& ShaderManager::getInstance()
{
    static ShaderManager instance;
    return instance;
}

ShaderManager::~ShaderManager()
{
    cleanup();
}

std::string ShaderManager::makeCacheKey(const std::string& vertexPath, const std::string& fragmentPath)
{
    return vertexPath + "|" + fragmentPath;
}

GLuint ShaderManager::getShaderProgram(const std::string& vertexPath, const std::string& fragmentPath)
{
    // Check if shader program is already cached
    std::string cacheKey = makeCacheKey(vertexPath, fragmentPath);
    
    auto it = m_shaderCache.find(cacheKey);
    if (it != m_shaderCache.end())
    {
        // Return cached shader program
        return it->second;
    }
    
    // Create new shader program
    GLuint program = createShaderProgram(vertexPath.c_str(), fragmentPath.c_str());
    
    if (program != 0)
    {
        // Cache the shader program
        m_shaderCache[cacheKey] = program;
    }
    
    return program;
}

void ShaderManager::cleanup()
{
    // Delete all shader programs
    for (auto& pair : m_shaderCache)
    {
        glDeleteProgram(pair.second);
    }
    m_shaderCache.clear();
}

std::string ShaderManager::readShaderFile(const char* filepath)
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

GLuint ShaderManager::compileShader(GLenum type, const char* source)
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

GLuint ShaderManager::createShaderProgram(const char* vertexPath, const char* fragmentPath)
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
