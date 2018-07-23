#pragma once

#include <StdInc.h>
#include <singleton.h>
#include <graphics/Types.h>

class Camera : public Singleton<Camera>
{
private:
    glm::vec3 m_Position;
    glm::vec3 m_Rotation;

    bool m_IsRotatingView = false;

    float m_FOV = 45.0f;
    float m_NearClip = 0.1f;
    float m_FarClip = 10000.0f;
    glm::mat4 m_View = glm::mat4(1);
    glm::mat4 m_Projection = glm::mat4(1);
    glm::mat4 m_ViewProjection = glm::mat4(1);
    glm::vec4 m_Viewport = glm::vec4(0);

public:
    Camera();
    virtual ~Camera();

    void Shutdown();

    void Update(RenderContext_t* context);
    void ResetToDefault();

    void WorldToScreen(const glm::vec3& world, glm::vec3* screen);
    void ScreenToWorld(const glm::vec3& screen, glm::vec3* world);

    const glm::vec3& GetPosition() const { return m_Position; }
    const glm::vec3& GetRotation() const { return m_Rotation; }
    const glm::mat4& GetViewMatrix() const { return m_View; }
    const glm::mat4& GetProjectionMatrix() const { return m_Projection; }
    const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjection; }
    const glm::vec4& GetViewport() const { return m_Viewport; }
};