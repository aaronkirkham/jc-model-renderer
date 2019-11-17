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

    m_Block                  = {};
    m_Block.attributes.scale = 1.0f;

    Create();
}

void RenderBlockCharacter::Create()
{
    m_PixelShader = ShaderManager::Get()->GetPixelShader(
        (m_Block.attributes.flags & BODY_PART) != HAIR ? "character" : "characterhair_msk");

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
                                                                           "RenderBlockCharacter (Packed4Bones1UV)");
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
                                                                           "RenderBlockCharacter (Packed4Bones2UVs)");
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
                                                                           "RenderBlockCharacter (Packed4Bones3UVs)");
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
                                                                           "RenderBlockCharacter (Packed8Bones1UV)");
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
                                                                           "RenderBlockCharacter (Packed8Bones2UVs)");
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
                                                                           "RenderBlockCharacter (Packed8Bones3UVs)");
            break;
        }
    }

    // create the constant buffer
    m_VertexShaderConstants =
        Renderer::Get()->CreateConstantBuffer(m_cbLocalConsts, "RenderBlockCharacter cbLocalConsts");
    m_PixelShaderConstants[0] =
        Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "RenderBlockCharacter cbInstanceConsts");
    m_PixelShaderConstants[1] =
        Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts, "RenderBlockCharacter cbMaterialConsts");

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

        m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockCharacter");
    }
}

void RenderBlockCharacter::Setup(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    IRenderBlock::Setup(context);

    const auto flags = m_Block.attributes.flags;

    // setup the constant buffer
    static auto world = glm::mat4(1);

    // set the textures
    IRenderBlock::BindTexture(0, m_SamplerState);

    // set vertex shader constants
    m_cbLocalConsts.World               = world;
    m_cbLocalConsts.WorldViewProjection = world * context->m_viewProjectionMatrix;
    m_cbLocalConsts.Scale               = glm::vec4(m_Block.attributes.scale * m_ScaleModifier);
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants, 1, m_cbLocalConsts);

    // set pixel shader constants
    context->m_Renderer->SetPixelShaderConstants(m_PixelShaderConstants[0], 1, m_cbInstanceConsts);
    context->m_Renderer->SetPixelShaderConstants(m_PixelShaderConstants[1], 2, m_cbMaterialConsts);

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

            if (m_Block.attributes.flags & TRANSPARENCY_ALPHABLENDING) {
                context->m_Renderer->SetBlendingEnabled(true);
                context->m_Renderer->SetBlendingFunc(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA,
                                                     D3D11_BLEND_ONE);
            } else {
                context->m_Renderer->SetBlendingEnabled(false);
            }
            break;
        }

        case EYES: {
            if (m_Block.attributes.flags & USE_EYE_REFLECTION) {
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

            if (m_Block.attributes.flags & TRANSPARENCY_ALPHABLENDING) {
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

    ImGuiCustom::DropDownFlags(m_Block.attributes.flags, flag_labels);
}

void RenderBlockCharacter::DrawUI()
{
    ImGui::Text(ICON_FA_COGS "  Attributes");
    ImGui::SliderFloat("Scale", &m_ScaleModifier, 0.0f, 20.0f);

    /*if (ImGui::RadioButton("Gear", ((m_Block.attributes.flags & BODY_PART) == GEAR))) {}

    ImGui::SameLine();

    if (ImGui::RadioButton("Eyes", ((m_Block.attributes.flags & BODY_PART) == EYES))) {}

    ImGui::SameLine();

    if (ImGui::RadioButton("Hair", ((m_Block.attributes.flags & BODY_PART) == HAIR))) {}*/

    ImGui::SliderFloat4("Unknown #1", glm::value_ptr(m_cbInstanceConsts._unknown), 0, 1);
    ImGui::ColorEdit4("Diffuse Colour", glm::value_ptr(m_cbInstanceConsts.DiffuseColour));
    ImGui::SliderFloat4("Unknown #2", glm::value_ptr(m_cbInstanceConsts._unknown2), 0, 1);

    // Textures
    ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
    ImGui::Columns(3, nullptr, false);
    {
        const auto flags = m_Block.attributes.flags;

        IRenderBlock::DrawUI_Texture("DiffuseMap", 0);

        if ((flags & BODY_PART) == GEAR || (flags & BODY_PART) == HAIR) {
            IRenderBlock::DrawUI_Texture("NormalMap", 1);
            IRenderBlock::DrawUI_Texture("PropertiesMap", 2);
        }

        if ((flags & BODY_PART) == GEAR) {
            IRenderBlock::DrawUI_Texture("DetailDiffuseMap", 3);
            IRenderBlock::DrawUI_Texture("DetailNormalMap", 4);

            if (m_Block.attributes.flags & USE_FEATURE_MAP) {
                IRenderBlock::DrawUI_Texture("FeatureMap", 5);
            }

            if (m_Block.attributes.flags & USE_WRINKLE_MAP) {
                IRenderBlock::DrawUI_Texture("WrinkleMap", 7);
            }

            if (m_Block.attributes.flags & USE_CAMERA_LIGHTING) {
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
