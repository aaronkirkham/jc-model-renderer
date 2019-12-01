#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "renderblockwindow.h"

#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/imgui/imgui_buttondropdown.h"
#include "graphics/imgui/imgui_disabled.h"
#include "graphics/renderer.h"
#include "graphics/shader_manager.h"

#include "game/render_block_factory.h"

namespace jc3
{
RenderBlockWindow::~RenderBlockWindow()
{
    Renderer::Get()->DestroyBuffer(m_VertexShaderConstants);
    Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants);
    Renderer::Get()->DestroySamplerState(_test);
}

uint32_t RenderBlockWindow::GetTypeHash() const
{
    return RenderBlockFactory::RB_WINDOW;
}

void RenderBlockWindow::Create()
{
    // load shaders
    m_VertexShader = ShaderManager::Get()->GetVertexShader("window");
    m_PixelShader  = ShaderManager::Get()->GetPixelShader("window");

    // create the element input desc
    // clang-format off
    D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
        { "POSITION",   0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        { "TEXCOORD",   0,  DXGI_FORMAT_R32G32B32A32_FLOAT,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
    };
    // clang-format on

    // create the vertex declaration
    m_VertexDeclaration =
        Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(), "RenderBlockWindow");

    // create the constant buffers
    m_VertexShaderConstants =
        Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "RenderBlockWindow InstanceConsts");
    m_FragmentShaderConstants =
        Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts, 3, "RenderBlockWindow MaterialConsts");

    // create the sampler states
    {
        D3D11_SAMPLER_DESC params{};
        params.Filter         = D3D11_FILTER_ANISOTROPIC;
        params.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
        params.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
        params.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
        params.MipLODBias     = 0.0f;
        params.MaxAnisotropy  = 2;
        params.ComparisonFunc = D3D11_COMPARISON_NEVER;
        params.MinLOD         = 0.0f;
        params.MaxLOD         = 13.0f;

        m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockWindow");

        // TODO: find out what the game is actually doing
        // couldn't find anything related to slot 15 in the RenderBlockWindow stuff
        params.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
        _test         = Renderer::Get()->CreateSamplerState(params, "RenderBlockWindow _test");
    }
}

void RenderBlockWindow::Setup(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    IRenderBlock::Setup(context);

    // setup the constant buffer
    {
        static const auto world = glm::scale(glm::mat4(1), glm::vec3{m_ScaleModifier});

        // set vertex shader constants
        m_cbInstanceConsts.World               = world;
        m_cbInstanceConsts.WorldViewProjection = world * context->m_viewProjectionMatrix;

        // set fragment shaders constants
        m_cbMaterialConsts.SpecGloss       = m_Block.m_Attributes.m_SpecGloss;
        m_cbMaterialConsts.SpecFresnel     = m_Block.m_Attributes.m_SpecFresnel;
        m_cbMaterialConsts.DiffuseRougness = m_Block.m_Attributes.m_DiffuseRougness;
        m_cbMaterialConsts.TintPower       = m_Block.m_Attributes.m_TintPower;
        m_cbMaterialConsts.MinAlpha        = m_Block.m_Attributes.m_MinAlpha;
        m_cbMaterialConsts.UVScale         = m_Block.m_Attributes.m_UVScale;
    }

    // set the textures
    IRenderBlock::BindTexture(0, m_SamplerState);

    if (!(m_Block.m_Attributes.m_Flags & SIMPLE)) {
        IRenderBlock::BindTexture(1, 2, m_SamplerState);
        IRenderBlock::BindTexture(2, 3, m_SamplerState);
    }

    // TODO: damage

    // set the sampler state
    // context->m_Renderer->SetSamplerState(m_SamplerState, 0);
    // context->m_Renderer->SetSamplerState(_test, 15);

#if 0
        // set lighting textures
        const auto gbuffer_diffuse = context->m_Renderer->GetGBufferSRV(0);
        context->m_DeviceContext->PSSetShaderResources(15, 1, &gbuffer_diffuse);
#endif

    // enable blending
    context->m_Renderer->SetBlendingEnabled(true);
    context->m_Renderer->SetBlendingFunc(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_DEST_ALPHA, D3D11_BLEND_INV_SRC_ALPHA,
                                         D3D11_BLEND_ONE);
    context->m_Renderer->SetBlendingEq(D3D11_BLEND_OP_ADD, D3D11_BLEND_OP_ADD);

    // enable depth
    context->m_Renderer->SetDepthEnabled(true);

    // set the constant buffers
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants, 1, m_cbInstanceConsts);
    context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants, 1, m_cbMaterialConsts);

    // set the culling mode
    if (m_Block.m_Attributes.m_Flags & ENABLE_BACKFACE_CULLING) {
        context->m_Renderer->SetCullMode(D3D11_CULL_BACK);
    }
}

void RenderBlockWindow::Draw(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    if (!(m_Block.m_Attributes.m_Flags & ENABLE_BACKFACE_CULLING)) {
        context->m_Renderer->SetCullMode(D3D11_CULL_FRONT);
        IRenderBlock::Draw(context);
        context->m_Renderer->SetCullMode(D3D11_CULL_BACK);
    }

    IRenderBlock::Draw(context);
}

void RenderBlockWindow::DrawContextMenu()
{
    // clang-format off
    static std::array flag_labels = {
        "Enable Backface Culling",      "Is Simple",                     "",                             "",
        "",                             "",                             "",                             ""
    };
    // clang-format on

    ImGuiCustom::DropDownFlags(m_Block.m_Attributes.m_Flags, flag_labels);
}

void RenderBlockWindow::DrawUI()
{
    ImGui::Text(ICON_FA_COGS "  Attributes");
    ImGui::DragFloat("Specular Gloss", &m_Block.m_Attributes.m_SpecGloss, 0, 1);
    ImGui::DragFloat("Specular Fresnel", &m_Block.m_Attributes.m_SpecFresnel, 0, 1);
    ImGui::DragFloat("Diffuse Rougness", &m_Block.m_Attributes.m_DiffuseRougness, 0, 1);
    ImGui::DragFloat("Tint Power", &m_Block.m_Attributes.m_TintPower, 0, 10);
    ImGui::DragFloat("Min Alpha", &m_Block.m_Attributes.m_MinAlpha, 0, 1);
    ImGui::DragFloat("UV Scale", &m_Block.m_Attributes.m_UVScale, 0, 1);
    ImGui::DragFloat("Alpha", &m_cbMaterialConsts.Alpha, 0, 1);

    // Textures
    ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
    ImGui::Columns(3, nullptr, false);
    {
        IRenderBlock::DrawUI_Texture("DiffuseMap", 0);

        if (!(m_Block.m_Attributes.m_Flags & SIMPLE)) {
            IRenderBlock::DrawUI_Texture("NormalMap", 1);
            IRenderBlock::DrawUI_Texture("PropertyMap", 2);
        }

        // damage
        IRenderBlock::DrawUI_Texture("DamagePointNormalMap", 3);
        IRenderBlock::DrawUI_Texture("DamagePointPropertyMap", 4);
    }
    ImGui::EndColumns();
}
}; // namespace jc3
