#pragma once

#include <StdInc.h>
#include <graphics/Types.h>
#include <singleton.h>

class RenderBlockModel;
class Camera : public Singleton<Camera>
{
  private:
    glm::vec3 m_Position;
    glm::vec3 m_Rotation;

    bool m_IsTranslatingView = false;
    bool m_IsRotatingView    = false;

    glm::vec2 m_LastWindowSize = glm::vec2(0);
    float     m_FOV            = 45.0f;
    float     m_NearClip       = 0.1f;
    float     m_FarClip        = 10000.0f;
    glm::mat4 m_View           = glm::mat4(1);
    glm::mat4 m_Projection     = glm::mat4(1);
    glm::mat4 m_ViewProjection = glm::mat4(1);
    glm::vec4 m_Viewport       = glm::vec4(0);

  public:
    Camera();
    virtual ~Camera() = default;

    void Update(RenderContext_t* context);
    void UpdateWindowSize(const glm::vec2& size);

    bool WorldToScreen(const glm::vec3& world, glm::vec3* screen);
    void ScreenToWorld(const glm::vec3& screen, glm::vec3* world);
    void MouseToWorld(const glm::vec2& mouse, glm::vec3* world);

    void FocusOn(RenderBlockModel* model);

    const glm::vec3& GetPosition() const
    {
        return m_Position;
    }
    const glm::vec3& GetRotation() const
    {
        return m_Rotation;
    }
    const glm::mat4& GetViewMatrix() const
    {
        return m_View;
    }
    const glm::mat4& GetProjectionMatrix() const
    {
        return m_Projection;
    }
    const glm::mat4& GetViewProjectionMatrix() const
    {
        return m_ViewProjection;
    }
    const glm::vec4& GetViewport() const
    {
        return m_Viewport;
    }
};
