#pragma once

#include "irenderblock.h"

#pragma pack(push, 1)
struct PropAttributes {
    float                        depthBias;
    char                         _pad[0xFC];
    jc::Vertex::SPackedAttribute packed;
    uint32_t                     flags;
};

namespace jc::RenderBlocks
{
static constexpr uint8_t PROP_VERSION = 3;

struct Prop {
    uint8_t        version;
    PropAttributes attributes;
};
}; // namespace jc::RenderBlocks
#pragma pack(pop)

namespace jc3
{
class RenderBlockProp : public IRenderBlock
{
  private:
    enum { DISABLE_BACKFACE_CULLING = 1, TRANSPARENCY_ALPHABLENDING = 2, SUPPORT_DECALS = 4, SUPPORT_OVERLAY = 8 };

    struct cbVertexInstanceConsts {
        glm::mat4   ViewProjection;
        glm::mat4   World;
        glm::mat4x3 Unknown = glm::mat4x3(1);
        glm::vec4   Colour;
        glm::vec4   Unknown2;
    } m_cbVertexInstanceConsts;

    struct cbVertexMaterialConsts {
        glm::vec4 DepthBias;
        glm::vec2 uv0Extent;
        glm::vec2 uv1Extent;
    } m_cbVertexMaterialConsts;

    struct cbFragmentMaterialConsts {
        glm::vec4 _pad[16];
    } m_cbFragmentMaterialConsts;

    jc::RenderBlocks::Prop           m_Block;
    VertexBuffer_t*                  m_VertexBufferData        = nullptr;
    std::array<ConstantBuffer_t*, 2> m_VertexShaderConstants   = {nullptr};
    ConstantBuffer_t*                m_FragmentShaderConstants = nullptr;

  public:
    RenderBlockProp() = default;
    virtual ~RenderBlockProp();

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockProp";
    }

    virtual uint32_t GetTypeHash() const override final;

    virtual bool IsOpaque() override final
    {
        const auto flags = m_Block.attributes.flags;
        return !(flags & TRANSPARENCY_ALPHABLENDING) && !(flags & SUPPORT_OVERLAY);
    }

    virtual void Create() override final;

    virtual void Read(std::istream& stream) override final
    {
        using namespace jc::Vertex;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));

        if (m_Block.version != jc::RenderBlocks::PROP_VERSION) {
            __debugbreak();
        }

        // read the block materials
        ReadMaterials(stream);

        // read the vertex buffers
        if (m_Block.attributes.packed.format != 1) {
            m_VertexBuffer = ReadVertexBuffer<UnpackedVertexPosition2UV>(stream);
        } else {
            m_VertexBuffer     = ReadVertexBuffer<PackedVertexPosition>(stream);
            m_VertexBufferData = ReadVertexBuffer<GeneralShortPacked>(stream);
        }

        // read index buffer
        m_IndexBuffer = ReadIndexBuffer(stream);
    }

    virtual void Write(std::ostream& stream) override final
    {
        // write the block attributes
        stream.write((char*)&m_Block, sizeof(m_Block));

        // write the materials
        WriteMaterials(stream);

        // write vertex buffers
        if (m_Block.attributes.packed.format != 1) {
            WriteBuffer(stream, m_VertexBuffer);
        } else {
            WriteBuffer(stream, m_VertexBuffer);
            WriteBuffer(stream, m_VertexBufferData);
        }

        // write index buffer
        WriteBuffer(stream, m_IndexBuffer);
    }

    virtual void Setup(RenderContext_t* context) override final;

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;

        IRenderBlock::Draw(context);
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

        if (m_Block.attributes.packed.format != 1) {
            const auto& vb = m_VertexBuffer->CastData<UnpackedVertexPosition2UV>();
            vertices.reserve(vb.size());

            for (const auto& vertex : vb) {
                vertex_t v;
                v.pos    = {vertex.x, vertex.y, vertex.z};
                v.uv     = glm::vec2{unpack(vertex.u0), unpack(vertex.v0)} * m_Block.attributes.packed.uv0Extent;
                v.normal = unpack_normal(vertex.n);
                vertices.emplace_back(std::move(v));
            }
        } else {
            const auto& vb     = m_VertexBuffer->CastData<PackedVertexPosition>();
            const auto& vbdata = m_VertexBufferData->CastData<GeneralShortPacked>();
            vertices.reserve(vb.size());

            for (auto i = 0; i < vb.size(); ++i) {
                auto& vertex = vb[i];
                auto& data   = vbdata[i];

                vertex_t v;
                v.pos    = {unpack(vertex.x), unpack(vertex.y), unpack(vertex.z)};
                v.uv     = glm::vec2{unpack(data.u0), unpack(data.v0)} * m_Block.attributes.packed.uv0Extent;
                v.normal = unpack_normal(data.n);
                vertices.emplace_back(std::move(v));
            }
        }

        return {vertices, indices};
    }
};
} // namespace jc3
