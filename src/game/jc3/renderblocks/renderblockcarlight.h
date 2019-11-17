#pragma once

#include "irenderblock.h"

#pragma pack(push, 1)
struct CarLightAttributes {
    float     _unused;
    float     specularGloss;
    float     reflectivity;
    char      _pad[16];
    float     specularFresnel;
    glm::vec4 diffuseModulator;
    glm::vec2 tilingUV;
    uint32_t  flags;
};

static_assert(sizeof(CarLightAttributes) == 0x3C, "CarLightAttributes alignment is wrong!");

namespace jc::RenderBlocks
{
static constexpr uint8_t CARLIGHT_VERSION = 1;

struct CarLight {
    uint8_t            version;
    CarLightAttributes attributes;
};
}; // namespace jc::RenderBlocks
#pragma pack(pop)

namespace jc3
{
class RenderBlockCarLight : public IRenderBlock
{
  private:
    enum {
        DISABLE_BACKFACE_CULLING = 0x1,
        IS_DEFORM                = 0x4,
        IS_SKINNED               = 0x8,
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

    struct MaterialConsts {
        float     SpecularGloss;
        float     Reflectivity;
        float     SpecularFresnel;
        float     _unknown;
        glm::vec4 DiffuseModulator;
        glm::vec4 TilingUV;
        glm::vec4 Colour = glm::vec4(1);
    } m_cbMaterialConsts;

    jc::RenderBlocks::CarLight       m_Block;
    jc3::CDeformTable                m_DeformTable;
    std::vector<jc3::CSkinBatch>     m_SkinBatches;
    VertexBuffer_t*                  m_VertexBufferData        = nullptr;
    std::array<ConstantBuffer_t*, 3> m_VertexShaderConstants   = {nullptr};
    ConstantBuffer_t*                m_FragmentShaderConstants = nullptr;

  public:
    RenderBlockCarLight() = default;
    virtual ~RenderBlockCarLight();

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockCarLight";
    }

    virtual uint32_t GetTypeHash() const override final;

    virtual bool IsOpaque() override final
    {
        return true;
    }

    virtual void Create() override final;

    virtual void Read(std::istream& stream) override final
    {
        using namespace jc::Vertex;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));

        if (m_Block.version != jc::RenderBlocks::CARLIGHT_VERSION) {
            __debugbreak();
        }

        // read the deform table
        ReadDeformTable(stream, &m_DeformTable);

        // read materials
        ReadMaterials(stream);

        // read vertex buffers
        if (m_Block.attributes.flags & IS_SKINNED) {
            m_VertexBuffer     = ReadVertexBuffer<UnpackedVertexWithNormal1>(stream);
            m_VertexBufferData = ReadVertexBuffer<UnpackedNormals>(stream);
        } else if (m_Block.attributes.flags & IS_DEFORM) {
            m_VertexBuffer     = ReadVertexBuffer<VertexDeformPos>(stream);
            m_VertexBufferData = ReadVertexBuffer<VertexDeformNormal2>(stream);
        } else {
            m_VertexBuffer     = ReadVertexBuffer<UnpackedVertexPosition>(stream);
            m_VertexBufferData = ReadVertexBuffer<UnpackedNormals>(stream);
        }

        // read skin batches
        if (m_Block.attributes.flags & IS_SKINNED) {
            ReadSkinBatch(stream, &m_SkinBatches);
        }

        // read index buffer
        m_IndexBuffer = ReadIndexBuffer(stream);
    }

    virtual void Write(std::ostream& stream) override final
    {
        // write the block attributes
        stream.write((char*)&m_Block, sizeof(m_Block));

        // write the deform table
        WriteDeformTable(stream, &m_DeformTable);

        // write the matierls
        WriteMaterials(stream);

        // write the vertex buffer
        WriteBuffer(stream, m_VertexBuffer);
        WriteBuffer(stream, m_VertexBufferData);

        // write skin batches
        if (m_Block.attributes.flags & IS_SKINNED) {
            WriteSkinBatch(stream, &m_SkinBatches);
        }

        // write the index buffer
        WriteBuffer(stream, m_IndexBuffer);
    }

    virtual void Setup(RenderContext_t* context) override final;

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible) {
            return;
        }

        if (m_Block.attributes.flags & IS_SKINNED) {
            IRenderBlock::DrawSkinBatches(context, m_SkinBatches);
        } else {
            IRenderBlock::Draw(context);
        }
    }

    virtual void DrawContextMenu() override final;
    virtual void DrawUI() override final;

    virtual void SetData(vertices_t* vertices, uint16s_t* indices, materials_t* materials) override final
    {
        //
    }

    virtual std::tuple<vertices_t, uint16s_t> GetData() override final
    {
        using namespace jc::Vertex;

        vertices_t vertices;
        uint16s_t  indices = m_IndexBuffer->CastData<uint16_t>();

        if (m_Block.attributes.flags & IS_SKINNED) {
            const auto& vb     = m_VertexBuffer->CastData<UnpackedVertexWithNormal1>();
            const auto& vbdata = m_VertexBufferData->CastData<UnpackedNormals>();

            assert(vb.size() == vbdata.size());
            vertices.reserve(vb.size());

            for (auto i = 0; i < vb.size(); ++i) {
                vertex_t v;
                v.pos    = {vb[i].x, vb[i].y, vb[i].z};
                v.uv     = {vbdata[i].u0, vbdata[i].v0};
                v.normal = unpack_normal(vbdata[i].n);
                vertices.emplace_back(std::move(v));
            }
        } else if (m_Block.attributes.flags & IS_DEFORM) {
            const auto& vb     = m_VertexBuffer->CastData<VertexDeformPos>();
            const auto& vbdata = m_VertexBufferData->CastData<VertexDeformNormal2>();

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
            const auto& vbdata = m_VertexBufferData->CastData<UnpackedNormals>();

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

        return {vertices, indices};
    }
};
} // namespace jc3
