#include "pch.h"

#include "camera.h"

#include "render/renderer.h"

namespace jcmr
{
struct CameraImpl final : Camera {
    CameraImpl(Renderer& renderer)
        : m_renderer(renderer)
    {
    }

    void rotate(float yaw, float pitch) override
    {
        auto rot = m_rotation;

        // yaw
        rot = glm::angleAxis(yaw, vec3(0, -1, 0)) * rot;
        rot = glm::normalize(rot);

        // pitch
        rot = glm::angleAxis(pitch, glm::rotate(rot, vec3(-1, 0, 0))) * rot;
        rot = glm::normalize(rot);

        m_rotation   = rot;
        m_view_dirty = true;
    }

    void update(RenderContext& context) override
    {
        update_matrices(context.width, context.height);

        // update the render context
        context.proj_matrix      = m_projection_matrix;
        context.view_matrix      = m_view_matrix;
        context.view_proj_matrix = m_view_projection_matrix;
    }

    void resize(u32 width, u32 height) override { m_projection_dirty = true; }

  private:
    void update_matrices(u32 width, u32 height)
    {
        if (width == 0 || height == 0) return;

        bool view_projection_dirty = false;

        if (m_projection_dirty) {
            m_projection_matrix =
                glm::perspectiveFov(glm::radians(m_fov), (float)width, (float)height, m_near_clip, m_far_clip);
            m_projection_dirty    = false;
            view_projection_dirty = true;
        }

        if (m_view_dirty) {
            m_view_matrix         = glm::toMat4(m_rotation);
            m_view_matrix         = glm::translate(m_view_matrix, m_position);
            m_view_matrix         = glm::inverse(m_view_matrix);
            m_view_dirty          = false;
            view_projection_dirty = true;
        }

        if (view_projection_dirty) {
            m_view_projection_matrix = (m_projection_matrix * m_view_matrix);
        }
    }

  private:
    Renderer& m_renderer;
    float     m_fov                    = 45.0f;
    float     m_near_clip              = 0.1f;
    float     m_far_clip               = 10'000.0f;
    vec3      m_position               = vec3(0, 1, -5);
    quat      m_rotation               = quat();
    mat4      m_view_matrix            = mat4(1);
    mat4      m_projection_matrix      = mat4(1);
    mat4      m_view_projection_matrix = mat4(1);
    bool      m_view_dirty             = true;
    bool      m_projection_dirty       = true;
};

Camera* Camera::create(Renderer& renderer)
{
    return new CameraImpl(renderer);
}

void Camera::destroy(Camera* instance)
{
    delete instance;
}
} // namespace jcmr