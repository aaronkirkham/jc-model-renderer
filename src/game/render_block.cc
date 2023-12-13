#include "pch.h"

#include "render_block.h"

#include "app/app.h"

#include "render/renderer.h"
#include "render/shader.h"
#include "render/texture.h"

#include <d3d11.h>

namespace jcmr::game
{
IRenderBlock::~IRenderBlock()
{
    auto& renderer = m_app.get_renderer();
    renderer.destroy_buffer(m_vertex_buffer);
    renderer.destroy_buffer(m_index_buffer);
}

bool IRenderBlock::setup(RenderContext& context)
{
    if (!m_vertex_shader || !m_pixel_shader || !m_vertex_buffer) return false;

    // bind shaders
    context.device_context->VSSetShader(m_vertex_shader->get_vertex_shader(), nullptr, 0);
    context.device_context->PSSetShader(m_pixel_shader->get_pixel_shader(), nullptr, 0);

    // bind vertex declaration
    context.device_context->IASetInputLayout(m_vertex_shader->get_input_layout());

    // bind vertex buffer
    context.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context.renderer->set_vertex_stream(m_vertex_buffer, 0);
    return true;
}

void IRenderBlock::draw(RenderContext& context)
{
    if (m_index_buffer) {
        context.renderer->draw_indexed(m_index_buffer);
        return;
    }

    context.renderer->draw(0, (m_vertex_buffer->count / 3));
}
} // namespace jcmr::game