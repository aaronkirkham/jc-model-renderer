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

    void update(RenderContext& context) override
    {
        if (m_is_dirty) {
            update_matrices();
            m_is_dirty = false;
        }

        // update the render context
        context.proj_matrix      = m_projection_matrix;
        context.view_matrix      = m_view_matrix;
        context.view_proj_matrix = m_view_projection_matrix;
    }

  private:
    void update_matrices()
    {
        // TODO
        float width  = 1920.0f;
        float height = 1080.0f;

        m_projection_matrix = glm::perspectiveFovLH(glm::radians(m_fov), width, height, m_near_clip, m_far_clip);

        m_view_matrix = glm::translate(mat4(1), m_position);
        m_view_matrix *= glm::toMat4(m_rotation);
        m_view_matrix = glm::inverse(m_view_matrix);

        m_view_projection_matrix = (m_projection_matrix * m_view_matrix);
    }

  private:
    Renderer& m_renderer;
    float     m_fov                    = 45.0f;
    float     m_near_clip              = 0.1f;
    float     m_far_clip               = 10'000.0f;
    vec3      m_position               = vec3(0, 3, -10);
    quat      m_rotation               = quat();
    mat4      m_view_matrix            = mat4(1);
    mat4      m_projection_matrix      = mat4(1);
    mat4      m_view_projection_matrix = mat4(1);
    bool      m_is_dirty               = true;
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