#pragma once

#include "irenderblock.h"

#pragma pack(push, 1)
struct CarPaintMMAttributes {
    uint32_t m_Flags;
    float    m_Unknown;
};

static_assert(sizeof(CarPaintMMAttributes) == 0x8, "CarPaintMMAttributes alignment is wrong!");

namespace jc::RenderBlocks
{
static constexpr uint8_t CARPAINTMM_VERSION = 14;

struct CarPaintMM {
    uint8_t              m_Version;
    CarPaintMMAttributes m_Attributes;
};
}; // namespace jc::RenderBlocks
#pragma pack(pop)

namespace jc3
{
class RenderBlockCarPaintMM : public IRenderBlock
{
  private:
    enum {
        SUPPORT_DECALS             = 0x1,
        SUPPORT_DAMAGE_BLEND       = 0x2,
        SUPPORT_DIRT               = 0x4,
        SUPPORT_SOFT_TINT          = 0x10,
        SUPPORT_LAYERED            = 0x20,
        SUPPORT_OVERLAY            = 0x40,
        DISABLE_BACKFACE_CULLING   = 0x80,
        TRANSPARENCY_ALPHABLENDING = 0x100,
        TRANSPARENCY_ALPHATESTING  = 0x200,
        IS_DEFORM                  = 0x1000,
        IS_SKINNED                 = 0x2000,
    };

    struct RBIInfo {
        glm::mat4 ModelWorldMatrix;
        glm::mat4 ModelWorldMatrixPrev; // [unused]
        glm::vec4 ModelDiffuseColor = glm::vec4(1);
        glm::vec4 ModelAmbientColor;  // [unused]
        glm::vec4 ModelSpecularColor; // [unused]
        glm::vec4 ModelEmissiveColor; // [unused]
        glm::vec4 ModelDebugColor;    // [unused]
    } m_cbRBIInfo;

    struct InstanceConsts {
        float IsDeformed = 0.0f;
        float _pad[3]; // [unused]
    } m_cbInstanceConsts;

    struct DeformConsts {
        glm::vec4 ControlPoints[216];
    } m_cbDeformConsts;

    struct CarPaintStaticMaterialParams {
        glm::vec4 m_SpecularGloss;
        glm::vec4 m_Metallic;
        glm::vec4 m_ClearCoat;
        glm::vec4 m_Emissive;
        glm::vec4 m_DiffuseWrap;
        glm::vec4 m_DirtParams;
        glm::vec4 m_DirtBlend;
        glm::vec4 m_DirtColor;
        glm::vec4 m_DecalCount; // [unused]
        glm::vec4 m_DecalWidth;
        glm::vec4 m_Decal1Color;
        glm::vec4 m_Decal2Color;
        glm::vec4 m_Decal3Color;
        glm::vec4 m_Decal4Color;
        glm::vec4 m_DecalBlend;
        glm::vec4 m_Damage;
        glm::vec4 m_DamageBlend;
        glm::vec4 m_DamageColor;
        float     SupportDecals;
        float     SupportDmgBlend;
        float     SupportLayered;
        float     SupportOverlay;
        float     SupportRotating; // [unused]
        float     SupportDirt;
        float     SupportSoftTint;
    } m_cbStaticMaterialParams;

    struct CarPaintDynamicMaterialParams {
        glm::vec4 m_TintColorR;
        glm::vec4 m_TintColorG;
        glm::vec4 m_TintColorB;
        glm::vec4 _unused; // [unused]
        float     m_SpecularGlossOverride;
        float     m_MetallicOverride;
        float     m_ClearCoatOverride;
    } m_cbDynamicMaterialParams;

    struct CarPaintDynamicObjectParams {
        glm::vec4 m_DecalIndex = glm::vec4(0);
        float     m_DirtAmount = 0.0f;
        char      _pad[16];
    } m_cbDynamicObjectParams;

    jc::RenderBlocks::CarPaintMM     m_Block;
    jc3::CDeformTable                m_DeformTable;
    std::vector<jc3::CSkinBatch>     m_SkinBatches;
    std::string                      m_ShaderName              = "carpaintmm";
    std::array<VertexBuffer_t*, 2>   m_VertexBufferData        = {nullptr};
    std::array<ConstantBuffer_t*, 3> m_VertexShaderConstants   = {nullptr};
    std::array<ConstantBuffer_t*, 4> m_FragmentShaderConstants = {nullptr};

  public:
    RenderBlockCarPaintMM() = default;
    virtual ~RenderBlockCarPaintMM();

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockCarPaintMM";
    }

    virtual uint32_t GetTypeHash() const override final;

    virtual bool IsOpaque() override final
    {
        return ~m_Block.m_Attributes.m_Flags & TRANSPARENCY_ALPHABLENDING;
    }

    virtual void Create() override final;

    virtual void Read(std::istream& stream) override final
    {
        using namespace jc::Vertex;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));
        assert(m_Block.m_Version == jc::RenderBlocks::CARPAINTMM_VERSION);

        // read static material params
        stream.read((char*)&m_cbStaticMaterialParams, sizeof(m_cbStaticMaterialParams));

        // read dynamic material params
        stream.read((char*)&m_cbDynamicMaterialParams, sizeof(m_cbDynamicMaterialParams));

        // read the deform table
        ReadDeformTable(stream, &m_DeformTable);

        // read the materials
        ReadMaterials(stream);

        // read the vertex buffers
        if (m_Block.m_Attributes.m_Flags & IS_SKINNED) {
            m_VertexBuffer        = ReadVertexBuffer<UnpackedVertexWithNormal1>(stream);
            m_VertexBufferData[0] = ReadVertexBuffer<UnpackedNormals>(stream);
            m_ShaderName          = "carpaintmm_skinned";
        } else if (m_Block.m_Attributes.m_Flags & IS_DEFORM) {
            m_VertexBuffer        = ReadVertexBuffer<VertexDeformPos>(stream);
            m_VertexBufferData[0] = ReadVertexBuffer<VertexDeformNormal2>(stream);
            m_ShaderName          = "carpaintmm_deform";
        } else {
            m_VertexBuffer        = ReadVertexBuffer<UnpackedVertexPosition>(stream);
            m_VertexBufferData[0] = ReadVertexBuffer<UnpackedNormals>(stream);
        }

        // read skin batch
        if (m_Block.m_Attributes.m_Flags & IS_SKINNED) {
            ReadSkinBatch(stream, &m_SkinBatches);
        }

        // read the layered uv data if needed
        if (m_Block.m_Attributes.m_Flags & (SUPPORT_LAYERED | SUPPORT_OVERLAY)) {
            m_VertexBufferData[1] = ReadVertexBuffer<UnpackedUV>(stream);
        }

        // read index buffer
        m_IndexBuffer = ReadIndexBuffer(stream);
    }

    virtual void Write(std::ostream& stream) override final
    {
        // write the block attributes
        stream.write((char*)&m_Block, sizeof(m_Block));

        // write the static material params
        stream.write((char*)&m_cbStaticMaterialParams, sizeof(m_cbStaticMaterialParams));

        // write the dynamic material params
        stream.write((char*)&m_cbDynamicMaterialParams, sizeof(m_cbDynamicMaterialParams));

        // write the deform table
        WriteDeformTable(stream, &m_DeformTable);

        // write the materials
        WriteMaterials(stream);

        // write the vertex buffer
        WriteBuffer(stream, m_VertexBuffer);
        WriteBuffer(stream, m_VertexBufferData[0]);

        // write skin batch
        if (m_Block.m_Attributes.m_Flags & IS_SKINNED) {
            WriteSkinBatch(stream, &m_SkinBatches);
        }

        // write layered UV data
        if (m_Block.m_Attributes.m_Flags & (SUPPORT_LAYERED | SUPPORT_OVERLAY)) {
            WriteBuffer(stream, m_VertexBufferData[1]);
        }

        // write the index buffer
        WriteBuffer(stream, m_IndexBuffer);
    }

    virtual void Setup(RenderContext_t* context) override final;

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;

        if (m_Block.m_Attributes.m_Flags & IS_SKINNED) {
            IRenderBlock::DrawSkinBatches(context, m_SkinBatches);
        } else {
            // set deform data
            if (m_Block.m_Attributes.m_Flags & IS_DEFORM) {
                // TODO
            }

            IRenderBlock::Draw(context);
        }
    }

    virtual void DrawContextMenu() override final;
    virtual void DrawUI() override final;

    virtual void SetData(vertices_t* vertices, uint16s_t* indices, materials_t* materials) override final;
    virtual std::tuple<vertices_t, uint16s_t> GetData() override final
    {
        using namespace jc::Vertex;

        vertices_t vertices;
        uint16s_t  indices = m_IndexBuffer->CastData<uint16_t>();

        if (m_Block.m_Attributes.m_Flags & IS_SKINNED) {
            const auto& vb     = m_VertexBuffer->CastData<UnpackedVertexWithNormal1>();
            const auto& vbdata = m_VertexBufferData[0]->CastData<UnpackedNormals>();

            assert(vb.size() == vbdata.size());
            vertices.reserve(vb.size());

            for (auto i = 0; i < vb.size(); ++i) {
                vertex_t v;
                v.pos    = {vb[i].x, vb[i].y, vb[i].z};
                v.uv     = {vbdata[i].u0, vbdata[i].v0};
                v.normal = unpack_normal(vbdata[i].n);
                vertices.emplace_back(std::move(v));
            }
        } else if (m_Block.m_Attributes.m_Flags & IS_DEFORM) {
            const auto& vb     = m_VertexBuffer->CastData<VertexDeformPos>();
            const auto& vbdata = m_VertexBufferData[0]->CastData<VertexDeformNormal2>();

            assert(vb.size() == vbdata.size());
            vertices.reserve(vb.size());

            for (auto i = 0; i < vb.size(); ++i) {
                vertex_t v;
                v.pos    = {vb[i].x, vb[i].y, vb[i].z};
                v.uv     = {vbdata[i].u0, vbdata[i].v0};
                v.normal = unpack_normal(vbdata[i].n);
                vertices.emplace_back(std::move(v));
            }
        } else {
            const auto& vb     = m_VertexBuffer->CastData<UnpackedVertexPosition>();
            const auto& vbdata = m_VertexBufferData[0]->CastData<UnpackedNormals>();

            assert(vb.size() == vbdata.size());
            vertices.reserve(vb.size());

            for (auto i = 0; i < vb.size(); ++i) {
                vertex_t v;
                v.pos    = {vb[i].x, vb[i].y, vb[i].z};
                v.uv     = {vbdata[i].u0, vbdata[i].v0};
                v.normal = unpack_normal(vbdata[i].n);
                vertices.emplace_back(std::move(v));
            }
        }

#if 0
        if (m_Block.m_Attributes.m_Flags & (SUPPORT_LAYERED | SUPPORT_OVERLAY)) {
            const auto& uvs = m_VertexBufferData[1]->CastData<UnpackedUV>();
            // TODO: u,v
        }
#endif

        return {vertices, indices};
    }

    rb_textures_t GetTextures() override final
    {
        rb_textures_t result;
        result.push_back({"diffuse", m_Textures[0]});
        result.push_back({"normal", m_Textures[1]});
        return result;
    }
};
} // namespace jc3
