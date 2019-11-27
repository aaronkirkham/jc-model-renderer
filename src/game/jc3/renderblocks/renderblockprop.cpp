#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "renderblockprop.h"

#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/imgui/imgui_buttondropdown.h"
#include "graphics/imgui/imgui_disabled.h"
#include "graphics/renderer.h"
#include "graphics/shader_manager.h"

#include "game/render_block_factory.h"

namespace jc3
{
RenderBlockProp::~RenderBlockProp()
{
    Renderer::Get()->DestroyBuffer(m_VertexBufferData);

    for (auto& vsc : m_VertexShaderConstants) {
        Renderer::Get()->DestroyBuffer(vsc);
    }

    Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants);
}

uint32_t RenderBlockProp::GetTypeHash() const
{
    return RenderBlockFactory::RB_PROP;
}

void RenderBlockProp::Create()
{
    // load shaders
    m_VertexShader = ShaderManager::Get()->GetVertexShader("prop");
    m_PixelShader  = ShaderManager::Get()->GetPixelShader("prop");

    // create the input desc
    if (m_Block.attributes.packed.format != 1) {
        // clang-format off
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   0,  DXGI_FORMAT_R32G32B32A32_FLOAT,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };
        // clang-format on

        m_VertexDeclaration =
            Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(), "RenderBlockProp (unpacked)");
    } else {
        // clang-format off
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };
        // clang-format on

        m_VertexDeclaration =
            Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(), "RenderBlockProp (packed)");
    }

    // create the constant buffer
    m_VertexShaderConstants[0] =
        Renderer::Get()->CreateConstantBuffer(m_cbVertexInstanceConsts, "RenderBlockProp cbVertexInstanceConsts");
    m_VertexShaderConstants[1] =
        Renderer::Get()->CreateConstantBuffer(m_cbVertexMaterialConsts, "RenderBlockProp cbVertexMaterialConsts");
    m_FragmentShaderConstants =
        Renderer::Get()->CreateConstantBuffer(m_cbFragmentMaterialConsts, "RenderBlockProp cbFragmentMaterialConsts");

    // TEMP
    memset(&m_cbFragmentMaterialConsts, 0, sizeof(m_cbFragmentMaterialConsts));
}

void RenderBlockProp::Setup(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    IRenderBlock::Setup(context);

    // setup the constant buffer
    {
        const auto scale = m_Block.attributes.packed.scale * m_ScaleModifier;
        const auto world = glm::scale(glm::mat4(1), {scale, scale, scale});

        // set vertex shader constants
        m_cbVertexInstanceConsts.ViewProjection = context->m_viewProjectionMatrix;
        m_cbVertexInstanceConsts.World          = world;
        m_cbVertexInstanceConsts.Colour         = {0, 0, 0, 1};
        m_cbVertexMaterialConsts.DepthBias      = {m_Block.attributes.depthBias, 0, 0, 0};
        // m_cbVertexMaterialConsts.uv0Extent      = m_Block.attributes.packed.uv0Extent;
        // m_cbVertexMaterialConsts.uv1Extent      = m_Block.attributes.packed.uv1Extent;

        // set fragment shader constants
        //
    }

    // set the diffuse texture
    IRenderBlock::BindTexture(0, m_SamplerState);

    if (!(m_Block.attributes.flags & SUPPORT_OVERLAY)) {
        // set the culling mode
        context->m_Renderer->SetCullMode((!(m_Block.attributes.flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK
                                                                                                  : D3D11_CULL_NONE);

        // set the normals & properties textures
        IRenderBlock::BindTexture(1, m_SamplerState);
        IRenderBlock::BindTexture(2, m_SamplerState);

        // set the decal textures
        if (m_Block.attributes.flags & SUPPORT_DECALS) {
            IRenderBlock::BindTexture(3, 4, m_SamplerState);
            IRenderBlock::BindTexture(4, 5, m_SamplerState);
            IRenderBlock::BindTexture(5, 6, m_SamplerState);
        }
    }

    // set the constant buffers
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 1, m_cbVertexInstanceConsts);
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 2, m_cbVertexMaterialConsts);
    context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants, 1, m_cbFragmentMaterialConsts);

    // if we are using packed vertices, set the 2nd vertex buffer
    if (m_Block.attributes.packed.format == 1) {
        context->m_Renderer->SetVertexStream(m_VertexBufferData, 1);
    }
}

void RenderBlockProp::DrawContextMenu()
{
    // clang-format off
    static std::array flag_labels = {
        "Disable Backface Culling",     "Transparency Alpha Blending",  "Support Decals",				"Support Overlay",
    };
    // clang-format on

    ImGuiCustom::DropDownFlags(m_Block.attributes.flags, flag_labels);
}

void RenderBlockProp::DrawUI()
{
    ImGui::Text(ICON_FA_COGS "  Attributes");

    ImGui::SliderFloat("Scale", &m_ScaleModifier, 0.1f, 10.0f);
    ImGui::SliderFloat("Depth Bias", &m_Block.attributes.depthBias, 0.0f, 10.0f);

    // Textures
    ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
    ImGui::Columns(3, nullptr, false);
    {
        IRenderBlock::DrawUI_Texture("DiffuseMap", 0);

        if (!(m_Block.attributes.flags & SUPPORT_OVERLAY)) {
            IRenderBlock::DrawUI_Texture("NormalMap", 1);
            IRenderBlock::DrawUI_Texture("PropertiesMap", 2);

            if (m_Block.attributes.flags & SUPPORT_DECALS) {
                IRenderBlock::DrawUI_Texture("DecalDiffuseMap", 3);
                IRenderBlock::DrawUI_Texture("DecalNormalMap", 4);
                IRenderBlock::DrawUI_Texture("DecalPropertiesMap", 5);
            }
        }
    }
    ImGui::EndColumns();
}
}; // namespace jc3
