//#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
//#include <imgui.h>

#include "renderblockcharacter.h"

#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/imgui/imgui_buttondropdown.h"
#include "graphics/renderer.h"
#include "graphics/shader_manager.h"

#include "game/render_block_factory.h"

namespace jc4
{
RenderBlockCharacter::~RenderBlockCharacter()
{
    Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[0]);
    Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[1]);
    Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[2]);
    Renderer::Get()->DestroyBuffer(m_PixelShaderConstants[0]);
    Renderer::Get()->DestroyBuffer(m_PixelShaderConstants[1]);
    Renderer::Get()->DestroyBuffer(m_PixelShaderConstants[2]);
}

uint32_t RenderBlockCharacter::GetTypeHash() const
{
    return RenderBlockFactory::RB_CHARACTER;
}

void RenderBlockCharacter::Create(const std::string& type_id, IBuffer_t* vertex_buffer, IBuffer_t* index_buffer)
{
    using namespace jc::Vertex::RenderBlockCharacter;

    m_VertexBuffer = vertex_buffer;
    m_IndexBuffer  = index_buffer;
    m_PixelShader  = ShaderManager::Get()->GetPixelShader("character");

    // CharacterMesh1UVMesh
    if (type_id == "CharacterMesh1UVMesh") {
        m_Stride = VertexStrides[0];

        m_VertexShader = ShaderManager::Get()->GetVertexShader("character");

        // create the element input desc
        // clang-format off
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   1,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   4,  DXGI_FORMAT_R16G16_SNORM,           0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   6,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };
        // clang-format on

        // create the vertex declaration
        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 5, m_VertexShader.get(),
                                                                       "RenderBlockCharacter (CharacterMesh1UVMesh)");
    }
    // CharacterMesh2UVMesh
    else if (type_id == "CharacterMesh2UVMesh") {
        m_Stride = VertexStrides[1];

        m_VertexShader = ShaderManager::Get()->GetVertexShader("character2uvs");

        // create the element input desc
        // clang-format off
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   1,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   4,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   6,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };
        // clang-format on

        // create the vertex declaration
        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 5, m_VertexShader.get(),
                                                                       "RenderBlockCharacter (CharacterMesh2UVMesh)");

    } else {
#ifdef DEBUG
        __debugbreak();
#endif
    }

    // create vertex shader constants
    m_VertexShaderConstants[0] =
        Renderer::Get()->CreateConstantBuffer(m_cbLocalConsts, "RenderBlockCharacter cbLocalConsts");
    m_VertexShaderConstants[1] =
        Renderer::Get()->CreateConstantBuffer(m_cbSkinningConsts, "RenderBlockCharacter cbSkinningConsts");
    m_VertexShaderConstants[2] =
        Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "RenderBlockCharacter cbInstanceConsts");

    // create pixel shader constants
    m_PixelShaderConstants[0] =
        Renderer::Get()->CreateConstantBuffer(m_cbDefaultConstants, "RenderBlockCharacter cbDefaultConstants");
    m_PixelShaderConstants[1] =
        Renderer::Get()->CreateConstantBuffer(m_cbUnknownCB4, "RenderBlockCharacter cbUnknownCB4");
    m_PixelShaderConstants[2] =
        Renderer::Get()->CreateConstantBuffer(m_cbUnknownCB6, "RenderBlockCharacter cbUnknownCB6");

    // init the skinning palette data
    for (auto i = 0; i < 70; ++i) {
        m_cbSkinningConsts.MatrixPalette[i] = glm::mat3x4(1);
    }

    //
    memset(&m_cbUnknownCB4, 0, sizeof(m_cbUnknownCB4));
    memset(&m_cbUnknownCB6, 0, sizeof(m_cbUnknownCB6));

    // create sampler state
    D3D11_SAMPLER_DESC params{};
    params.Filter         = D3D11_FILTER_ANISOTROPIC;
    params.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
    params.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
    params.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
    params.MipLODBias     = 0.0f;
    params.MaxAnisotropy  = 8;
    params.ComparisonFunc = D3D11_COMPARISON_NEVER;
    params.MinLOD         = 0.0f;
    params.MaxLOD         = 13.0f;
    m_SamplerState        = Renderer::Get()->CreateSamplerState(params, "RenderBlockCharacter");
}

void RenderBlockCharacter::Setup(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    IRenderBlock::Setup(context);

    static auto world = glm::mat4(1);

    // set vertex shader constants
    m_cbLocalConsts.WorldViewProjection = world * context->m_viewProjectionMatrix;
    m_cbLocalConsts.World               = world;
    m_cbInstanceConsts.Scale            = glm::vec4(1 * m_ScaleModifier);
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 2, m_cbLocalConsts);
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 3, m_cbSkinningConsts);
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[2], 4, m_cbInstanceConsts);

    // set pixel shader constants
    context->m_Renderer->SetPixelShaderConstants(m_PixelShaderConstants[0], 1, m_cbDefaultConstants);
    context->m_Renderer->SetPixelShaderConstants(m_PixelShaderConstants[1], 4, m_cbUnknownCB4);
    context->m_Renderer->SetPixelShaderConstants(m_PixelShaderConstants[2], 6, m_cbUnknownCB6);

    // set textures
    IRenderBlock::BindTexture(0, m_SamplerState);
    IRenderBlock::BindTexture(1, m_SamplerState);
    IRenderBlock::BindTexture(2, m_SamplerState);
}

void RenderBlockCharacter::DrawContextMenu()
{
#ifdef _DEBUG
    m_Attributes;
    __debugbreak();
#endif
}

void RenderBlockCharacter::DrawUI()
{
    ImGui::Text(ICON_FA_COGS "  Attributes");
    ImGui::SliderFloat("Scale", &m_ScaleModifier, 0.0f, 20.0f);

    if (ImGui::CollapsingHeader("cbDefaultConstants")) {
        ImGui::SliderFloat("Dirt Factor", &m_cbDefaultConstants.DirtFactor, 0.0f, 1.0f);
        ImGui::SliderFloat("Wet Factor", &m_cbDefaultConstants.WetFactor, 0.0f, 1.0f);
        ImGui::SliderFloat("Lod Fade", &m_cbDefaultConstants.LodFade, 0.0f, 1.0f);
        ImGui::SliderFloat("Alpha Fade", &m_cbDefaultConstants.AlphaFade, 0.0f, 1.0f);
        ImGui::SliderFloat("Snow Factor", &m_cbDefaultConstants.SnowFactor, 0.0f, 1.0f);
        ImGui::ColorEdit4("Debug Color", glm::value_ptr(m_cbDefaultConstants.DebugColor));
        ImGui::ColorEdit4("Tint Color 1", glm::value_ptr(m_cbDefaultConstants.TintColor1));
        ImGui::ColorEdit4("Tint Color 2", glm::value_ptr(m_cbDefaultConstants.TintColor2));
        ImGui::ColorEdit4("Tint Color 3", glm::value_ptr(m_cbDefaultConstants.TintColor3));
        ImGui::SliderFloat("Alpha Test Ref", &m_cbDefaultConstants.AlphaTestRef, 0.0f, 1.0f);
    }

#if 0
        if (ImGui::CollapsingHeader("cbUnknownCB4")) {
            ImGui::SliderFloat4("cb4_0", glm::value_ptr(m_cbUnknownCB4.Unknown[0]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb4_1", glm::value_ptr(m_cbUnknownCB4.Unknown[1]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb4_2", glm::value_ptr(m_cbUnknownCB4.Unknown[2]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb4_3", glm::value_ptr(m_cbUnknownCB4.Unknown[3]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb4_4", glm::value_ptr(m_cbUnknownCB4.Unknown[4]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb4_5", glm::value_ptr(m_cbUnknownCB4.Unknown[5]), 0.0f, 1.0f);
        }

        if (ImGui::CollapsingHeader("cbUnknownCB6")) {
            ImGui::SliderFloat4("cb6_0", glm::value_ptr(m_cbUnknownCB6.Unknown[0]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_1", glm::value_ptr(m_cbUnknownCB6.Unknown[1]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_2", glm::value_ptr(m_cbUnknownCB6.Unknown[2]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_3", glm::value_ptr(m_cbUnknownCB6.Unknown[3]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_4", glm::value_ptr(m_cbUnknownCB6.Unknown[4]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_5", glm::value_ptr(m_cbUnknownCB6.Unknown[5]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_6", glm::value_ptr(m_cbUnknownCB6.Unknown[6]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_7", glm::value_ptr(m_cbUnknownCB6.Unknown[7]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_8", glm::value_ptr(m_cbUnknownCB6.Unknown[8]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_9", glm::value_ptr(m_cbUnknownCB6.Unknown[9]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_10", glm::value_ptr(m_cbUnknownCB6.Unknown[10]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_11", glm::value_ptr(m_cbUnknownCB6.Unknown[11]), 0.0f, 1.0f);
        }
#endif

    // Textures
    ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
    ImGui::Columns(3, nullptr, false);
    {
        for (auto i = 0; i < m_Textures.size(); ++i) {
            const auto name = "Texture-" + std::to_string(i);
            IRenderBlock::DrawUI_Texture(name, i);
        }
    }
    ImGui::EndColumns();
}
}; // namespace jc4
