#pragma once

#include <glm/glm.hpp>

#include "singleton.h"

#include <memory>

struct RenderContext_t;
struct BoundingBox;
class RenderBlockModel;

class Camera : public Singleton<Camera>
{
  private:
    glm::vec3 m_Position;
    glm::vec3 m_Rotation;

    bool m_IsTranslatingView = false;
    bool m_IsRotatingView    = false;

    float     m_FOV            = 45.0f;
    float     m_NearClip       = 0.001f;
    float     m_FarClip        = 10000.0f;
    glm::mat4 m_View           = glm::mat4(1);
    glm::mat4 m_Projection     = glm::mat4(1);
    glm::mat4 m_ViewProjection = glm::mat4(1);
    glm::vec2 m_Viewport       = glm::vec2(0);

  public:
    Camera();
    virtual ~Camera() = default;

    void      Update(RenderContext_t* context);
    void      UpdateViewport(const glm::vec2& size);
    glm::vec2 WorldToScreen(const glm::vec3& world);
    glm::vec3 ScreenToWorld(const glm::vec2& screen);

    void                              FocusOn(const BoundingBox& box);
    std::shared_ptr<RenderBlockModel> Pick(const glm::vec2& mouse);

    void OnMousePress(int32_t button, bool is_button_down, const glm::vec2& position);
    void OnMouseMove(const glm::vec2& delta);
    void OnMouseScroll(const float delta);

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

    const glm::vec2& GetViewport() const
    {
        return m_Viewport;
    }
};
