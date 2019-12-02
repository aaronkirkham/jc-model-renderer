#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "renderblockcharacterskin.h"

#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/imgui/imgui_buttondropdown.h"
#include "graphics/renderer.h"
#include "graphics/shader_manager.h"

#include "game/render_block_factory.h"

namespace jc3
{
RenderBlockCharacterSkin::~RenderBlockCharacterSkin()
{
    // delete the skin batch lookup
    for (auto& batch : m_SkinBatches) {
        SAFE_DELETE(batch.m_BatchLookup);
    }

    // destroy shader constants
    Renderer::Get()->DestroyBuffer(m_VertexShaderConstants);
    Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants[0]);
    Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants[1]);
}

uint32_t RenderBlockCharacterSkin::GetTypeHash() const
{
    return RenderBlockFactory::RB_CHARACTERSKIN;
}

void RenderBlockCharacterSkin::Create()
{
    m_PixelShader = ShaderManager::Get()->GetPixelShader("characterskin");

    switch (m_Stride) {
            // 4bones1uv
        case 0: {
            m_VertexShader = ShaderManager::Get()->GetVertexShader("characterskin");

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
                                                                           "CharacterSkin VertexDecl (4bones1uv)");
            break;
        }

            // 4bones2uvs
        case 1: {
            m_VertexShader = ShaderManager::Get()->GetVertexShader("characterskin2uvs");

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
                                                                           "CharacterSkin VertexDecl (4bones2uvs)");
            break;
        }

            // 4bones3uvs
        case 2: {
            m_VertexShader = ShaderManager::Get()->GetVertexShader("characterskin3uvs");

            // create the element input desc
            // clang-format off
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   4,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   5,  DXGI_FORMAT_R16G16_SNORM,           0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   6,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };
            // clang-format on

            // create the vertex declaration
            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 6, m_VertexShader.get(),
                                                                           "CharacterSkin VertexDecl (4bones3uvs)");
            break;
        }

            // 8bones1uv
        case 3: {
            m_VertexShader = ShaderManager::Get()->GetVertexShader("characterskin8");

            // create the element input desc
            // clang-format off
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   2,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   3,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   4,  DXGI_FORMAT_R16G16_SNORM,           0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   6,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };
            // clang-format on

            // create the vertex declaration
            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 7, m_VertexShader.get(),
                                                                           "CharacterSkin VertexDecl (8bones1uv)");
            break;
        }

            // 8bones2uvs
        case 4: {
            m_VertexShader = ShaderManager::Get()->GetVertexShader("characterskin82uvs");

            // create the element input desc
            // clang-format off
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   2,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   3,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   4,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   6,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };
            // clang-format on

            // create the vertex declaration
            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 7, m_VertexShader.get(),
                                                                           "CharacterSkin VertexDecl (8bones3uvs)");
            break;
        }

            // 8bones3uvs
        case 5: {
            m_VertexShader = ShaderManager::Get()->GetVertexShader("characterskin83uvs");

            // create the element input desc
            // clang-format off
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   2,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   3,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   4,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   5,  DXGI_FORMAT_R16G16_SNORM,           0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   6,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };
            // clang-format on

            // create the vertex declaration
            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 8, m_VertexShader.get(),
                                                                           "CharacterSkin VertexDecl (8bones3uvs)");
            break;
        }
    }

    // create the constant buffer
    m_VertexShaderConstants = Renderer::Get()->CreateConstantBuffer(m_cbLocalConsts, "CharacterSkin LocalConsts");
    m_FragmentShaderConstants[0] =
        Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "CharacterSkin InstanceConsts");
    m_FragmentShaderConstants[1] =
        Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts, "CharacterSkin MaterialConsts");

    // identity the palette data
    for (int i = 0; i < 70; ++i) {
        m_cbLocalConsts.MatrixPalette[i] = glm::mat3x4(1);
    }

    //
    memset(&m_cbMaterialConsts._unknown, 0, sizeof(m_cbMaterialConsts._unknown));

    // create the sampler states
    {
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

        m_SamplerState = Renderer::Get()->CreateSamplerState(params, "CharacterSkin SamplerState");
    }
}

void RenderBlockCharacterSkin::Setup(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    IRenderBlock::Setup(context);

    const auto  flags = m_Block.m_Attributes.m_Flags;
    static auto world = glm::mat4(1);

    // local consts
    m_cbLocalConsts.World               = world;
    m_cbLocalConsts.WorldViewProjection = world * context->m_viewProjectionMatrix;
    m_cbLocalConsts.Scale               = glm::vec4{m_Block.m_Attributes.m_Scale};
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants, 1, m_cbLocalConsts);

    // instance consts
    context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[0], 1, m_cbInstanceConsts);

    // material consts
    m_cbMaterialConsts.m_DetailTilingFactorUV = m_Block.m_Attributes.m_DetailTilingFactorUV;
    m_cbMaterialConsts.m_DecalBlendFactors    = m_Block.m_Attributes.m_DecalBlendFactors;
    context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[1], 2, m_cbMaterialConsts);

    // set the textures
    for (int i = 0; i < 5; ++i) {
        BindTexture(i, m_SamplerState);
    }

    if (flags & USE_FEATURE_MAP) {
        BindTexture(7, 5, m_SamplerState);
    } else {
        context->m_Renderer->ClearTexture(5);
    }

    if (flags & USE_WRINKLE_MAP) {
        BindTexture(8, 6, m_SamplerState);
    } else {
        context->m_Renderer->ClearTexture(6);
    }

    // set the culling mode
    context->m_Renderer->SetCullMode((!(flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);

    // setup blending
    if (flags & ALPHA_MASK) {
        context->m_Renderer->SetBlendingEnabled(false);
        context->m_Renderer->SetAlphaTestEnabled(false);
    }

    context->m_Renderer->SetBlendingEnabled(true);
    context->m_Renderer->SetBlendingFunc(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA,
                                         D3D11_BLEND_ONE);
}

void RenderBlockCharacterSkin::DrawContextMenu()
{
    // clang-format off
    static std::array flag_labels = {
        "Disable Backface Culling",     "Use Wrinkle Map",              "Eight Bones",                  "",
        "Use Feature Map",              "Use Alpha Mask",               "",                             "",
    };
    // clang-format on

    ImGuiCustom::DropDownFlags(m_Block.m_Attributes.m_Flags, flag_labels);
}

void RenderBlockCharacterSkin::DrawUI()
{
    ImGui::Text(ICON_FA_COGS "  Attributes");
    ImGui::DragFloat("Scale", &m_Block.m_Attributes.m_Scale, 1.0f, 0.0f);
    ImGui::DragFloat2("Detail Tiling Factor UV", glm::value_ptr(m_Block.m_Attributes.m_DetailTilingFactorUV));
    ImGui::DragFloat2("Decal Blend Factors", glm::value_ptr(m_Block.m_Attributes.m_DecalBlendFactors));
    ImGui::DragFloat("Diffuse Roughness", &m_Block.m_Attributes.m_DiffuseRoughness, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Diffuse Wrap", &m_Block.m_Attributes.m_DiffuseWrap, 1.0f);
    ImGui::DragFloat("Dirt Factor", &m_Block.m_Attributes.m_DirtFactor, 0.01f, -1.0f, 1.0f);

    // Textures
    ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
    ImGui::Columns(3, nullptr, false);
    {
        DrawUI_Texture("DiffuseMap", 0);
        DrawUI_Texture("NormalMap", 1);
        DrawUI_Texture("PropertiesMap", 2);
        DrawUI_Texture("DetailDiffuseMap", 3);
        DrawUI_Texture("DetailNormalMap", 4);

        if (m_Block.m_Attributes.m_Flags & USE_FEATURE_MAP) {
            DrawUI_Texture("FeatureMap", 7);
        }

        if (m_Block.m_Attributes.m_Flags & USE_WRINKLE_MAP) {
            DrawUI_Texture("WrinkleMap", 8);
        }
    }
    ImGui::EndColumns();
}
}; // namespace jc3
