#pragma once

// Silence OpenGL deprecation warnings on macOS
#define GL_SILENCE_DEPRECATION

#include <vector>
#include <string>

#if defined(__APPLE__)
    #include <OpenGL/gl3.h>
#else
    #include <SDL3/SDL_opengl.h>
#endif

class Voxel
{
public:
    // Constructor
    Voxel(const std::string& name = "Voxel", 
          float x = 0.0f, float y = 0.0f, float z = 0.0f, float size = 1.0f,
          const std::string& vertexShaderPath = "",
          const std::string& fragmentShaderPath = "");
    
    // Destructor
    ~Voxel();
    
    // Delete copy constructor and assignment operator
    Voxel(const Voxel&) = delete;
    Voxel& operator=(const Voxel&) = delete;
    
    // Move constructor and assignment operator
    Voxel(Voxel&& other) noexcept;
    Voxel& operator=(Voxel&& other) noexcept;
    
    // Render the voxel (uses internal shader if shaderProgram is 0)
    void render(GLuint shaderProgram, const float* viewMatrix, const float* projectionMatrix);
    
    // Render with voxel's own shader
    void render(const float* viewMatrix, const float* projectionMatrix);
    
    // Show ImGui controls for this voxel in its own window
    void showControls();
    
    // Window visibility control
    void setWindowVisible(bool visible) { m_windowVisible = visible; }
    bool isWindowVisible() const { return m_windowVisible; }
    
    // Ray intersection test for picking
    bool intersectsRay(const float* rayOrigin, const float* rayDirection, float& distance) const;
    
    // Setters
    void setName(const std::string& name) { m_name = name; }
    void setPosition(float x, float y, float z);
    void setSize(float size);
    void setRotation(float angleX, float angleY, float angleZ);
    void setColor(float r, float g, float b);
    
    // Screen-space rotation (rotates around camera's horizontal and vertical axes)
    void rotateScreenSpace(float horizontalDelta, float verticalDelta, const float* cameraRight, const float* cameraUp);
    
    // Update for auto-rotation (call each frame with deltaTime)
    void update(float deltaTime);
    
    // Getters
    void getPosition(float& x, float& y, float& z) const;
    float getSize() const { return m_size; }
    void getRotation(float& x, float& y, float& z) const { x = m_rotX; y = m_rotY; z = m_rotZ; }
    const std::string& getName() const { return m_name; }
    GLuint getShaderProgram() const { return m_shaderProgram; }
    
private:
    void initialize();
    void cleanup();
    void updateModelMatrix();
    void updateEulerFromQuaternion();
    
    // Name for ImGui identification
    std::string m_name;
    
    // Shader paths and program
    std::string m_vertexShaderPath;
    std::string m_fragmentShaderPath;
    GLuint m_shaderProgram;
    
    // Position and transform
    float m_posX, m_posY, m_posZ;
    float m_size;
    float m_rotX, m_rotY, m_rotZ;
    
    // Quaternion for screen-space rotation (w, x, y, z)
    float m_quat[4];
    
    // Auto-rotation
    bool m_autoRotate;
    float m_rotationSpeed;
    
    // Color (uniform for all faces, or can be extended)
    float m_colorR, m_colorG, m_colorB;
    
    // OpenGL objects
    GLuint m_VAO;
    GLuint m_VBO;
    GLuint m_EBO;
    bool m_ownsShader; // Whether this voxel loaded its own shader
    
    // Model matrix
    float m_modelMatrix[16];
    
    // Flag to track if OpenGL resources are initialized
    bool m_initialized;
    
    // UI state
    bool m_windowVisible;
};
