// Silence OpenGL deprecation warnings on macOS
#define GL_SILENCE_DEPRECATION

#include "donut.h"
#include "shader_manager.h"
#include "imgui.h"
#include <cmath>
#include <cstring>
#include <vector>

Donut::Donut(const std::string& name, float x, float y, float z,
             float outerRadius, float innerRadius,
             const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
    : m_name(name)
    , m_vertexShaderPath(vertexShaderPath)
    , m_fragmentShaderPath(fragmentShaderPath)
    , m_shaderProgram(0)
    , m_posX(x), m_posY(y), m_posZ(z)
    , m_outerRadius(outerRadius)
    , m_innerRadius(innerRadius)
    , m_rotX(0.0f), m_rotY(0.0f), m_rotZ(0.0f)
    , m_autoRotate(false)
    , m_rotationSpeed(20.0f)
    , m_colorR(1.0f), m_colorG(0.5f), m_colorB(0.0f)
    , m_majorSegments(48)
    , m_minorSegments(24)
    , m_VAO(0), m_VBO(0), m_EBO(0)
    , m_ownsShader(false)
    , m_initialized(false)
    , m_windowVisible(true)
    , m_vertexCount(0)
    , m_indexCount(0)
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

Donut::~Donut()
{
    cleanup();
}

Donut::Donut(Donut&& other) noexcept
    : m_name(std::move(other.m_name))
    , m_vertexShaderPath(std::move(other.m_vertexShaderPath))
    , m_fragmentShaderPath(std::move(other.m_fragmentShaderPath))
    , m_shaderProgram(other.m_shaderProgram)
    , m_posX(other.m_posX), m_posY(other.m_posY), m_posZ(other.m_posZ)
    , m_outerRadius(other.m_outerRadius)
    , m_innerRadius(other.m_innerRadius)
    , m_rotX(other.m_rotX), m_rotY(other.m_rotY), m_rotZ(other.m_rotZ)
    , m_autoRotate(other.m_autoRotate)
    , m_rotationSpeed(other.m_rotationSpeed)
    , m_colorR(other.m_colorR), m_colorG(other.m_colorG), m_colorB(other.m_colorB)
    , m_majorSegments(other.m_majorSegments)
    , m_minorSegments(other.m_minorSegments)
    , m_VAO(other.m_VAO), m_VBO(other.m_VBO), m_EBO(other.m_EBO)
    , m_ownsShader(other.m_ownsShader)
    , m_initialized(other.m_initialized)
    , m_windowVisible(other.m_windowVisible)
    , m_vertexCount(other.m_vertexCount)
    , m_indexCount(other.m_indexCount)
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

Donut& Donut::operator=(Donut&& other) noexcept
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
        m_outerRadius = other.m_outerRadius;
        m_innerRadius = other.m_innerRadius;
        m_rotX = other.m_rotX;
        m_rotY = other.m_rotY;
        m_rotZ = other.m_rotZ;
        m_autoRotate = other.m_autoRotate;
        m_rotationSpeed = other.m_rotationSpeed;
        m_colorR = other.m_colorR;
        m_colorG = other.m_colorG;
        m_colorB = other.m_colorB;
        m_majorSegments = other.m_majorSegments;
        m_minorSegments = other.m_minorSegments;
        m_VAO = other.m_VAO;
        m_VBO = other.m_VBO;
        m_EBO = other.m_EBO;
        m_ownsShader = other.m_ownsShader;
        m_initialized = other.m_initialized;
        m_windowVisible = other.m_windowVisible;
        m_vertexCount = other.m_vertexCount;
        m_indexCount = other.m_indexCount;
        
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

void Donut::generateTorusGeometry()
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    const float PI = 3.14159265359f;
    float tubeRadius = (m_outerRadius - m_innerRadius) * 0.5f;
    float torusRadius = m_innerRadius + tubeRadius;
    
    // Generate vertices
    for (int i = 0; i <= m_majorSegments; ++i)
    {
        float theta = (float)i / m_majorSegments * 2.0f * PI;
        float cosTheta = std::cos(theta);
        float sinTheta = std::sin(theta);
        
        for (int j = 0; j <= m_minorSegments; ++j)
        {
            float phi = (float)j / m_minorSegments * 2.0f * PI;
            float cosPhi = std::cos(phi);
            float sinPhi = std::sin(phi);
            
            // Position
            float x = (torusRadius + tubeRadius * cosPhi) * cosTheta;
            float y = tubeRadius * sinPhi;
            float z = (torusRadius + tubeRadius * cosPhi) * sinTheta;
            
            // Normal
            float nx = cosPhi * cosTheta;
            float ny = sinPhi;
            float nz = cosPhi * sinTheta;
            
            // Color (gradient based on position using base color)
            float colorVariation = (sinPhi + 1.0f) * 0.5f;
            float r = m_colorR * (0.7f + colorVariation * 0.3f);
            float g = m_colorG * (0.7f + colorVariation * 0.3f);
            float b = m_colorB * (0.7f + colorVariation * 0.3f);
            
            // Add vertex data: position (3) + color (3) + normal (3)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(r);
            vertices.push_back(g);
            vertices.push_back(b);
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
        }
    }
    
    // Generate indices
    for (int i = 0; i < m_majorSegments; ++i)
    {
        for (int j = 0; j < m_minorSegments; ++j)
        {
            int first = i * (m_minorSegments + 1) + j;
            int second = first + m_minorSegments + 1;
            
            // First triangle
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            // Second triangle
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    
    m_vertexCount = vertices.size() / 9;
    m_indexCount = indices.size();
    
    // Upload to GPU
    glBindVertexArray(m_VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
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
}

void Donut::initialize()
{
    if (m_initialized)
        return;
    
    // Create VAO, VBO, and EBO
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    
    generateTorusGeometry();
    
    m_initialized = true;
}

void Donut::cleanup()
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

void Donut::updateModelMatrix()
{
    // Convert quaternion to rotation matrix
    float w = m_quat[0], x = m_quat[1], y = m_quat[2], z = m_quat[3];
    
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;
    
    // Rotation matrix from quaternion (no scale for donut, size controlled by radii)
    m_modelMatrix[0] = 1.0f - 2.0f * (yy + zz);
    m_modelMatrix[1] = 2.0f * (xy + wz);
    m_modelMatrix[2] = 2.0f * (xz - wy);
    m_modelMatrix[3] = 0.0f;
    
    m_modelMatrix[4] = 2.0f * (xy - wz);
    m_modelMatrix[5] = 1.0f - 2.0f * (xx + zz);
    m_modelMatrix[6] = 2.0f * (yz + wx);
    m_modelMatrix[7] = 0.0f;
    
    m_modelMatrix[8] = 2.0f * (xz + wy);
    m_modelMatrix[9] = 2.0f * (yz - wx);
    m_modelMatrix[10] = 1.0f - 2.0f * (xx + yy);
    m_modelMatrix[11] = 0.0f;
    
    // Translation
    m_modelMatrix[12] = m_posX;
    m_modelMatrix[13] = m_posY;
    m_modelMatrix[14] = m_posZ;
    m_modelMatrix[15] = 1.0f;
}

void Donut::updateEulerFromQuaternion()
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
        m_rotY = std::copysign(90.0f, sinp);
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

void Donut::render(GLuint shaderProgram, const float* viewMatrix, const float* projectionMatrix)
{
    if (!m_initialized)
        return;
    
    // Use provided shader or donut's own shader
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
    
    // Draw donut
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Donut::render(const float* viewMatrix, const float* projectionMatrix)
{
    // Use donut's own shader
    render(0, viewMatrix, projectionMatrix);
}

void Donut::setPosition(float x, float y, float z)
{
    m_posX = x;
    m_posY = y;
    m_posZ = z;
    updateModelMatrix();
}

void Donut::setOuterRadius(float radius)
{
    if (radius > m_innerRadius)
    {
        m_outerRadius = radius;
        generateTorusGeometry();
    }
}

void Donut::setInnerRadius(float radius)
{
    if (radius < m_outerRadius && radius > 0.0f)
    {
        m_innerRadius = radius;
        generateTorusGeometry();
    }
}

void Donut::setRotation(float angleX, float angleY, float angleZ)
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

void Donut::setColor(float r, float g, float b)
{
    m_colorR = r;
    m_colorG = g;
    m_colorB = b;
    // Regenerate geometry with new color
    generateTorusGeometry();
}

void Donut::getPosition(float& x, float& y, float& z) const
{
    x = m_posX;
    y = m_posY;
    z = m_posZ;
}

void Donut::showControls()
{
    // Only show window if visible
    if (!m_windowVisible)
        return;
    
    // Create individual window for this donut with close button
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
        
        // Diameter controls
        ImGui::Separator();
        ImGui::Text("Donut Dimensions:");
        float outerDiameter = m_outerRadius * 2.0f;
        float innerDiameter = m_innerRadius * 2.0f;
        
        if (ImGui::SliderFloat("Outer Diameter", &outerDiameter, 0.2f, 5.0f))
        {
            setOuterRadius(outerDiameter * 0.5f);
        }
        
        if (ImGui::SliderFloat("Inner Diameter", &innerDiameter, 0.1f, outerDiameter - 0.1f))
        {
            setInnerRadius(innerDiameter * 0.5f);
        }
        
        // Color control
        float tempColor[3] = {m_colorR, m_colorG, m_colorB};
        if (ImGui::ColorEdit3("Color", tempColor))
        {
            setColor(tempColor[0], tempColor[1], tempColor[2]);
        }
        
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

void Donut::update(float deltaTime)
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

void Donut::rotateScreenSpace(float horizontalDelta, float verticalDelta, const float* cameraRight, const float* cameraUp)
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

bool Donut::intersectsRay(const float* rayOrigin, const float* rayDirection, float& distance) const
{
    // Simplified bounding sphere test for torus
    // Use outer radius as bounding sphere radius
    float dx = rayOrigin[0] - m_posX;
    float dy = rayOrigin[1] - m_posY;
    float dz = rayOrigin[2] - m_posZ;
    
    float a = rayDirection[0] * rayDirection[0] + rayDirection[1] * rayDirection[1] + rayDirection[2] * rayDirection[2];
    float b = 2.0f * (dx * rayDirection[0] + dy * rayDirection[1] + dz * rayDirection[2]);
    float c = dx * dx + dy * dy + dz * dz - m_outerRadius * m_outerRadius;
    
    float discriminant = b * b - 4.0f * a * c;
    
    if (discriminant < 0.0f)
        return false;
    
    float sqrtDisc = std::sqrt(discriminant);
    float t1 = (-b - sqrtDisc) / (2.0f * a);
    float t2 = (-b + sqrtDisc) / (2.0f * a);
    
    if (t1 > 0.0f)
        distance = t1;
    else if (t2 > 0.0f)
        distance = t2;
    else
        return false;
    
    return true;
}
