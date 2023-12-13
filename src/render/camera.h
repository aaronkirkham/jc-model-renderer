#ifndef JCMR_RENDER_CAMERA_H_HEADER_GUARD
#define JCMR_RENDER_CAMERA_H_HEADER_GUARD

#include "platform.h"

namespace jcmr
{
struct Renderer;
struct RenderContext;

struct Camera {
    static Camera* create(Renderer& renderer);
    static void    destroy(Camera* instance);

    virtual ~Camera() = default;

    virtual void update(RenderContext& context) = 0;
};
} // namespace jcmr

#endif // JCMR_RENDER_CAMERA_H_HEADER_GUARD
