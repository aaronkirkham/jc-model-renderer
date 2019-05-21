#pragma once

#include "irenderblock.h"

namespace jc4
{
static constexpr uint32_t CHARACTER_CONSTANTS      = 0x25970695;
static constexpr uint32_t CHARACTER_EYES_CONSTANTS = 0x1BAC0639;
static constexpr uint32_t CHARACTER_HAIR_CONSTANTS = 0x342303CE;
static constexpr uint32_t CHARACTER_SKIN_CONSTANTS = 0xD8749464;

#pragma pack(push, 8)
struct SCharacterConstants {
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
};

struct SEyeGlossConstants {
    float    m_EyeGlossiness;
    float    m_EyeSpecularIntensity;
    float    m_EyeReflectIntensity;
    float    m_EyeReflectThreshold;
    float    m_EyeGlossShadowIntensity;
    uint16_t m_CustomEyeReflectionCube : 1;
};

struct SHairConstants {
    float    m_RoughnessModulator;
    float    m_SpecularMultiplier;
    float    m_ScatteringMultiplier;
    float    m_ShiftFactor;
    float    m_DielectricReflectance;
    uint16_t m_DoubleSided : 1;
    uint16_t m_AlphaTest : 1;
    uint16_t m_AlphaBlending : 1;
};

struct SCharacterSkinConstants {
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
    uint16_t m_DoubleSided : 1;
    uint16_t m_UseAlphaMask : 1;
    uint16_t m_UseWrinkleMap : 1;
    uint16_t m_UseFur : 1;
    uint16_t m_UseMPMChannelInput : 1;
    uint16_t m_UseCustomDiffuseRoughness : 1;
    uint16_t m_UseTransmission : 1;
    uint16_t m_HeadMaterial : 1;
};
#pragma pack(pop)

class RenderBlockCharacter : public IRenderBlock
{
  private:
#pragma pack(push, 1)
    struct cbLocalConsts {
        glm::mat4 WorldViewProjection;
        glm::mat4 World;
    } m_cbLocalConsts;

    struct cbSkinningConsts {
        glm::mat3x4 MatrixPalette[70];
    } m_cbSkinningConsts;

    struct cbInstanceConsts {
        glm::vec2 TilingUV = glm::vec2(1, 1);
        glm::vec2 Unknown;
        glm::vec4 Scale = glm::vec4(1, 1, 1, 1);
    } m_cbInstanceConsts;

    struct cbDefaultConstants {
        float     DirtFactor    = 0;
        float     WetFactor     = 0;
        float     LodFade       = 0;
        float     AlphaFade     = 0;
        float     SnowFactor    = 0;
        float     _unknown[4]   = {0};
        int32_t   UseDetail     = 0;
        int32_t   UseDecal      = 0;
        int32_t   UseTint       = 0;
        int32_t   UseSoftTint   = 0;
        float     _unknown2[15] = {0};
        glm::vec4 DebugColor    = {0, 0, 0, 1};
        glm::vec4 TintColor1    = {0, 0, 0, 0};
        glm::vec4 TintColor2    = {0, 0, 0, 0};
        glm::vec4 TintColor3    = {0, 0, 0, 0};
        float     AlphaTestRef  = 0.5f;
        float     _unknown3[3]  = {0};
        char      FurConsts[32] = {0};
    } m_cbDefaultConstants;

    struct cbHairConstants {
    } m_cbHairConstants;

    struct cbUnknownCB4 {
        glm::vec4 Unknown[5];
    } m_cbUnknownCB4;

    struct cbUnknownCB6 {
        glm::vec4 Unknown[46];
    } m_cbUnknownCB6;
#pragma pack(pop)

    std::array<ConstantBuffer_t*, 3> m_VertexShaderConstants = {nullptr};
    std::array<ConstantBuffer_t*, 3> m_PixelShaderConstants  = {nullptr};
    int32_t                          m_Stride                = 0;

  public:
    RenderBlockCharacter() = default;
    virtual ~RenderBlockCharacter()
    {
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[0]);
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[1]);
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[2]);
        Renderer::Get()->DestroyBuffer(m_PixelShaderConstants[0]);
        Renderer::Get()->DestroyBuffer(m_PixelShaderConstants[1]);
        Renderer::Get()->DestroyBuffer(m_PixelShaderConstants[2]);
    }

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockCharacter";
    }

    virtual uint32_t GetTypeHash() const override final
    {
        return RenderBlockFactory::RB_CHARACTER;
    }

    virtual bool IsOpaque() override final
    {
        // TODO
        // Find out where the flags are stored. They must be on the modelc "Attributes" reference that we stil cannot
        // parse within ADF (0xDEFE88ED type)

        /*
            _flags = *(v2 + 0x258);
            if ( _bittest(&_flags, 0x10u) || _flags & 8 )
                result = 0;
        */

        return true;
    }

    virtual void Load(SAdfDeferredPtr* constants) override final
    {
#if 0
        SCharacterAttribute attributes{};

        if (constants->m_Type == CHARACTER_CONSTANTS) {
            auto character_constants = (SCharacterConstants*)constants->m_Ptr;
            __debugbreak();
        } else if (constants->m_Type == CHARACTER_EYES_CONSTANTS) {
            auto eye_constants = (SEyeGlossConstants*)constants->m_Ptr;

            attributes.m_Flags = 0x10000;
            /*v14->attributes.flags = (*(_eye_consts + 10) & 1 | 0x200) << 7;*/
            attributes.m_EyeGlossiness           = eye_constants->m_EyeGlossiness;
            attributes.m_EyeSpecularIntensity    = eye_constants->m_EyeSpecularIntensity;
            attributes.m_EyeReflectIntensity     = eye_constants->m_EyeReflectIntensity;
            attributes.m_EyeReflectThreshold     = eye_constants->m_EyeReflectThreshold;
            attributes.m_EyeGlossShadowIntensity = eye_constants->m_EyeGlossShadowIntensity;

            __debugbreak();
        } else if (constants->m_Type == CHARACTER_HAIR_CONSTANTS) {
            auto hair_constants = (SHairConstants*)constants->m_Ptr;

            attributes.m_Flags = 0x20000;

            __debugbreak();
        } else if (constants->m_Type == CHARACTER_SKIN_CONSTANTS) {
            auto skin_constants = (SCharacterSkinConstants*)constants->m_Ptr;

            attributes.m_DirtFactor = skin_constants->m_DirtFactor;
            attributes.m_FurLength  = skin_constants->m_FurLength;
            attributes.m_FurGravity = skin_constants->m_FurGravity;

            __debugbreak();
        }
#endif
    }

    virtual void Create(const std::string& type_id, IBuffer_t* vertex_buffer, IBuffer_t* index_buffer) override final
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
            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(
                inputDesc, 5, m_VertexShader.get(), "RenderBlockCharacter (CharacterMesh1UVMesh)");
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
            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(
                inputDesc, 5, m_VertexShader.get(), "RenderBlockCharacter (CharacterMesh2UVMesh)");

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

    virtual void Setup(RenderContext_t* context) override final
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

    virtual void DrawContextMenu() override final {}

    virtual void DrawUI() override final
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

    virtual void SetData(vertices_t* vertices, uint16s_t* indices, materials_t* materials) override final
    {
        //
    }

    virtual std::tuple<vertices_t, uint16s_t> GetData() override final
    {
        using namespace jc::Vertex;
        using namespace jc::Vertex::RenderBlockCharacter;

        vertices_t vertices;
        uint16s_t  indices = m_IndexBuffer->CastData<uint16_t>();

        switch (m_VertexBuffer->m_ElementStride) {
            case sizeof(Packed4Bones1UV): {
                const auto& vb = m_VertexBuffer->CastData<Packed4Bones1UV>();
                vertices.reserve(vb.size());
                for (const auto& vertex : vb) {
                    vertex_t v{};
                    v.pos = glm::vec3{unpack(vertex.x), unpack(vertex.y), unpack(vertex.z)};
                    v.uv  = glm::vec2{unpack(vertex.u0), unpack(vertex.v0)};
                    vertices.emplace_back(std::move(v));
                }
                break;
            }

#ifdef DEBUG
            default: {
                __debugbreak();
                break;
            }
#endif
        }

        return {vertices, indices};
    }
};
} // namespace jc4
