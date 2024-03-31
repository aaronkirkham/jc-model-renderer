#pragma once

#include "irenderblock.h"

#pragma pack(push, 1)
struct GeneralAttributes {
    float                        m_DepthBias;
    float                        m_SpecularGloss;
    jc::Vertex::SPackedAttribute m_Packed;
    uint32_t                     m_Flags;
    glm::vec3                    m_BoundingBoxMin;
    glm::vec3                    m_BoundingBoxMax;
};

namespace jc::RenderBlocks
{
static constexpr uint8_t GENERAL_VERSION = 6;

struct General {
    uint8_t           m_Version;
    GeneralAttributes m_Attributes;
};
}; // namespace jc::RenderBlocks
#pragma pack(pop)

namespace jc3
{
class RenderBlockGeneral : public IRenderBlock
{
  private:
    enum { DISABLE_BACKFACE_CULLING = 0x1, TRANSPARENCY_ALPHABLENDING = 0x2, TRANSPARENCY_ALPHATEST = 0x16 };

    struct cbVertexInstanceConsts {
        glm::mat4   ViewProjection;
        glm::mat4   World;
        glm::vec4   _unknown[3];
        glm::vec4   _thing;
        glm::mat4x3 _thing2;
    } m_cbVertexInstanceConsts;

    struct cbVertexMaterialConsts {
        glm::vec4 DepthBias;
        glm::vec2 uv0Extent;
        glm::vec2 uv1Extent;
    } m_cbVertexMaterialConsts;

    struct cbFragmentMaterialConsts {
        float specularGloss;
        float _unknown   = 1.0f;
        float _unknown2  = 0.0f;
        float _unknown3  = 1.0f;
        float _unknown4  = 0.0f;
        float _unknown5  = 0.0f;
        float _unknown6  = 0.0f;
        float _unknown7  = 0.0f;
        float _unknown8  = 0.0f;
        float _unknown9  = 0.0f;
        float _unknown10 = 0.0f;
        float _unknown11 = 0.0f;
    } m_cbFragmentMaterialConsts;

    jc::RenderBlocks::General        m_Block;
    VertexBuffer_t*                  m_VertexBufferData       = nullptr;
    std::array<ConstantBuffer_t*, 2> m_VertexShaderConstants  = {nullptr};
    ConstantBuffer_t*                m_FragmentShaderConstant = nullptr;

  public:
    RenderBlockGeneral() = default;
    virtual ~RenderBlockGeneral();

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockGeneral";
    }

    virtual uint32_t GetTypeHash() const override final;

    virtual bool IsOpaque() override final
    {
        return ~m_Block.m_Attributes.m_Flags & TRANSPARENCY_ALPHABLENDING;
    }

    void CreateBuffers(std::vector<uint8_t>* vertex_buffer, std::vector<uint8_t>* vertex_data_buffer,
                       std::vector<uint8_t>* index_buffer);

    virtual void Create() override final;

    virtual void Read(std::istream& stream) override final
    {
        using namespace jc::Vertex;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));
        assert(m_Block.m_Version == jc::RenderBlocks::GENERAL_VERSION);

        // read the block materials
        ReadMaterials(stream);

        // read the vertex buffers
        if (m_Block.m_Attributes.m_Packed.m_Format == PACKED_FORMAT_FLOAT) {
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
        using namespace jc::Vertex;

        // write the block attributes
        stream.write((char*)&m_Block, sizeof(m_Block));

        // write the materials
        WriteMaterials(stream);

        // write vertex buffers
        if (m_Block.m_Attributes.m_Packed.m_Format == PACKED_FORMAT_FLOAT) {
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

        if (m_Block.m_Attributes.m_Packed.m_Format == PACKED_FORMAT_FLOAT) {
            const auto& vb = m_VertexBuffer->CastData<UnpackedVertexPosition2UV>();
            vertices.reserve(vb.size());

            for (const auto& vertex : vb) {
                vertex_t v;
                v.pos    = {vertex.x, vertex.y, vertex.z};
                v.uv     = glm::vec2{unpack(vertex.u0), unpack(vertex.v0)} * m_Block.m_Attributes.m_Packed.m_UV0Extent;
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
                v.uv     = glm::vec2{unpack(data.u0), unpack(data.v0)} * m_Block.m_Attributes.m_Packed.m_UV0Extent;
                v.normal = unpack_normal(data.n);
                vertices.emplace_back(std::move(v));
            }
        }

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
