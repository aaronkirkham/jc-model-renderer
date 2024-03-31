#pragma once

#include "irenderblock.h"

#pragma pack(push, 1)
struct GeneralMkIIIAttributes {
    float                        m_DepthBias;
    char                         _pad[0x8];
    float                        m_EmissiveTODScale;
    float                        m_EmissiveStartFadeDistSq;
    char                         _pad2[0x10];
    jc::Vertex::SPackedAttribute m_Packed;
    char                         _pad3[0x4];
    uint32_t                     m_Flags;
    char                         _pad4[0x1C];
};

static_assert(sizeof(GeneralMkIIIAttributes) == 0x68, "GeneralMkIIIAttributes alignment is wrong!");

namespace jc::RenderBlocks
{
static constexpr uint8_t GENERALMKIII_VERSION        = 5;
static constexpr uint8_t GENERALMKIII_TEXTURES_COUNT = 20;

struct GeneralMkIII {
    uint8_t                m_Version;
    GeneralMkIIIAttributes m_Attributes;
};
}; // namespace jc::RenderBlocks
#pragma pack(pop)

namespace jc3
{
class RenderBlockGeneralMkIII : public IRenderBlock
{
  private:
    enum {
        BACKFACE_CULLING           = 0x1,
        TRANSPARENCY_ALPHABLENDING = 0x2,
        TRANSPARENCY_ALPHATESTING  = 0x4,
        DYNAMIC_EMISSIVE           = 0x8,
        WORLD_SPACE_NORMALS        = 0x10,
        IS_SKINNED                 = 0x20,
        UNKNOWN                    = 0x40,
        USE_LAYERED                = 0x80,
        USE_OVERLAY                = 0x100,
        USE_DECAL                  = 0x200,
        USE_DAMAGE                 = 0x400,
        ANISOTROPIC_FILTERING      = 0x4000,
        DESTRUCTION                = 0x8000,
    };

    struct RBIInfo {
        glm::mat4 ModelWorldMatrix;
        glm::mat4 ModelWorldMatrixPrev; // [unused]
        glm::vec4 ModelDiffuseColor;
        glm::vec4 ModelAmbientColor;  // [unused]
        glm::vec4 ModelSpecularColor; // [unused]
        glm::vec4 ModelEmissiveColor;
        glm::vec4 ModelDebugColor; // [unused]
    } m_cbRBIInfo;

    struct InstanceAttributes {
        glm::vec4 UVScale;
        glm::vec4 RippleAngle;    // [unused]
        glm::vec4 FlashDirection; // [unused]
        float     DepthBias;
        float     QuantizationScale;
        float     EmissiveStartFadeDistSq;
        float     EmissiveTODScale;
        float     RippleSpeed;      // [unused]
        float     RippleMagnitude;  // [unused]
        float     PixelScale;       // [unused]
        float     OutlineThickness; // [unused]
    } m_cbInstanceAttributes;

    struct cbSkinningConsts {
        glm::mat3x4 MatrixPalette[256];
    } m_cbSkinningConsts;

    struct MaterialConsts {
        glm::vec4 DebugColor; // [unused]
        float     EnableVariableDialecticSpecFresnel;
        float     UseMetallicVer2;
    } m_cbMaterialConsts;

    struct MaterialConsts2 {
        float NormalStrength; // [unused]
        float Reflectivity_1; // [unused]
        float Roughness_1;
        float DiffuseWrap_1;
        float Emissive_1;
        float Transmission_1;
        float ClearCoat_1;
        float Roughness_2;               // [unused]
        float DiffuseWrap_2;             // [unused]
        float Emissive_2;                // [unused]
        float Transmission_2;            // [unused]
        float Reflectivity_2;            // [unused]
        float ClearCoat_2;               // [unused]
        float Roughness_3;               // [unused]
        float DiffuseWrap_3;             // [unused]
        float Emissive_3;                // [unused]
        float Transmission_3;            // [unused]
        float Reflectivity_3;            // [unused]
        float ClearCoat_3;               // [unused]
        float Roughness_4;               // [unused]
        float DiffuseWrap_4;             // [unused]
        float Emissive_4;                // [unused]
        float Transmission_4;            // [unused]
        float Reflectivity_4;            // [unused]
        float ClearCoat_4;               // [unused]
        float LayeredHeightMapUVScale;   // [unused]
        float LayeredUVScale;            // [unused]
        float LayeredHeight1Influence;   // [unused]
        float LayeredHeight2Influence;   // [unused]
        float LayeredHeightMapInfluence; // [unused]
        float LayeredMaskInfluence;      // [unused]
        float LayeredShift;              // [unused]
        float LayeredRoughness;          // [unused]
        float LayeredDiffuseWrap;        // [unused]
        float LayeredEmissive;           // [unused]
        float LayeredTransmission;       // [unused]
        float LayeredReflectivity;       // [unused]
        float LayeredClearCoat;          // [unused]
        float DecalBlend;                // [unused]
        float DecalBlendNormal;          // [unused]
        float DecalReflectivity;         // [unused]
        float DecalRoughness;            // [unused]
        float DecalDiffuseWrap;          // [unused]
        float DecalEmissive;             // [unused]
        float DecalTransmission;         // [unused]
        float DecalClearCoat;            // [unused]
        float OverlayHeightInfluence;    // [unused]
        float OverlayHeightMapInfluence; // [unused]
        float OverlayMaskInfluence;      // [unused]
        float OverlayShift;              // [unused]
        float OverlayColorR;             // [unused]
        float OverlayColorG;             // [unused]
        float OverlayColorB;             // [unused]
        float OverlayBrightness;         // [unused]
        float OverlayGloss;              // [unused]
        float OverlayMetallic;           // [unused]
        float OverlayReflectivity;       // [unused]
        float OverlayRoughness;          // [unused]
        float OverlayDiffuseWrap;        // [unused]
        float OverlayEmissive;           // [unused]
        float OverlayTransmission;       // [unused]
        float OverlayClearCoat;          // [unused]
        float DamageReflectivity;        // [unused]
        float DamageRoughness;           // [unused]
        float DamageDiffuseWrap;         // [unused]
        float DamageEmissive;            // [unused]
        float DamageTransmission;        // [unused]
        float DamageHeightInfluence;     // [unused]
        float DamageMaskInfluence;       // [unused]
        float DamageClearCoat;           // [unused]
    } m_cbMaterialConsts2;

    jc::RenderBlocks::GeneralMkIII   m_Block;
    std::vector<jc3::CSkinBatch>     m_SkinBatches;
    VertexBuffer_t*                  m_VertexBufferData        = nullptr;
    std::string                      m_ShaderName              = "generalmkiii";
    std::array<ConstantBuffer_t*, 3> m_VertexShaderConstants   = {nullptr};
    std::array<ConstantBuffer_t*, 2> m_FragmentShaderConstants = {nullptr};

    bool show_material_consts = false;

  public:
    RenderBlockGeneralMkIII() = default;
    virtual ~RenderBlockGeneralMkIII();

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockGeneralMkIII";
    }

    virtual uint32_t GetTypeHash() const override final;

    virtual bool IsOpaque() override final
    {
        return ~m_Block.m_Attributes.m_Flags & TRANSPARENCY_ALPHABLENDING;
    }

    virtual float GetScale() const override final
    {
        return m_Block.m_Attributes.m_Packed.m_Scale;
    }

    virtual void Create() override final;

    virtual void Read(std::istream& stream) override final;

    virtual void Write(std::ostream& stream) override final
    {
        // write the block attributes
        stream.write((char*)&m_Block, sizeof(m_Block));

        // write constant buffer data
        stream.write((char*)&m_cbMaterialConsts2, sizeof(m_cbMaterialConsts2));

        // write the materials
        WriteMaterials(stream);

        // write the vertex buffer
        WriteBuffer(stream, m_VertexBuffer);
        WriteBuffer(stream, m_VertexBufferData);

        // write the skin batches
        if (m_Block.m_Attributes.m_Flags & (IS_SKINNED | DESTRUCTION)) {
            WriteSkinBatch(stream, &m_SkinBatches);
        }

        // write the index buffer
        WriteBuffer(stream, m_IndexBuffer);
    }

    virtual void Setup(RenderContext_t* context) override final;
    virtual void Draw(RenderContext_t* context) override final;

    virtual void DrawContextMenu() override final;
    virtual void DrawUI() override final;

    virtual void SetData(vertices_t* vertices, uint16s_t* indices, materials_t* materials) override final;

    virtual std::tuple<vertices_t, uint16s_t> GetData() override final
    {
        using namespace jc::Vertex;

        vertices_t vertices;
        uint16s_t  indices = m_IndexBuffer->CastData<uint16_t>();

        if (m_Block.m_Attributes.m_Flags & IS_SKINNED) {
            const auto& vb = m_VertexBuffer->CastData<UnpackedVertexPositionXYZW>();
            vertices.reserve(vb.size());

            for (const auto& vertex : vb) {
                vertex_t v;
                v.pos = glm::vec3{vertex.x, vertex.y, vertex.z};
                vertices.emplace_back(std::move(v));
            }
        } else {
            const auto& vb = m_VertexBuffer->CastData<PackedVertexPosition>();
            vertices.reserve(vb.size());

            for (const auto& vertex : vb) {
                vertex_t v;
                v.pos = glm::vec3{unpack(vertex.x), unpack(vertex.y), unpack(vertex.z)};
                vertices.emplace_back(std::move(v));
            }
        }

        const auto& vbdata = m_VertexBufferData->CastData<GeneralShortPacked>();
        for (auto i = 0; i < vbdata.size(); ++i) {
            vertices[i].uv =
                glm::vec2{unpack(vbdata[i].u0), unpack(vbdata[i].v0)} * m_Block.m_Attributes.m_Packed.m_UV0Extent;
            vertices[i].normal = unpack_normal(vbdata[i].n);
        }

        return {vertices, indices};
    }

    rb_textures_t GetTextures() override final
    {
        rb_textures_t result;
        result.push_back({"diffuse", m_Textures[0]});
        result.push_back({"specular", m_Textures[1]});
        result.push_back({"metallic", m_Textures[2]});
        result.push_back({"normal", m_Textures[3]});

        return result;
    }
};
} // namespace jc3
