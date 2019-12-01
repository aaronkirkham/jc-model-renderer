#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "renderblockgeneral.h"

#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/imgui/imgui_buttondropdown.h"
#include "graphics/renderer.h"
#include "graphics/shader_manager.h"

#include "game/render_block_factory.h"

namespace jc3
{
RenderBlockGeneral::~RenderBlockGeneral()
{
    Renderer::Get()->DestroyBuffer(m_VertexBufferData);

    for (auto& vsc : m_VertexShaderConstants) {
        Renderer::Get()->DestroyBuffer(vsc);
    }

    Renderer::Get()->DestroyBuffer(m_FragmentShaderConstant);
}

uint32_t RenderBlockGeneral::GetTypeHash() const
{
    return RenderBlockFactory::RB_GENERAL;
}

void RenderBlockGeneral::CreateBuffers(std::vector<uint8_t>* vertex_buffer, std::vector<uint8_t>* vertex_data_buffer,
                                       std::vector<uint8_t>* index_buffer)
{
    using namespace jc::Vertex;

    uint32_t stride_1 = sizeof(jc::Vertex::PackedVertexPosition);
    uint32_t stride_2 = sizeof(jc::Vertex::GeneralShortPacked);

    m_VertexBuffer =
        Renderer::Get()->CreateBuffer(vertex_buffer->data(), vertex_buffer->size() / stride_1, stride_1, VERTEX_BUFFER);
    m_VertexBufferData = Renderer::Get()->CreateBuffer(vertex_data_buffer->data(),
                                                       vertex_data_buffer->size() / stride_2, stride_2, VERTEX_BUFFER);
    m_IndexBuffer      = Renderer::Get()->CreateBuffer(index_buffer->data(), index_buffer->size() / sizeof(uint16_t),
                                                  sizeof(uint16_t), INDEX_BUFFER);

    m_Block                                   = {};
    m_Block.m_Attributes.m_Packed.m_Format    = PACKED_FORMAT_INT16;
    m_Block.m_Attributes.m_Packed.m_Scale     = 1.0f;
    m_Block.m_Attributes.m_Packed.m_UV0Extent = {1.0f, 1.0f};

    Create();
}

void RenderBlockGeneral::Create()
{
    using namespace jc::Vertex;

    // load shaders
    m_VertexShader = ShaderManager::Get()->GetVertexShader("general");
    m_PixelShader  = ShaderManager::Get()->GetPixelShader("general");

    // create the input desc
    if (m_Block.m_Attributes.m_Packed.m_Format == PACKED_FORMAT_FLOAT) {
        // clang-format off
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   0,  DXGI_FORMAT_R32G32B32A32_FLOAT,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };
        // clang-format on

        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(),
                                                                       "RenderBlockGeneral (unpacked)");
    } else {
        // clang-format off
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };
        // clang-format on

        m_VertexDeclaration =
            Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(), "RenderBlockGeneral (packed)");
    }

    // create the constant buffer
    m_VertexShaderConstants[0] =
        Renderer::Get()->CreateConstantBuffer(m_cbVertexInstanceConsts, "RenderBlockGeneral cbVertexInstanceConsts");
    m_VertexShaderConstants[1] =
        Renderer::Get()->CreateConstantBuffer(m_cbVertexMaterialConsts, "RenderBlockGeneral cbVertexMaterialConsts");
    m_FragmentShaderConstant = Renderer::Get()->CreateConstantBuffer(m_cbFragmentMaterialConsts,
                                                                     "RenderBlockGeneral cbFragmentMaterialConsts");
}

void RenderBlockGeneral::Setup(RenderContext_t* context)
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
        m_cbVertexInstanceConsts.ViewProjection = world * context->m_viewProjectionMatrix;
        // m_cbVertexInstanceConsts.World          = world;
        // m_cbVertexInstanceConsts.colour         = glm::vec4(1, 1, 1, 1);
        // m_cbVertexInstanceConsts._thing         = glm::vec4(0, 1, 2, 1);
        m_cbVertexInstanceConsts._thing    = glm::vec4(0, 0, 0, 1);
        m_cbVertexInstanceConsts._thing2   = world;
        m_cbVertexMaterialConsts.DepthBias = {m_Block.m_Attributes.m_DepthBias, 0, 0, 0};
        m_cbVertexMaterialConsts.uv0Extent = m_Block.m_Attributes.m_Packed.m_UV0Extent;
        m_cbVertexMaterialConsts.uv1Extent = m_Block.m_Attributes.m_Packed.m_UV1Extent;

        // set fragment shader constants
        m_cbFragmentMaterialConsts.specularGloss = m_Block.m_Attributes.m_SpecularGloss;
    }

    // set the textures
    for (int i = 0; i < m_Textures.size(); ++i) {
        IRenderBlock::BindTexture(i, m_SamplerState);
    }

    // set the constant buffers
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 1, m_cbVertexInstanceConsts);
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 2, m_cbVertexMaterialConsts);
    context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstant, 2, m_cbFragmentMaterialConsts);

    // set the culling mode
    context->m_Renderer->SetCullMode((!(m_Block.m_Attributes.m_Flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK
                                                                                                  : D3D11_CULL_NONE);

    // if we are using packed vertices, set the 2nd vertex buffer
    if (m_Block.m_Attributes.m_Packed.m_Format == PACKED_FORMAT_INT16) {
        context->m_Renderer->SetVertexStream(m_VertexBufferData, 1);
    }
}

void RenderBlockGeneral::DrawContextMenu()
{
    // clang-format off
    static std::array flag_labels = {
        "Disable Backface Culling",     "Transparency Alpha Blending",  "",								"",
        "Transparency Alpha Testing",   "",                             "",                             ""
    };
    // clang-format on

    ImGuiCustom::DropDownFlags(m_Block.m_Attributes.m_Flags, flag_labels);
}

void RenderBlockGeneral::DrawUI()
{
    ImGui::Text(ICON_FA_COGS "  Attributes");
    ImGui::DragFloat("Scale", &m_Block.m_Attributes.m_Packed.m_Scale, 1.0f, 0.0f);
    ImGui::DragFloat("Depth Bias", &m_Block.m_Attributes.m_DepthBias, 0.0f, 10.0f);
    ImGui::DragFloat("Specular Gloss", &m_Block.m_Attributes.m_SpecularGloss, 0.0f, 10.0f);

    // Textures
    ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
    ImGui::Columns(3, nullptr, false);
    {
        IRenderBlock::DrawUI_Texture("DiffuseMap", 0);
        IRenderBlock::DrawUI_Texture("NormalMap", 1);
        IRenderBlock::DrawUI_Texture("PropertiesMap", 2);
        IRenderBlock::DrawUI_Texture("AOBlendMap", 3);
    }
    ImGui::EndColumns();
}
}; // namespace jc3
