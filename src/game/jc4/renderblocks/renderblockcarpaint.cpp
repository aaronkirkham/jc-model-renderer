//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
//#include <imgui.h>

#include "renderblockcarpaint.h"

#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/imgui/imgui_buttondropdown.h"
#include "graphics/imgui/imgui_disabled.h"
#include "graphics/renderer.h"
#include "graphics/shader_manager.h"

#include "game/render_block_factory.h"

namespace jc4
{
RenderBlockCarPaint::~RenderBlockCarPaint() {}

uint32_t RenderBlockCarPaint::GetTypeHash() const
{
    // return RenderBlockFactory::RB_CARPAINT;
    return 0xCD931E75;
}
}; // namespace jc4
