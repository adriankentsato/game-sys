// Silence OpenGL deprecation warnings on macOS
#define GL_SILENCE_DEPRECATION

#include "voxel.h"
#include "shader_manager.h"
#include "imgui.h"
#include <cmath>
#include <cstring>

Voxel::Voxel(const std::string& name, float x, float y, float z, float size,
             const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
    : m_name(name)
    , m_vertexShaderPath(vertexShaderPath)
    , m_fragmentShaderPath(fragmentShaderPath)
    , m_shaderProgram(0)
    , m_posX(x), m_posY(y), m_posZ(z)
    , m_size(size)
    , m_rotX(0.0f), m_rotY(0.0f), m_rotZ(0.0f)
    , m_autoRotate(false)
    , m_rotationSpeed(20.0f)
    , m_colorR(1.0f), m_colorG(1.0f), m_colorB(1.0f)
    , m_VAO(0), m_VBO(0), m_EBO(0)
    , m_ownsShader(false)
    , m_initialized(false)
    , m_windowVisible(true)
{
    std::memset(m_modelMatrix, 0, sizeof(m_modelMatrix));
    
    // Initialize quaternion to identity (no rotation)
    m_quat[0] = 1.0f; // w
    m_quat[1] = 0.0f; // x
    m_quat[2] = 0.0f; // y
    m_quat[3] = 0.0f; // z
    
    // Load shader if paths are provided
    if (!m_vertexShaderPath.empty() && !m_fragmentShaderPath.empty())
    {
        m_shaderProgram = ShaderManager::getInstance().getShaderProgram(
            m_vertexShaderPath, m_fragmentShaderPath);
        m_ownsShader = true;
    }
    
    initialize();
    updateModelMatrix();
}

Voxel::~Voxel()
{
    cleanup();
}

Voxel::Voxel(Voxel&& other) noexcept
    : m_name(std::move(other.m_name))
    , m_vertexShaderPath(std::move(other.m_vertexShaderPath))
    , m_fragmentShaderPath(std::move(other.m_fragmentShaderPath))
    , m_shaderProgram(other.m_shaderProgram)
    , m_posX(other.m_posX), m_posY(other.m_posY), m_posZ(other.m_posZ)
    , m_size(other.m_size)
    , m_rotX(other.m_rotX), m_rotY(other.m_rotY), m_rotZ(other.m_rotZ)
    , m_autoRotate(other.m_autoRotate)
    , m_rotationSpeed(other.m_rotationSpeed)
    , m_colorR(other.m_colorR), m_colorG(other.m_colorG), m_colorB(other.m_colorB)
    , m_VAO(other.m_VAO), m_VBO(other.m_VBO), m_EBO(other.m_EBO)
    , m_ownsShader(other.m_ownsShader)
    , m_initialized(other.m_initialized)
    , m_windowVisible(other.m_windowVisible)
{
    std::memcpy(m_modelMatrix, other.m_modelMatrix, sizeof(m_modelMatrix));
    std::memcpy(m_quat, other.m_quat, sizeof(m_quat));
    
    // Reset other's resources
    other.m_shaderProgram = 0;
    other.m_VAO = 0;
    other.m_VBO = 0;
    other.m_EBO = 0;
    other.m_ownsShader = false;
    other.m_initialized = false;
}

Voxel& Voxel::operator=(Voxel&& other) noexcept
{
    if (this != &other)
    {
        cleanup();
        
        m_name = std::move(other.m_name);
        m_vertexShaderPath = std::move(other.m_vertexShaderPath);
        m_fragmentShaderPath = std::move(other.m_fragmentShaderPath);
        m_shaderProgram = other.m_shaderProgram;
        m_posX = other.m_posX;
        m_posY = other.m_posY;
        m_posZ = other.m_posZ;
        m_size = other.m_size;
        m_rotX = other.m_rotX;
        m_rotY = other.m_rotY;
        m_rotZ = other.m_rotZ;
        m_autoRotate = other.m_autoRotate;
        m_rotationSpeed = other.m_rotationSpeed;
        m_colorR = other.m_colorR;
        m_colorG = other.m_colorG;
        m_colorB = other.m_colorB;
        m_VAO = other.m_VAO;
        m_VBO = other.m_VBO;
        m_EBO = other.m_EBO;
        m_ownsShader = other.m_ownsShader;
        m_initialized = other.m_initialized;
        m_windowVisible = other.m_windowVisible;
        
        std::memcpy(m_modelMatrix, other.m_modelMatrix, sizeof(m_modelMatrix));
        std::memcpy(m_quat, other.m_quat, sizeof(m_quat));
        
        other.m_shaderProgram = 0;
        other.m_VAO = 0;
        other.m_VBO = 0;
        other.m_EBO = 0;
        other.m_ownsShader = false;
        other.m_initialized = false;
    }
    return *this;
}

void Voxel::initialize()
{
    if (m_initialized)
        return;
    
    // Create cube vertices (position + color + normal)
    // Each face has a different color
    float cubeVertices[] = {
        // Front face (red)
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f,  0.0f,  1.0f,
        
        // Back face (green)
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f,  0.0f, -1.0f,
        
        // Left face (blue)
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f, -1.0f,  0.0f,  0.0f,
        
        // Right face (yellow)
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  1.0f,  0.0f,  0.0f,
        
        // Top face (cyan)
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  0.0f,  1.0f,  0.0f,
        
        // Bottom face (magenta)
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  0.0f, -1.0f,  0.0f
    };
    
    // Cube indices for drawing with triangles
    unsigned int cubeIndices[] = {
        // Front face
        0, 1, 2,  2, 3, 0,
        // Back face
        4, 6, 5,  6, 4, 7,
        // Left face
        8, 9, 10,  10, 11, 8,
        // Right face
        12, 14, 13,  14, 12, 15,
        // Top face
        16, 17, 18,  18, 19, 16,
        // Bottom face
        20, 22, 21,  22, 20, 23
    };
    
    // Create VAO, VBO, and EBO
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    
    glBindVertexArray(m_VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);
    
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Normal attribute (location 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    m_initialized = true;
}

void Voxel::cleanup()
{
    if (m_initialized)
    {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);
        
        m_VAO = 0;
        m_VBO = 0;
        m_EBO = 0;
        m_initialized = false;
    }
}

void Voxel::updateModelMatrix()
{
    // Convert quaternion to rotation matrix
    float w = m_quat[0], x = m_quat[1], y = m_quat[2], z = m_quat[3];
    
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;
    
    // Rotation matrix from quaternion with scale
    m_modelMatrix[0] = m_size * (1.0f - 2.0f * (yy + zz));
    m_modelMatrix[1] = m_size * (2.0f * (xy + wz));
    m_modelMatrix[2] = m_size * (2.0f * (xz - wy));
    m_modelMatrix[3] = 0.0f;
    
    m_modelMatrix[4] = m_size * (2.0f * (xy - wz));
    m_modelMatrix[5] = m_size * (1.0f - 2.0f * (xx + zz));
    m_modelMatrix[6] = m_size * (2.0f * (yz + wx));
    m_modelMatrix[7] = 0.0f;
    
    m_modelMatrix[8] = m_size * (2.0f * (xz + wy));
    m_modelMatrix[9] = m_size * (2.0f * (yz - wx));
    m_modelMatrix[10] = m_size * (1.0f - 2.0f * (xx + yy));
    m_modelMatrix[11] = 0.0f;
    
    // Translation
    m_modelMatrix[12] = m_posX;
    m_modelMatrix[13] = m_posY;
    m_modelMatrix[14] = m_posZ;
    m_modelMatrix[15] = 1.0f;
}

void Voxel::updateEulerFromQuaternion()
{
    // Convert quaternion to Euler angles (ZYX order)
    float w = m_quat[0], x = m_quat[1], y = m_quat[2], z = m_quat[3];
    
    // Roll (X-axis rotation)
    float sinr_cosp = 2.0f * (w * x + y * z);
    float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
    m_rotX = std::atan2(sinr_cosp, cosr_cosp) * 180.0f / 3.14159265359f;
    
    // Pitch (Y-axis rotation)
    float sinp = 2.0f * (w * y - z * x);
    if (std::abs(sinp) >= 1.0f)
        m_rotY = std::copysign(90.0f, sinp); // Use 90 degrees if out of range
    else
        m_rotY = std::asin(sinp) * 180.0f / 3.14159265359f;
    
    // Yaw (Z-axis rotation)
    float siny_cosp = 2.0f * (w * z + x * y);
    float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
    m_rotZ = std::atan2(siny_cosp, cosy_cosp) * 180.0f / 3.14159265359f;
    
    // Normalize angles to [0, 360)
    if (m_rotX < 0.0f) m_rotX += 360.0f;
    if (m_rotY < 0.0f) m_rotY += 360.0f;
    if (m_rotZ < 0.0f) m_rotZ += 360.0f;
}

void Voxel::render(GLuint shaderProgram, const float* viewMatrix, const float* projectionMatrix)
{
    if (!m_initialized)
        return;
    
    // Use provided shader or voxel's own shader
    GLuint programToUse = (shaderProgram != 0) ? shaderProgram : m_shaderProgram;
    
    if (programToUse == 0)
        return; // No shader available
    
    // Use shader program and set uniforms
    glUseProgram(programToUse);
    
    GLint modelLoc = glGetUniformLocation(programToUse, "model");
    GLint viewLoc = glGetUniformLocation(programToUse, "view");
    GLint projLoc = glGetUniformLocation(programToUse, "projection");
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, m_modelMatrix);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMatrix);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projectionMatrix);
    
    // Draw voxel
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Voxel::render(const float* viewMatrix, const float* projectionMatrix)
{
    // Use voxel's own shader
    render(0, viewMatrix, projectionMatrix);
}

void Voxel::setPosition(float x, float y, float z)
{
    m_posX = x;
    m_posY = y;
    m_posZ = z;
    updateModelMatrix();
}

void Voxel::setSize(float size)
{
    m_size = size;
    updateModelMatrix();
}

void Voxel::setRotation(float angleX, float angleY, float angleZ)
{
    m_rotX = angleX;
    m_rotY = angleY;
    m_rotZ = angleZ;
    
    // Convert Euler angles to quaternion (Z * Y * X order)
    float radX = angleX * 3.14159265359f / 180.0f;
    float radY = angleY * 3.14159265359f / 180.0f;
    float radZ = angleZ * 3.14159265359f / 180.0f;
    
    float cx = std::cos(radX * 0.5f), sx = std::sin(radX * 0.5f);
    float cy = std::cos(radY * 0.5f), sy = std::sin(radY * 0.5f);
    float cz = std::cos(radZ * 0.5f), sz = std::sin(radZ * 0.5f);
    
    m_quat[0] = cx * cy * cz + sx * sy * sz; // w
    m_quat[1] = sx * cy * cz - cx * sy * sz; // x
    m_quat[2] = cx * sy * cz + sx * cy * sz; // y
    m_quat[3] = cx * cy * sz - sx * sy * cz; // z
    
    updateModelMatrix();
}

void Voxel::setColor(float r, float g, float b)
{
    m_colorR = r;
    m_colorG = g;
    m_colorB = b;
    // Note: This currently doesn't update the vertex buffer
    // You could extend this to update colors dynamically if needed
}

void Voxel::getPosition(float& x, float& y, float& z) const
{
    x = m_posX;
    y = m_posY;
    z = m_posZ;
}

void Voxel::showControls()
{
    // Only show window if visible
    if (!m_windowVisible)
        return;
    
    // Create individual window for this voxel with close button
    if (ImGui::Begin(m_name.c_str(), &m_windowVisible))
    {
        // Position controls
        ImGui::Text("Position:");
        ImGui::PushItemWidth(100);
        if (ImGui::DragFloat("X##pos", &m_posX, 0.1f, -10.0f, 10.0f))
            updateModelMatrix();
        ImGui::SameLine();
        if (ImGui::DragFloat("Y##pos", &m_posY, 0.1f, -10.0f, 10.0f))
            updateModelMatrix();
        ImGui::SameLine();
        if (ImGui::DragFloat("Z##pos", &m_posZ, 0.1f, -10.0f, 10.0f))
            updateModelMatrix();
        ImGui::PopItemWidth();
        
        // Rotation controls
        ImGui::Text("Rotation:");
        ImGui::PushItemWidth(100);
        if (ImGui::DragFloat("X##rot", &m_rotX, 1.0f, 0.0f, 360.0f))
            setRotation(m_rotX, m_rotY, m_rotZ);
        ImGui::SameLine();
        if (ImGui::DragFloat("Y##rot", &m_rotY, 1.0f, 0.0f, 360.0f))
            setRotation(m_rotX, m_rotY, m_rotZ);
        ImGui::SameLine();
        if (ImGui::DragFloat("Z##rot", &m_rotZ, 1.0f, 0.0f, 360.0f))
            setRotation(m_rotX, m_rotY, m_rotZ);
        ImGui::PopItemWidth();
        
        // Size control
        if (ImGui::SliderFloat("Size", &m_size, 0.1f, 5.0f))
            updateModelMatrix();
        
        // Color control (note: doesn't update vertex buffer yet)
        ImGui::ColorEdit3("Color", &m_colorR);
        
        ImGui::Separator();
        
        // Auto-rotation controls
        ImGui::Checkbox("Auto Rotate", &m_autoRotate);
        if (m_autoRotate)
        {
            ImGui::SliderFloat("Rotation Speed", &m_rotationSpeed, 0.0f, 100.0f);
        }
    }
    ImGui::End();
}

void Voxel::update(float deltaTime)
{
    if (m_autoRotate)
    {
        // Auto-rotate around the world Y-axis (up)
        float angle = m_rotationSpeed * deltaTime * 3.14159265359f / 180.0f;
        float halfAngle = angle * 0.5f;
        float sinHalf = std::sin(halfAngle);
        float cosHalf = std::cos(halfAngle);
        
        // Create rotation quaternion around Y-axis
        float qRot[4] = {
            cosHalf,      // w
            0.0f,         // x
            sinHalf,      // y (up axis)
            0.0f          // z
        };
        
        // Apply rotation to current quaternion (qRot * m_quat)
        float qNew[4];
        qNew[0] = qRot[0] * m_quat[0] - qRot[1] * m_quat[1] - qRot[2] * m_quat[2] - qRot[3] * m_quat[3];
        qNew[1] = qRot[0] * m_quat[1] + qRot[1] * m_quat[0] + qRot[2] * m_quat[3] - qRot[3] * m_quat[2];
        qNew[2] = qRot[0] * m_quat[2] - qRot[1] * m_quat[3] + qRot[2] * m_quat[0] + qRot[3] * m_quat[1];
        qNew[3] = qRot[0] * m_quat[3] + qRot[1] * m_quat[2] - qRot[2] * m_quat[1] + qRot[3] * m_quat[0];
        
        // Normalize to prevent drift
        float len = std::sqrt(qNew[0] * qNew[0] + qNew[1] * qNew[1] + qNew[2] * qNew[2] + qNew[3] * qNew[3]);
        m_quat[0] = qNew[0] / len;
        m_quat[1] = qNew[1] / len;
        m_quat[2] = qNew[2] / len;
        m_quat[3] = qNew[3] / len;
        
        // Update Euler angles for ImGui display
        updateEulerFromQuaternion();
        
        updateModelMatrix();
    }
}

void Voxel::rotateScreenSpace(float horizontalDelta, float verticalDelta, const float* cameraRight, const float* cameraUp)
{
    // Create rotation quaternions for horizontal and vertical rotations
    // Horizontal rotation is around the camera's up axis (screen Y)
    // Vertical rotation is around the camera's right axis (screen X)
    
    // Convert deltas to radians
    float hAngle = horizontalDelta * 3.14159265359f / 180.0f;
    float vAngle = verticalDelta * 3.14159265359f / 180.0f;
    
    // Create quaternion for horizontal rotation (around camera up axis)
    float hCos = std::cos(hAngle * 0.5f);
    float hSin = std::sin(hAngle * 0.5f);
    float qh[4] = {
        hCos,
        cameraUp[0] * hSin,
        cameraUp[1] * hSin,
        cameraUp[2] * hSin
    };
    
    // Create quaternion for vertical rotation (around camera right axis)
    float vCos = std::cos(vAngle * 0.5f);
    float vSin = std::sin(vAngle * 0.5f);
    float qv[4] = {
        vCos,
        cameraRight[0] * vSin,
        cameraRight[1] * vSin,
        cameraRight[2] * vSin
    };
    
    // Combine rotations: first vertical, then horizontal (qh * qv)
    float qCombined[4];
    qCombined[0] = qh[0] * qv[0] - qh[1] * qv[1] - qh[2] * qv[2] - qh[3] * qv[3];
    qCombined[1] = qh[0] * qv[1] + qh[1] * qv[0] + qh[2] * qv[3] - qh[3] * qv[2];
    qCombined[2] = qh[0] * qv[2] - qh[1] * qv[3] + qh[2] * qv[0] + qh[3] * qv[1];
    qCombined[3] = qh[0] * qv[3] + qh[1] * qv[2] - qh[2] * qv[1] + qh[3] * qv[0];
    
    // Apply combined rotation to current quaternion (qCombined * m_quat)
    float qNew[4];
    qNew[0] = qCombined[0] * m_quat[0] - qCombined[1] * m_quat[1] - qCombined[2] * m_quat[2] - qCombined[3] * m_quat[3];
    qNew[1] = qCombined[0] * m_quat[1] + qCombined[1] * m_quat[0] + qCombined[2] * m_quat[3] - qCombined[3] * m_quat[2];
    qNew[2] = qCombined[0] * m_quat[2] - qCombined[1] * m_quat[3] + qCombined[2] * m_quat[0] + qCombined[3] * m_quat[1];
    qNew[3] = qCombined[0] * m_quat[3] + qCombined[1] * m_quat[2] - qCombined[2] * m_quat[1] + qCombined[3] * m_quat[0];
    
    // Normalize the quaternion to prevent drift
    float len = std::sqrt(qNew[0] * qNew[0] + qNew[1] * qNew[1] + qNew[2] * qNew[2] + qNew[3] * qNew[3]);
    m_quat[0] = qNew[0] / len;
    m_quat[1] = qNew[1] / len;
    m_quat[2] = qNew[2] / len;
    m_quat[3] = qNew[3] / len;
    
    // Update Euler angles for ImGui display
    updateEulerFromQuaternion();
    
    updateModelMatrix();
}

bool Voxel::intersectsRay(const float* rayOrigin, const float* rayDirection, float& distance) const
{
    // AABB (Axis-Aligned Bounding Box) ray intersection test
    // Calculate box bounds in world space
    float halfSize = m_size * 0.5f;
    float minX = m_posX - halfSize;
    float maxX = m_posX + halfSize;
    float minY = m_posY - halfSize;
    float maxY = m_posY + halfSize;
    float minZ = m_posZ - halfSize;
    float maxZ = m_posZ + halfSize;
    
    // Ray-AABB intersection using slab method
    float tMin = -1e30f;
    float tMax = 1e30f;
    
    // X slab
    if (std::abs(rayDirection[0]) > 1e-8f)
    {
        float t1 = (minX - rayOrigin[0]) / rayDirection[0];
        float t2 = (maxX - rayOrigin[0]) / rayDirection[0];
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
    }
    else if (rayOrigin[0] < minX || rayOrigin[0] > maxX)
    {
        return false;
    }
    
    // Y slab
    if (std::abs(rayDirection[1]) > 1e-8f)
    {
        float t1 = (minY - rayOrigin[1]) / rayDirection[1];
        float t2 = (maxY - rayOrigin[1]) / rayDirection[1];
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
    }
    else if (rayOrigin[1] < minY || rayOrigin[1] > maxY)
    {
        return false;
    }
    
    // Z slab
    if (std::abs(rayDirection[2]) > 1e-8f)
    {
        float t1 = (minZ - rayOrigin[2]) / rayDirection[2];
        float t2 = (maxZ - rayOrigin[2]) / rayDirection[2];
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
    }
    else if (rayOrigin[2] < minZ || rayOrigin[2] > maxZ)
    {
        return false;
    }
    
    // Check if there's an intersection
    if (tMax < tMin || tMax < 0.0f)
    {
        return false;
    }
    
    distance = tMin > 0.0f ? tMin : tMax;
    return true;
}
