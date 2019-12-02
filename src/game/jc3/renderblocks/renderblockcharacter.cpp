#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "renderblockcharacter.h"

#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/imgui/imgui_buttondropdown.h"
#include "graphics/renderer.h"
#include "graphics/shader_manager.h"

#include "game/render_block_factory.h"

namespace jc3
{
RenderBlockCharacter::~RenderBlockCharacter()
{
    // delete the skin batch lookup
    for (auto& batch : m_SkinBatches) {
        SAFE_DELETE(batch.m_BatchLookup);
    }

    // destroy shader constants
    Renderer::Get()->DestroyBuffer(m_VertexShaderConstants);
    Renderer::Get()->DestroyBuffer(m_PixelShaderConstants[0]);
    Renderer::Get()->DestroyBuffer(m_PixelShaderConstants[1]);
}

uint32_t RenderBlockCharacter::GetTypeHash() const
{
    return RenderBlockFactory::RB_CHARACTER;
}

void RenderBlockCharacter::CreateBuffers(std::vector<uint8_t>* vertex_buffer, std::vector<uint8_t>* index_buffer)
{
    // create vertex buffer
    m_VertexBuffer = Renderer::Get()->CreateBuffer(
        vertex_buffer->data(), (vertex_buffer->size() / sizeof(jc::Vertex::RenderBlockCharacter::Packed4Bones1UV)),
        sizeof(jc::Vertex::RenderBlockCharacter::Packed4Bones1UV), VERTEX_BUFFER);

    // create index buffer
    m_IndexBuffer = Renderer::Get()->CreateBuffer(index_buffer->data(), (index_buffer->size() / sizeof(uint16_t)),
                                                  sizeof(uint16_t), INDEX_BUFFER);

    m_Block                      = {};
    m_Block.m_Attributes.m_Scale = 1.0f;

    Create();
}

void RenderBlockCharacter::Create()
{
    m_PixelShader = ShaderManager::Get()->GetPixelShader(
        (m_Block.m_Attributes.m_Flags & BODY_PART) != HAIR ? "character" : "characterhair_msk");

    switch (m_Stride) {
            // Packed4Bones1UV
        case 0: {
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
                                                                           "Character VertexDecl (Packed4Bones1UV)");
            break;
        }

            // Packed4Bones2UVs
        case 1: {
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
                                                                           "Character VertexDecl (Packed4Bones2UVs)");
            break;
        }

            // Packed4Bones3UVs
        case 2: {
            m_VertexShader = ShaderManager::Get()->GetVertexShader("character3uvs");

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
                                                                           "Character VertexDecl (Packed4Bones3UVs)");
            break;
        }

            // Packed8Bones1UV
        case 3: {
            m_VertexShader = ShaderManager::Get()->GetVertexShader("character8");

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
                                                                           "Character VertexDecl (Packed8Bones1UV)");
            break;
        }

            // Packed8Bones2UVs
        case 4: {
            m_VertexShader = ShaderManager::Get()->GetVertexShader("character82uvs");

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
                                                                           "Character VertexDecl (Packed8Bones2UVs)");
            break;
        }

            // Packed8Bones3UVs
        case 5: {
            m_VertexShader = ShaderManager::Get()->GetVertexShader("character83uvs");

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
                                                                           "Character VertexDecl (Packed8Bones3UVs)");
            break;
        }
    }

    // create the constant buffer
    m_VertexShaderConstants   = Renderer::Get()->CreateConstantBuffer(m_cbLocalConsts, "Character LocalConsts");
    m_PixelShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "Character InstanceConsts");
    m_PixelShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts, "Character MaterialConsts");

    // reset fragment shader material consts
    memset(&m_cbMaterialConsts, 0, sizeof(m_cbMaterialConsts));

    // identity the palette data
    for (int i = 0; i < 70; ++i) {
        m_cbLocalConsts.MatrixPalette[i] = glm::mat3x4(1);
    }

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

        m_SamplerState = Renderer::Get()->CreateSamplerState(params, "Character SamplerState");
    }
}

void RenderBlockCharacter::Setup(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    IRenderBlock::Setup(context);

    const auto  flags = m_Block.m_Attributes.m_Flags;
    static auto world = glm::mat4(1);

    // set the textures
    IRenderBlock::BindTexture(0, m_SamplerState);

    // local consts
    m_cbLocalConsts.World               = world;
    m_cbLocalConsts.WorldViewProjection = world * context->m_viewProjectionMatrix;
    m_cbLocalConsts.Scale               = glm::vec4{m_Block.m_Attributes.m_Scale};
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants, 1, m_cbLocalConsts);

    // instance consts
    context->m_Renderer->SetPixelShaderConstants(m_PixelShaderConstants[0], 1, m_cbInstanceConsts);

    // material consts
    {
        if ((flags & BODY_PART) == EYES) {
            m_cbMaterialConsts.DetailTilingFactorUV = {600.0f, 1.0f};
            m_cbMaterialConsts.DecalBlendFactors    = {1.0f, 0.0f};
        } else {
            m_cbMaterialConsts.DetailTilingFactorUV = m_Block.m_Attributes.m_DetailTilingFactorUV;
            m_cbMaterialConsts.DecalBlendFactors    = m_Block.m_Attributes.m_DecalBlendFactors;
        }

        m_cbMaterialConsts._unknown         = m_Block.m_Attributes._unknown;
        m_cbMaterialConsts.SpecularGloss    = m_Block.m_Attributes.m_SpecularGloss;
        m_cbMaterialConsts.Transmission     = m_Block.m_Attributes.m_Transmission;
        m_cbMaterialConsts.SpecularFresnel  = m_Block.m_Attributes.m_SpecularFresnel;
        m_cbMaterialConsts.DiffuseWrap      = (flags & BODY_PART) == HAIR ? m_Block.m_Attributes.m_DiffuseWrap : 0.0f;
        m_cbMaterialConsts.DiffuseRoughness = m_Block.m_Attributes.m_DiffuseRoughness;
        m_cbMaterialConsts.DirtFactor       = m_Block.m_Attributes.m_DirtFactor;
        m_cbMaterialConsts.Emissive         = m_Block.m_Attributes.m_Emissive;

        context->m_Renderer->SetPixelShaderConstants(m_PixelShaderConstants[1], 2, m_cbMaterialConsts);
    }

    // set the culling mode
    context->m_Renderer->SetCullMode((!(flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);

    // toggle alpha mask
    if (flags & ALPHA_MASK) {
        context->m_Renderer->SetAlphaTestEnabled(true);
        context->m_Renderer->SetBlendingEnabled(false);
    } else {
        context->m_Renderer->SetAlphaTestEnabled(false);
    }

    // setup textures and blending
    switch (flags & BODY_PART) {
        case GEAR: {
            IRenderBlock::BindTexture(1, m_SamplerState);
            IRenderBlock::BindTexture(2, m_SamplerState);
            IRenderBlock::BindTexture(3, m_SamplerState);
            IRenderBlock::BindTexture(4, m_SamplerState);
            IRenderBlock::BindTexture(9, m_SamplerState);

            if (flags & USE_FEATURE_MAP) {
                IRenderBlock::BindTexture(5, m_SamplerState);
            }

            if (flags & USE_WRINKLE_MAP) {
                IRenderBlock::BindTexture(7, 6, m_SamplerState);
            }

            if (flags & USE_CAMERA_LIGHTING) {
                IRenderBlock::BindTexture(8, m_SamplerState);
            }

            if (m_Block.m_Attributes.m_Flags & TRANSPARENCY_ALPHABLENDING) {
                context->m_Renderer->SetBlendingEnabled(true);
                context->m_Renderer->SetBlendingFunc(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA,
                                                     D3D11_BLEND_ONE);
            } else {
                context->m_Renderer->SetBlendingEnabled(false);
            }
            break;
        }

        case EYES: {
            if (m_Block.m_Attributes.m_Flags & USE_EYE_REFLECTION) {
                IRenderBlock::BindTexture(10, 11, m_SamplerState);
            }

            context->m_Renderer->SetBlendingEnabled(true);
            context->m_Renderer->SetBlendingFunc(D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_SRC_ALPHA,
                                                 D3D11_BLEND_ONE);
            break;
        }

        case HAIR: {
            IRenderBlock::BindTexture(1, m_SamplerState);
            IRenderBlock::BindTexture(2, m_SamplerState);

            if (m_Block.m_Attributes.m_Flags & TRANSPARENCY_ALPHABLENDING) {
                context->m_Renderer->SetBlendingEnabled(true);
                context->m_Renderer->SetBlendingFunc(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA,
                                                     D3D11_BLEND_ONE);
            } else {
                context->m_Renderer->SetBlendingEnabled(false);
            }
            break;
        }
    }
}

void RenderBlockCharacter::DrawContextMenu()
{
    // clang-format off
    static std::array flag_labels = {
        "Disable Backface Culling",     "Eight Bones",                  "Use Alpha Mask",               "Transparency Alpha Blending",
        "Use Feature Map",              "Use Wrinkle Map",              "Use Camera Lighting",          "Use Eye Reflection",
    };
    // clang-format on

    ImGuiCustom::DropDownFlags(m_Block.m_Attributes.m_Flags, flag_labels);
}

void RenderBlockCharacter::DrawUI()
{
    const auto flags = m_Block.m_Attributes.m_Flags;

    ImGui::Text(ICON_FA_COGS "  Attributes");
    ImGui::DragFloat("Scale", &m_Block.m_Attributes.m_Scale, 1.0f, 0.0f);
    ImGui::DragFloat2("Detail Tiling Factor UV", glm::value_ptr(m_Block.m_Attributes.m_DetailTilingFactorUV));
    ImGui::DragFloat2("Decal Blend Factors", glm::value_ptr(m_Block.m_Attributes.m_DecalBlendFactors));
    ImGui::DragFloat("Specular Gloss", &m_Block.m_Attributes.m_SpecularGloss, 0.01f, 0.0f, 20.0f);
    ImGui::DragFloat("Transmission", &m_Block.m_Attributes.m_Transmission, 1.0f);
    ImGui::DragFloat("Specular Fresnel", &m_Block.m_Attributes.m_SpecularFresnel, 1.0f);
    ImGui::DragFloat("Diffuse Roughness", &m_Block.m_Attributes.m_DiffuseRoughness, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Diffuse Wrap", &m_Block.m_Attributes.m_DiffuseWrap, 1.0f);
    ImGui::DragFloat("Dirt Factor", &m_Block.m_Attributes.m_DirtFactor, 0.01f, -1.0f, 1.0f);
    ImGui::DragFloat("Emissive", &m_Block.m_Attributes.m_Emissive, 1.0f);

    // Textures
    ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
    ImGui::Columns(3, nullptr, false);
    {
        IRenderBlock::DrawUI_Texture("DiffuseMap", 0);

        if ((flags & BODY_PART) == GEAR || (flags & BODY_PART) == HAIR) {
            IRenderBlock::DrawUI_Texture("NormalMap", 1);
            IRenderBlock::DrawUI_Texture("PropertiesMap", 2);
        }

        if ((flags & BODY_PART) == GEAR) {
            IRenderBlock::DrawUI_Texture("DetailDiffuseMap", 3);
            IRenderBlock::DrawUI_Texture("DetailNormalMap", 4);

            if (m_Block.m_Attributes.m_Flags & USE_FEATURE_MAP) {
                IRenderBlock::DrawUI_Texture("FeatureMap", 5);
            }

            if (m_Block.m_Attributes.m_Flags & USE_WRINKLE_MAP) {
                IRenderBlock::DrawUI_Texture("WrinkleMap", 7);
            }

            if (m_Block.m_Attributes.m_Flags & USE_CAMERA_LIGHTING) {
                IRenderBlock::DrawUI_Texture("CameraMap", 8);
            }

            IRenderBlock::DrawUI_Texture("MetallicMap", 9);
        } else if ((flags & BODY_PART) == EYES && flags & USE_EYE_REFLECTION) {
            IRenderBlock::DrawUI_Texture("ReflectionMap", 10);
        }
    }
    ImGui::EndColumns();
}
}; // namespace jc3
