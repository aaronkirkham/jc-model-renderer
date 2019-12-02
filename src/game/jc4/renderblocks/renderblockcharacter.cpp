//#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

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
                                                                       "Character VertexDecl (CharacterMesh1UVMesh)");
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
                                                                       "Character VertexDecl (CharacterMesh2UVMesh)");

    } else {
#ifdef DEBUG
        __debugbreak();
#endif
    }

    // create vertex shader constants
    m_VertexShaderConstants[0] =
        Renderer::Get()->CreateConstantBuffer(m_VertexPerDrawConsts, "Character VertexPerDrawConsts");
    m_VertexShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(m_cbSkinningConsts, "Character SkinningConsts");
    m_VertexShaderConstants[2] = Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts, "Character MaterialConsts");

    // create pixel shader constants
    m_PixelShaderConstants[0] =
        Renderer::Get()->CreateConstantBuffer(m_DefaultPerDrawConstants, "Character DefaultPerDrawConstants");
    m_PixelShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(m_cbUnknownCB4, "Character UnknownCB4");
    m_PixelShaderConstants[2] = Renderer::Get()->CreateConstantBuffer(m_cbUnknownCB6, "Character UnknownCB6");

    // init the skinning palette data
    for (auto i = 0; i < 70; ++i) {
        m_cbSkinningConsts.MatrixPalette[i] = glm::mat3x4(1);
    }

    //
    memset(&m_cbMaterialConsts, 0, sizeof(m_cbMaterialConsts));
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
    m_SamplerState        = Renderer::Get()->CreateSamplerState(params, "Character SamplerState");
}

void RenderBlockCharacter::Setup(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    IRenderBlock::Setup(context);

    static auto world = glm::mat4(1);

    // set vertex shader constants
    m_VertexPerDrawConsts.WorldViewProjection = world * context->m_viewProjectionMatrix;
    m_VertexPerDrawConsts.ModelWorld          = world;
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 2, m_VertexPerDrawConsts);
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 3, m_cbSkinningConsts);

    // update shader constants
    switch (m_Attributes->m_Type) {
        case ATTR_TYPE_CHARACTER_CONSTANTS: {
            const auto attributes = (SCharacterConstants*)m_Attributes->m_Ptr;

            // material consts
            {
                m_cbMaterialConsts.UVScale1              = {1, 1};
                m_cbMaterialConsts.UVScale2              = {1, 1};
                m_cbMaterialConsts.Scale                 = m_ScaleModifier;
                m_cbMaterialConsts.DetailTileFactor      = {attributes->m_DetailTilingFactorUV[0],
                                                       attributes->m_DetailTilingFactorUV[1]};
                m_cbMaterialConsts.DecalBlendFactor      = {attributes->m_DecalBlendFactors[0],
                                                       attributes->m_DecalBlendFactors[1]};
                m_cbMaterialConsts.RoughnessModulator    = attributes->m_RoughnessModulator;
                m_cbMaterialConsts.MetallicModulator     = attributes->m_MetallicModulator;
                m_cbMaterialConsts.DielectricReflectance = attributes->m_DielectricReflectance;
                m_cbMaterialConsts.DiffuseRoughness      = attributes->m_DiffuseRoughness;
                m_cbMaterialConsts.SpecularAniso         = attributes->m_SpecularAniso;
                m_cbMaterialConsts.Transmission          = attributes->m_Transmission;
                m_cbMaterialConsts.EmissiveIntensity     = attributes->m_EmissiveIntensity;
                m_cbMaterialConsts.DirtFactor            = attributes->m_DirtFactor;

                context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[2], 4, m_cbMaterialConsts);
            }

            // per vertex consts
            {

                m_DefaultPerDrawConstants.DirtFactor = attributes->m_DirtFactor;
                // @TODO: other SCharacterConsts members
                m_DefaultPerDrawConstants.UseMPMChannelInput = attributes->m_UseMPMChannelInput;
                m_DefaultPerDrawConstants.UseTransmission    = attributes->m_UseTransmission;
                m_DefaultPerDrawConstants.UseDetail          = attributes->m_UseDetail;
                m_DefaultPerDrawConstants.UseDecal           = attributes->m_UseDecal;
                m_DefaultPerDrawConstants.UseTint            = attributes->m_UseTint;
                m_DefaultPerDrawConstants.UseTintSoftBlend   = attributes->m_UseTintSoftBlend;

                context->m_Renderer->SetPixelShaderConstants(m_PixelShaderConstants[0], 1, m_DefaultPerDrawConstants);
            }
            break;
        }
    }

    // set pixel shader constants
    // context->m_Renderer->SetPixelShaderConstants(m_PixelShaderConstants[0], 1, m_cbDefaultConstants);
    context->m_Renderer->SetPixelShaderConstants(m_PixelShaderConstants[1], 4, m_cbUnknownCB4);
    context->m_Renderer->SetPixelShaderConstants(m_PixelShaderConstants[2], 6, m_cbUnknownCB6);

    // set textures
    IRenderBlock::BindTexture(0, m_SamplerState);
    IRenderBlock::BindTexture(1, m_SamplerState);
    IRenderBlock::BindTexture(2, m_SamplerState);
}

void RenderBlockCharacter::DrawContextMenu()
{
    assert(m_Attributes);

    switch (m_Attributes->m_Type) {
        case ATTR_TYPE_CHARACTER_CONSTANTS: {
            ImGui::MenuItem("Disable Backface Culling");
            ImGui::MenuItem("Transparency Alpha Testing");
            ImGui::MenuItem("Transparency Alpha Blending");
            ImGui::MenuItem("Use Detail");
            ImGui::MenuItem("Use Decal");
            ImGui::MenuItem("Use Tint");
            ImGui::MenuItem("Use Tint Soft Blend");
            ImGui::MenuItem("Use Wrinkle Map");
            ImGui::MenuItem("Use Custom Emissive Hue");
            ImGui::MenuItem("Use MPM Channel Input");
            ImGui::MenuItem("Use Custom Diffuse Roughness");
            ImGui::MenuItem("Use Specular Aniso");
            ImGui::MenuItem("Use Transmission");

            /*
                uint32_t m_DoubleSided : 1;
                uint32_t m_AlphaTest : 1;
                uint32_t m_AlphaBlending : 1;
                uint32_t m_UseDetail : 1;
                uint32_t m_UseDecal : 1;
                uint32_t m_UseTint : 1;
                uint32_t m_UseTintSoftBlend : 1;
                uint32_t m_UseWrinkleMap : 1;
                uint32_t m_UseCustomEmissiveHue : 1;
                uint32_t m_UseMPMChannelInput : 1;
                uint32_t m_UseCustomDiffuseRoughness : 1;
                uint32_t m_UseSpecularAniso : 1;
                uint32_t m_UseTransmission : 1;
            */
            break;
        }

        case ATTR_TYPE_CHARACTER_EYES_CONSTANTS: {
            ImGui::MenuItem("Custom Eye Reflection Cube");

            /*
                uint16_t m_CustomEyeReflectionCube : 1;
            */
            break;
        }

        case ATTR_TYPE_CHARACTER_HAIR_CONSTANTS: {
            ImGui::MenuItem("Disable Backface Culling");
            ImGui::MenuItem("Transparency Alpha Testing");
            ImGui::MenuItem("Transparency Alpha Blending");

            /*
                uint16_t m_DoubleSided : 1;
                uint16_t m_AlphaTest : 1;
                uint16_t m_AlphaBlending : 1;
            */
            break;
        }

        case ATTR_TYPE_CHARACTER_SKIN_CONSTANTS: {
            ImGui::MenuItem("Disable Backface Culling");
            ImGui::MenuItem("Use Alpha Mask");
            ImGui::MenuItem("Use Wrinkle Map");
            ImGui::MenuItem("Use Fur");
            ImGui::MenuItem("Use MPM Channel Input");
            ImGui::MenuItem("Use Custom Diffuse Roughness");
            ImGui::MenuItem("Use Transmission");
            ImGui::MenuItem("Head Material");

            /*
                uint16_t m_DoubleSided : 1;
                uint16_t m_UseAlphaMask : 1;
                uint16_t m_UseWrinkleMap : 1;
                uint16_t m_UseFur : 1;
                uint16_t m_UseMPMChannelInput : 1;
                uint16_t m_UseCustomDiffuseRoughness : 1;
                uint16_t m_UseTransmission : 1;
                uint16_t m_HeadMaterial : 1;
            */
            break;
        }
    }
}

void RenderBlockCharacter::DrawUI()
{
    ImGui::Text(ICON_FA_COGS "  Attributes");
    ImGui::SliderFloat("Scale", &m_ScaleModifier, 0.0f, 20.0f);

    assert(m_Attributes);

    switch (m_Attributes->m_Type) {
        case ATTR_TYPE_CHARACTER_CONSTANTS: {
            /*
                float    m_DetailTilingFactorUV[2];
                float    m_DecalBlendFactors[2];
                float    m_RoughnessModulator;
                float    m_MetallicModulator;
                float    m_DielectricReflectance;
                float    m_DiffuseRoughness;
                float    m_SpecularAniso;
                float    m_Transmission;
                float    m_EmissiveIntensity;
                float    m_DirtFactor;
            */

            const auto attributes = (SCharacterConstants*)m_Attributes->m_Ptr;

            // @TODO: min/max
            ImGui::DragFloat2("Detail Tiling Factor UV", attributes->m_DetailTilingFactorUV, 1.0f, -1.0f, 1.0f);
            ImGui::DragFloat2("Decal Blend Factors", attributes->m_DecalBlendFactors);
            ImGui::DragFloat("m_RoughnessModulator", &attributes->m_RoughnessModulator);
            ImGui::DragFloat("m_MetallicModulator", &attributes->m_MetallicModulator);
            ImGui::DragFloat("m_DielectricReflectance", &attributes->m_DielectricReflectance);
            ImGui::DragFloat("m_DiffuseRoughness", &attributes->m_DiffuseRoughness);
            ImGui::DragFloat("m_SpecularAniso", &attributes->m_SpecularAniso);
            ImGui::DragFloat("m_Transmission", &attributes->m_Transmission);
            ImGui::DragFloat("m_EmissiveIntensity", &attributes->m_EmissiveIntensity);
            ImGui::DragFloat("m_DirtFactor", &attributes->m_DirtFactor);
            break;
        }

        case ATTR_TYPE_CHARACTER_EYES_CONSTANTS: {
            /*
                float    m_EyeGlossiness;
                float    m_EyeSpecularIntensity;
                float    m_EyeReflectIntensity;
                float    m_EyeReflectThreshold;
                float    m_EyeGlossShadowIntensity;
            */
            break;
        }

        case ATTR_TYPE_CHARACTER_HAIR_CONSTANTS: {
            /*
                float    m_RoughnessModulator;
                float    m_SpecularMultiplier;
                float    m_ScatteringMultiplier;
                float    m_ShiftFactor;
                float    m_DielectricReflectance;
            */
            break;
        }

        case ATTR_TYPE_CHARACTER_SKIN_CONSTANTS: {
            /*
                float    m_DetailTilingFactorUV[2];
                float    m_RoughnessModulator;
                float    m_TransmissionModulator;
                float    m_DiffuseRoughness;
                float    m_Transmission;
                float    m_DirtFactor;
                float    m_FurLength;
                float    m_FurThickness;
                float    m_FurRoughness;
                float    m_FurGravity;
                float    m_FurSize;
            */
            break;
        }
    }

#if 0
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
#endif

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
