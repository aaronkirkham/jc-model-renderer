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
    using namespace jc::Vertex;

    // load shaders
    m_VertexShader = ShaderManager::Get()->GetVertexShader("prop");
    m_PixelShader  = ShaderManager::Get()->GetPixelShader("prop");

    // create the input desc
    if (m_Block.m_Attributes.m_Packed.m_Format == PACKED_FORMAT_FLOAT) {
        // clang-format off
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   0,  DXGI_FORMAT_R32G32B32A32_FLOAT,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };
        // clang-format on

        m_VertexDeclaration =
            Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(), "Prop VertexDecl (unpacked)");
    } else {
        // clang-format off
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };
        // clang-format on

        m_VertexDeclaration =
            Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(), "Prop VertexDecl (packed)");
    }

    // create the constant buffer
    m_VertexShaderConstants[0] =
        Renderer::Get()->CreateConstantBuffer(m_cbVertexInstanceConsts, "Prop VertexInstanceConsts");
    m_VertexShaderConstants[1] =
        Renderer::Get()->CreateConstantBuffer(m_cbVertexMaterialConsts, "Prop VertexMaterialConsts");
    m_FragmentShaderConstants =
        Renderer::Get()->CreateConstantBuffer(m_cbFragmentMaterialConsts, "Prop FragmentMaterialConsts");

    // TEMP
    memset(&m_cbFragmentMaterialConsts, 0, sizeof(m_cbFragmentMaterialConsts));
}

void RenderBlockProp::Setup(RenderContext_t* context)
{
    using namespace jc::Vertex;

    if (!m_Visible) {
        return;
    }

    IRenderBlock::Setup(context);

    // setup the constant buffer
    {
        const auto world = glm::scale(glm::mat4(1), glm::vec3{m_Block.m_Attributes.m_Packed.m_Scale});

        // set vertex shader constants
        m_cbVertexInstanceConsts.ViewProjection = context->m_viewProjectionMatrix;
        m_cbVertexInstanceConsts.World          = world;
        m_cbVertexInstanceConsts.Colour         = {0, 0, 0, 1};
        m_cbVertexMaterialConsts.DepthBias      = {m_Block.m_Attributes.m_DepthBias, 0, 0, 0};
        // m_cbVertexMaterialConsts.uv0Extent      = m_Block.attributes.packed.uv0Extent;
        // m_cbVertexMaterialConsts.uv1Extent      = m_Block.attributes.packed.uv1Extent;

        // set fragment shader constants
        //
    }

    // set the diffuse texture
    BindTexture(0, m_SamplerState);

    if (!(m_Block.m_Attributes.m_Flags & SUPPORT_OVERLAY)) {
        // set the culling mode
        context->m_Renderer->SetCullMode(
            (!(m_Block.m_Attributes.m_Flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);

        // set the normals & properties textures
        BindTexture(1, m_SamplerState);
        BindTexture(2, m_SamplerState);

        // set the decal textures
        if (m_Block.m_Attributes.m_Flags & SUPPORT_DECALS) {
            BindTexture(3, 4, m_SamplerState);
            BindTexture(4, 5, m_SamplerState);
            BindTexture(5, 6, m_SamplerState);
        } else {
            context->m_Renderer->ClearTextures(4, 6);
        }
    }

    // set the constant buffers
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 1, m_cbVertexInstanceConsts);
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 2, m_cbVertexMaterialConsts);
    context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants, 1, m_cbFragmentMaterialConsts);

    // if we are using packed vertices, set the 2nd vertex buffer
    if (m_Block.m_Attributes.m_Packed.m_Format == PACKED_FORMAT_INT16) {
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

    ImGuiCustom::DropDownFlags(m_Block.m_Attributes.m_Flags, flag_labels);
}

void RenderBlockProp::DrawUI()
{
    ImGui::Text(ICON_FA_COGS "  Attributes");
    ImGui::DragFloat("Scale", &m_Block.m_Attributes.m_Packed.m_Scale, 0.1f, 10.0f);
    ImGui::DragFloat("Depth Bias", &m_Block.m_Attributes.m_DepthBias, 0.0f, 10.0f);

    // Textures
    ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
    ImGui::Columns(3, nullptr, false);
    {
        DrawUI_Texture("DiffuseMap", 0);

        if (!(m_Block.m_Attributes.m_Flags & SUPPORT_OVERLAY)) {
            DrawUI_Texture("NormalMap", 1);
            DrawUI_Texture("PropertiesMap", 2);

            if (m_Block.m_Attributes.m_Flags & SUPPORT_DECALS) {
                DrawUI_Texture("DecalDiffuseMap", 3);
                DrawUI_Texture("DecalNormalMap", 4);
                DrawUI_Texture("DecalPropertiesMap", 5);
            }
        }
    }
    ImGui::EndColumns();
}
}; // namespace jc3
