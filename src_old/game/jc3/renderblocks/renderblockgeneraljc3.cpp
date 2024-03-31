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
    using namespace jc::Vertex;

    // load shaders
    m_VertexShader = ShaderManager::Get()->GetVertexShader("generaljc3");
    m_PixelShader  = ShaderManager::Get()->GetPixelShader("generaljc3");

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
                                                                       "GeneralJC3 VertexDecl (unpacked)");
    } else {
        // clang-format off
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };
        // clang-format on

        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(),
                                                                       "GeneralJC3 VertexDecl (packed)");
    }

    // create the constant buffer
    m_VertexShaderConstants[0] =
        Renderer::Get()->CreateConstantBuffer(m_cbVertexInstanceConsts, "GeneralJC3 VertexInstanceConsts");
    m_VertexShaderConstants[1] =
        Renderer::Get()->CreateConstantBuffer(m_cbVertexMaterialConsts, "GeneralJC3 VertexMaterialConsts");
    m_FragmentShaderConstants[0] =
        Renderer::Get()->CreateConstantBuffer(m_cbFragmentMaterialConsts, "GeneralJC3 FragmentMaterialConsts");
    m_FragmentShaderConstants[1] =
        Renderer::Get()->CreateConstantBuffer(m_cbFragmentInstanceConsts, "GeneralJC3 FragmentInstanceConsts");
}

void RenderBlockGeneralJC3::Setup(RenderContext_t* context)
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
        m_cbVertexInstanceConsts.viewProjection          = context->m_viewProjectionMatrix;
        m_cbVertexInstanceConsts.world                   = world;
        m_cbVertexInstanceConsts.colour                  = glm::vec4(1, 1, 1, 1);
        m_cbVertexInstanceConsts._thing                  = glm::vec4(0, 1, 2, 1);
        m_cbVertexMaterialConsts.DepthBias               = m_Block.m_Attributes.m_DepthBias;
        m_cbVertexMaterialConsts.EmissiveStartFadeDistSq = m_Block.m_Attributes.m_StartFadeOutDistanceEmissiveSq;
        m_cbVertexMaterialConsts._unknown                = 0;
        m_cbVertexMaterialConsts._unknown2               = 0;
        m_cbVertexMaterialConsts.uv0Extent               = m_Block.m_Attributes.m_Packed.m_UV0Extent;
        m_cbVertexMaterialConsts.uv1Extent               = m_Block.m_Attributes.m_Packed.m_UV1Extent;

        // set fragment shader constants
        m_cbFragmentMaterialConsts.specularGloss    = m_Block.m_Attributes.m_SpecularGloss;
        m_cbFragmentMaterialConsts.reflectivity     = m_Block.m_Attributes.m_Reflectivity;
        m_cbFragmentMaterialConsts.specularFresnel  = m_Block.m_Attributes.m_SpecularFresnel;
        m_cbFragmentMaterialConsts.diffuseModulator = m_Block.m_Attributes.m_DiffuseModulator;
        m_cbFragmentInstanceConsts.colour           = glm::vec4(1, 1, 1, 0);
    }

    // set the textures
    for (int i = 0; i < m_Textures.size(); ++i) {
        BindTexture(i, m_SamplerState);
    }

    // set the constant buffers
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 1, m_cbVertexInstanceConsts);
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 2, m_cbVertexMaterialConsts);
    context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[0], 1, m_cbFragmentMaterialConsts);
    context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[1], 2, m_cbFragmentInstanceConsts);

    // set the culling mode
    context->m_Renderer->SetCullMode((!(m_Block.m_Attributes.m_Flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK
                                                                                                  : D3D11_CULL_NONE);

    // if we are using packed vertices, set the 2nd vertex buffer
    if (m_Block.m_Attributes.m_Packed.m_Format == PACKED_FORMAT_INT16) {
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

    ImGuiCustom::DropDownFlags(m_Block.m_Attributes.m_Flags, flag_labels);
}

void RenderBlockGeneralJC3::DrawUI()
{
    ImGui::Text(ICON_FA_COGS "  Attributes");
    ImGui::DragFloat("Scale", &m_Block.m_Attributes.m_Packed.m_Scale, 1.0f, 0.0f);
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
        DrawUI_Texture("DiffuseMap", 0);
        DrawUI_Texture("NormalMap", 1);
        DrawUI_Texture("PropertiesMap", 2);
        DrawUI_Texture("AOBlendMap", 3);
    }
    ImGui::EndColumns();
}
}; // namespace jc3
