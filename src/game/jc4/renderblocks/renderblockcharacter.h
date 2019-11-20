#pragma once

#include "irenderblock.h"

namespace jc4
{
static constexpr uint32_t ATTR_TYPE_CHARACTER_CONSTANTS      = 0x25970695;
static constexpr uint32_t ATTR_TYPE_CHARACTER_EYES_CONSTANTS = 0x1BAC0639;
static constexpr uint32_t ATTR_TYPE_CHARACTER_HAIR_CONSTANTS = 0x342303CE;
static constexpr uint32_t ATTR_TYPE_CHARACTER_SKIN_CONSTANTS = 0xD8749464;

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
    virtual ~RenderBlockCharacter();

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockCharacter";
    }

    virtual uint32_t GetTypeHash() const override final;

    virtual bool IsOpaque() override final
    {
        // @TODO
        /*
            _flags = *(v2 + 0x258);
            if ( _bittest(&_flags, 0x10u) || _flags & 8 )
                result = 0;
        */

        return true;
    }

    virtual void Load(ava::AvalancheDataFormat::SAdfDeferredPtr* constants) override final
    {
        // m_AttributesType = constants->m_Type;
        m_Attributes = constants;

#if 0
        switch (m_AttributesType) {
            case ATTR_TYPE_CHARACTER_CONSTANTS: {
                auto character_constants = (SCharacterConstants*)constants->m_Ptr;
                character_constants->
                __debugbreak();
                break;
            }

            case ATTR_TYPE_CHARACTER_EYES_CONSTANTS: {
                //
                break;
            }

            case ATTR_TYPE_CHARACTER_HAIR_CONSTANTS: {
                //
                break;
            }

            case ATTR_TYPE_CHARACTER_SKIN_CONSTANTS: {
                //
                break;
            }
        }
#endif

#ifdef _DEBUG
        // __debugbreak();
#endif

#if 0
        // SCharacterAttribute attributes{};
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

    virtual void Create(const std::string& type_id, IBuffer_t* vertex_buffer, IBuffer_t* index_buffer) override final;
    virtual void Setup(RenderContext_t* context) override final;

    virtual void DrawContextMenu() override final;
    virtual void DrawUI() override final;

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
