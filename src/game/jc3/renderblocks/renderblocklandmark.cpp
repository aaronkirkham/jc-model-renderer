#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "renderblocklandmark.h"

#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/imgui/imgui_buttondropdown.h"
#include "graphics/imgui/imgui_disabled.h"
#include "graphics/renderer.h"
#include "graphics/shader_manager.h"

#include "game/render_block_factory.h"

namespace jc3
{
RenderBlockLandmark::~RenderBlockLandmark()
{
    Renderer::Get()->DestroyBuffer(m_VertexBufferData);

    for (auto& fsc : m_VertexShaderConstants) {
        Renderer::Get()->DestroyBuffer(fsc);
    }

    for (auto& fsc : m_FragmentShaderConstants) {
        Renderer::Get()->DestroyBuffer(fsc);
    }
}

uint32_t RenderBlockLandmark::GetTypeHash() const
{
    return RenderBlockFactory::RB_LANDMARK;
}

void RenderBlockLandmark::Create()
{
    // load shaders
    m_VertexShader = ShaderManager::Get()->GetVertexShader("landmark");
    m_PixelShader  = ShaderManager::Get()->GetPixelShader("landmark");

    // create the input desc
    // clang-format off
    D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
        { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SINT,      0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        { "TEXCOORD",   0,  DXGI_FORMAT_R16G16_SNORM,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        { "TEXCOORD",   1,  DXGI_FORMAT_R32_FLOAT,              1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
    };
    // clang-format on

    m_VertexDeclaration =
        Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(), "RenderBlockLandmark");

    // create the constant buffer
    m_VertexShaderConstants[0] =
        Renderer::Get()->CreateConstantBuffer(m_cbVertexInstanceConsts, "RenderBlockLandmark cbVertexInstanceConsts");
    m_VertexShaderConstants[1] =
        Renderer::Get()->CreateConstantBuffer(m_cbVertexMaterialConsts, "RenderBlockLandmark cbVertexMaterialConsts");
    m_FragmentShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(
        m_cbFragmentMaterialConsts, 3, "RenderBlockLandmark cbFragmentMaterialConsts");
    m_FragmentShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(
        m_cbFragmentInstanceConsts, "RenderBlockLandmark cbFragmentInstanceConsts");
}

void RenderBlockLandmark::Setup(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    IRenderBlock::Setup(context);

    // setup the constant buffer
    {
        const auto world = glm::scale(glm::mat4(1), glm::vec3{m_Block.m_Attributes.m_Packed.m_Scale});

        // set vertex shader constants
        m_cbVertexInstanceConsts.worldViewProjection     = context->m_viewProjectionMatrix * world;
        m_cbVertexInstanceConsts.world                   = world;
        m_cbVertexInstanceConsts.colour                  = glm::vec4(0, 0, 0, 1);
        m_cbVertexMaterialConsts.DepthBias               = m_Block.m_Attributes.m_DepthBias;
        m_cbVertexMaterialConsts.EmissiveStartFadeDistSq = m_Block.m_Attributes.m_StartFadeOutDistanceEmissiveSq;
        m_cbVertexMaterialConsts.UVScale                 = m_Block.m_Attributes.m_Packed.m_UV0Extent;

        // set fragment shader constants
        m_cbFragmentMaterialConsts.SpecularGloss    = m_Block.m_Attributes.m_SpecularGloss;
        m_cbFragmentMaterialConsts.Reflectivity     = m_Block.m_Attributes.m_Reflectivity;
        m_cbFragmentMaterialConsts.Emissive         = m_Block.m_Attributes.m_Emmissive;
        m_cbFragmentMaterialConsts.SpecularFresnel  = m_Block.m_Attributes.m_SpecularFresnel;
        m_cbFragmentMaterialConsts.DiffuseModulator = m_Block.m_Attributes.m_DiffuseModulator;
        m_cbFragmentMaterialConsts.DiffuseWrap      = m_Block.m_Attributes.m_DiffuseWrap;
        m_cbFragmentMaterialConsts.BackLight        = m_Block.m_Attributes.m_BackLight;
        m_cbFragmentInstanceConsts.DebugColor       = glm::vec4(1, 1, 1, 0);
    }

    // set the textures
    for (int i = 0; i < m_Textures.size(); ++i) {
        IRenderBlock::BindTexture(i, m_SamplerState);
    }

    // set the vertex shader resource
    // TODO: NormalTableMap slot 0

    // set the constant buffers
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 1, m_cbVertexInstanceConsts);
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 2, m_cbVertexMaterialConsts);
    context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[0], 1, m_cbFragmentMaterialConsts);
    context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[1], 2, m_cbFragmentInstanceConsts);

    // set the 2nd vertex buffer
    context->m_Renderer->SetVertexStream(m_VertexBufferData, 1);
}

void RenderBlockLandmark::DrawContextMenu()
{
    // clang-format off
    static std::array flag_labels = {
        "",                             "",                             "",                             "",
        "",                             "",                             "",                             ""
    };
    // clang-format on

    ImGuiCustom::DropDownFlags(m_Block.m_Attributes.m_Flags, flag_labels);
}

void RenderBlockLandmark::DrawUI()
{
    ImGui::Text(ICON_FA_COGS "  Attributes");
    ImGui::DragFloat("Scale", &m_Block.m_Attributes.m_Packed.m_Scale, 0.1f, 10.0f);
    ImGui::DragFloat("Depth Bias", &m_Block.m_Attributes.m_DepthBias, 0.0f, 10.0f);
    ImGui::DragFloat("Specular Gloss", &m_Block.m_Attributes.m_SpecularGloss, 0.0f, 10.0f);
    ImGui::DragFloat("Reflectivity", &m_Block.m_Attributes.m_Reflectivity, 0.0f, 10.0f);
    ImGui::DragFloat("Emissive", &m_Block.m_Attributes.m_Emmissive, 0.0f, 10.0f);
    ImGui::DragFloat("Diffuse Wrap", &m_Block.m_Attributes.m_DiffuseWrap, 0.0f, 10.0f);
    ImGui::DragFloat("Specular Fresnel", &m_Block.m_Attributes.m_SpecularFresnel, 0.0f, 10.0f);
    ImGui::ColorEdit3("Diffuse Modulator", glm::value_ptr(m_Block.m_Attributes.m_DiffuseModulator));
    ImGui::DragFloat("Back Light", &m_Block.m_Attributes.m_BackLight, 0.0f, 10.0f);
    ImGui::DragFloat("Unknown #2", &m_Block.m_Attributes._unknown2, 0.0f, 10.0f);
    ImGui::DragFloat("Fade Out Distance Emissive", &m_Block.m_Attributes.m_StartFadeOutDistanceEmissiveSq);

    // Textures
    ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
    ImGui::Columns(3, nullptr, false);
    {
        IRenderBlock::DrawUI_Texture("DiffuseMap", 0);
        IRenderBlock::DrawUI_Texture("PropertiesMap", 2);
    }
    ImGui::EndColumns();
}
}; // namespace jc3
