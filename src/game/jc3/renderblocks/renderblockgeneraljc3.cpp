#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "renderblockgeneraljc3.h"

#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/imgui/imgui_buttondropdown.h"
#include "graphics/renderer.h"
#include "graphics/shader_manager.h"

#include "game/render_block_factory.h"

namespace jc3
{
RenderBlockGeneralJC3::~RenderBlockGeneralJC3()
{
    Renderer::Get()->DestroyBuffer(m_VertexBufferData);

    for (auto& vsc : m_VertexShaderConstants) {
        Renderer::Get()->DestroyBuffer(vsc);
    }

    for (auto& fsc : m_FragmentShaderConstants) {
        Renderer::Get()->DestroyBuffer(fsc);
    }
}

uint32_t RenderBlockGeneralJC3::GetTypeHash() const
{
    return RenderBlockFactory::RB_GENERALJC3;
}

void RenderBlockGeneralJC3::Create()
{
    // load shaders
    m_VertexShader = ShaderManager::Get()->GetVertexShader("generaljc3");
    m_PixelShader  = ShaderManager::Get()->GetPixelShader("generaljc3");

    // create the input desc
    if (m_Block.attributes.packed.format != 1) {
        // clang-format off
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   0,  DXGI_FORMAT_R32G32B32A32_FLOAT,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };
        // clang-format on

        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(),
                                                                       "RenderBlockGeneralJC3 (unpacked)");
    } else {
        // clang-format off
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };
        // clang-format on

        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(),
                                                                       "RenderBlockGeneralJC3 (packed)");
    }

    // create the constant buffer
    m_VertexShaderConstants[0] =
        Renderer::Get()->CreateConstantBuffer(m_cbVertexInstanceConsts, "RenderBlockGeneralJC3 cbVertexInstanceConsts");
    m_VertexShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(
        m_cbVertexMaterialConsts, "RenderBlockGeneralJC3 m_cbVertexMaterialConsts");
    m_FragmentShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(
        m_cbFragmentMaterialConsts, "RenderBlockGeneralJC3 cbFragmentMaterialConsts");
    m_FragmentShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(
        m_cbFragmentInstanceConsts, "RenderBlockGeneralJC3 cbFragmentInstanceConsts");
}

void RenderBlockGeneralJC3::Setup(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    IRenderBlock::Setup(context);

    // setup the constant buffer
    {
        const auto scale = m_Block.attributes.packed.scale * m_ScaleModifier;

        // set vertex shader constants
        m_cbVertexInstanceConsts.viewProjection          = context->m_viewProjectionMatrix;
        m_cbVertexInstanceConsts.world                   = glm::scale(glm::mat4(1), {scale, scale, scale});
        m_cbVertexInstanceConsts.colour                  = glm::vec4(1, 1, 1, 1);
        m_cbVertexInstanceConsts._thing                  = glm::vec4(0, 1, 2, 1);
        m_cbVertexMaterialConsts.DepthBias               = m_Block.attributes.depthBias;
        m_cbVertexMaterialConsts.EmissiveStartFadeDistSq = m_Block.attributes.startFadeOutDistanceEmissiveSq;
        m_cbVertexMaterialConsts._unknown                = 0;
        m_cbVertexMaterialConsts._unknown2               = 0;
        m_cbVertexMaterialConsts.uv0Extent               = m_Block.attributes.packed.uv0Extent;
        m_cbVertexMaterialConsts.uv1Extent               = m_Block.attributes.packed.uv1Extent;

        // set fragment shader constants
        m_cbFragmentMaterialConsts.specularGloss    = m_Block.attributes.specularGloss;
        m_cbFragmentMaterialConsts.reflectivity     = m_Block.attributes.reflectivity;
        m_cbFragmentMaterialConsts.specularFresnel  = m_Block.attributes.specularFresnel;
        m_cbFragmentMaterialConsts.diffuseModulator = m_Block.attributes.diffuseModulator;
        m_cbFragmentInstanceConsts.colour           = glm::vec4(1, 1, 1, 0);
    }

    // set the textures
    for (int i = 0; i < m_Textures.size(); ++i) {
        IRenderBlock::BindTexture(i, m_SamplerState);
    }

    // set the constant buffers
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 1, m_cbVertexInstanceConsts);
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 2, m_cbVertexMaterialConsts);
    context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[0], 1, m_cbFragmentMaterialConsts);
    context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[1], 2, m_cbFragmentInstanceConsts);

    // set the culling mode
    context->m_Renderer->SetCullMode((!(m_Block.attributes.flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK
                                                                                              : D3D11_CULL_NONE);

    // if we are using packed vertices, set the 2nd vertex buffer
    if (m_Block.attributes.packed.format == 1) {
        context->m_Renderer->SetVertexStream(m_VertexBufferData, 1);
    }
}

void RenderBlockGeneralJC3::DrawContextMenu()
{
    // clang-format off
    static std::array flag_labels = {
        "Disable Backface Culling",     "Transparency Alpha Blending",  "Dynamic Emissive",             "",
        "",                             "",                             "",                             ""
    };
    // clang-format on

    ImGuiCustom::DropDownFlags(m_Block.attributes.flags, flag_labels);
}

void RenderBlockGeneralJC3::DrawUI()
{
    ImGui::Text(ICON_FA_COGS "  Attributes");

    ImGui::SliderFloat("Scale", &m_ScaleModifier, 0.1f, 10.0f);
    ImGui::SliderFloat("Depth Bias", &m_Block.attributes.depthBias, 0.0f, 10.0f);
    ImGui::SliderFloat("Specular Gloss", &m_Block.attributes.specularGloss, 0.0f, 10.0f);
    ImGui::SliderFloat("Reflectivity", &m_Block.attributes.reflectivity, 0.0f, 10.0f);
    ImGui::SliderFloat("Emissive", &m_Block.attributes.emmissive, 0.0f, 10.0f);
    ImGui::SliderFloat("Diffuse Wrap", &m_Block.attributes.diffuseWrap, 0.0f, 10.0f);
    ImGui::SliderFloat("Specular Fresnel", &m_Block.attributes.specularFresnel, 0.0f, 10.0f);
    ImGui::ColorEdit3("Diffuse Modulator", glm::value_ptr(m_Block.attributes.diffuseModulator));
    ImGui::SliderFloat("Back Light", &m_Block.attributes.backLight, 0.0f, 10.0f);
    ImGui::SliderFloat("Unknown #2", &m_Block.attributes._unknown2, 0.0f, 10.0f);
    ImGui::InputFloat("Fade Out Distance Emissive", &m_Block.attributes.startFadeOutDistanceEmissiveSq);

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
