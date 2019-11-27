#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "renderblockcarlight.h"

#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/imgui/imgui_buttondropdown.h"
#include "graphics/renderer.h"
#include "graphics/shader_manager.h"

#include "game/render_block_factory.h"

namespace jc3
{
RenderBlockCarLight::~RenderBlockCarLight()
{
    // delete the skin batch lookup
    for (auto& batch : m_SkinBatches) {
        SAFE_DELETE(batch.m_BatchLookup);
    }

    // destroy shader constants
    Renderer::Get()->DestroyBuffer(m_VertexBufferData);
    Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants);

    for (auto& vsc : m_VertexShaderConstants) {
        Renderer::Get()->DestroyBuffer(vsc);
    }
}

uint32_t RenderBlockCarLight::GetTypeHash() const
{
    return RenderBlockFactory::RB_BUILDINGJC3;
}

void RenderBlockCarLight::Create()
{
    // load shaders
    m_VertexShader = ShaderManager::Get()->GetVertexShader("carpaintmm");
    m_PixelShader  = ShaderManager::Get()->GetPixelShader("carlight");

    // create the element input desc
    // clang-format off
    D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
        { "POSITION",   0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        { "TEXCOORD",   1,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        { "TEXCOORD",   2,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        { "TEXCOORD",   3,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        { "TEXCOORD",   4,  DXGI_FORMAT_R32G32_FLOAT,           2,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
    };
    // clang-format on

    // create the vertex declaration
    m_VertexDeclaration =
        Renderer::Get()->CreateVertexDeclaration(inputDesc, 5, m_VertexShader.get(), "RenderBlockCarLight");

    // create the constant buffers
    m_VertexShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbRBIInfo, "RenderBlockCarLight RBIInfo");
    m_VertexShaderConstants[1] =
        Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "RenderBlockCarLight InstanceConsts");
    m_VertexShaderConstants[2] =
        Renderer::Get()->CreateConstantBuffer(m_cbDeformConsts, "RenderBlockCarLight DeformConsts");
    m_FragmentShaderConstants =
        Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts, "RenderBlockCarLight MaterialConsts");

    // create the sampler states
    {
        D3D11_SAMPLER_DESC params{};
        params.Filter         = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        params.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
        params.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
        params.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
        params.MipLODBias     = 0.0f;
        params.MaxAnisotropy  = 1;
        params.ComparisonFunc = D3D11_COMPARISON_NEVER;
        params.MinLOD         = 0.0f;
        params.MaxLOD         = 13.0f;

        m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockCarLight");
    }

    //
    memset(&m_cbDeformConsts, 0, sizeof(m_cbDeformConsts));
    memset(&m_cbMaterialConsts, 0, sizeof(m_cbMaterialConsts));
}

void RenderBlockCarLight::Setup(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    IRenderBlock::Setup(context);

    // setup the constant buffer
    {
        static const auto world = glm::mat4(1);

        // set vertex shader constants
        m_cbRBIInfo.ModelWorldMatrix = glm::scale(world, {1, 1, 1});

        // set fragment shaders constants
        m_cbMaterialConsts.SpecularGloss    = m_Block.attributes.specularGloss;
        m_cbMaterialConsts.Reflectivity     = m_Block.attributes.reflectivity;
        m_cbMaterialConsts.SpecularFresnel  = m_Block.attributes.specularFresnel;
        m_cbMaterialConsts.DiffuseModulator = m_Block.attributes.diffuseModulator;
        m_cbMaterialConsts.TilingUV         = {m_Block.attributes.tilingUV, 1, 0};
    }

    // set the textures
    for (int i = 0; i < m_Textures.size(); ++i) {
        IRenderBlock::BindTexture(i, m_SamplerState);
    }

    // set the constant buffers
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 6, m_cbRBIInfo);
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 1, m_cbInstanceConsts);
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[2], 2, m_cbDeformConsts);
    context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants, 2, m_cbMaterialConsts);

    // set the culling mode
    context->m_Renderer->SetCullMode((!(m_Block.attributes.flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK
                                                                                              : D3D11_CULL_NONE);

    // set the 2nd and 3rd vertex buffers
    context->m_Renderer->SetVertexStream(m_VertexBufferData, 1);
    context->m_Renderer->SetVertexStream(m_VertexBufferData, 2);
}

void RenderBlockCarLight::DrawContextMenu()
{
    // clang-format off
    static std::array flag_labels = {
        "Disable Backface Culling",     "",                             "Is Deformable",                "Is Skinned",
        "",                             "",                             "",                             ""
    };
    // clang-format on

    ImGuiCustom::DropDownFlags(m_Block.attributes.flags, flag_labels);
}

void RenderBlockCarLight::DrawUI()
{
    ImGui::Text(ICON_FA_COGS "  Attributes");

    ImGui::SliderFloat("Specular Gloss", &m_Block.attributes.specularGloss, 0, 1);
    ImGui::SliderFloat("Reflectivity", &m_Block.attributes.reflectivity, 0, 1);
    ImGui::SliderFloat("Specular Fresnel", &m_Block.attributes.specularFresnel, 0, 1);
    ImGui::ColorEdit3("Diffuse Modulator", glm::value_ptr(m_Block.attributes.diffuseModulator));
    ImGui::SliderFloat2("Tiling", glm::value_ptr(m_Block.attributes.tilingUV), 0, 10);

    // Textures
    ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
    ImGui::Columns(3, nullptr, false);
    {
        IRenderBlock::DrawUI_Texture("DiffuseMap", 0);
        IRenderBlock::DrawUI_Texture("NormalMap", 1);
        IRenderBlock::DrawUI_Texture("PropertyMap", 2);
        // 3 unknown
        IRenderBlock::DrawUI_Texture("NormalDetailMap", 4);
        IRenderBlock::DrawUI_Texture("EmissiveMap", 5);
    }
    ImGui::EndColumns();
}
}; // namespace jc3
