#pragma once

#include "irenderblock.h"

#pragma pack(push, 1)
struct GeneralJC3Attributes {
    float                        depthBias;
    float                        specularGloss;
    float                        reflectivity;
    float                        emmissive;
    float                        diffuseWrap;
    float                        specularFresnel;
    glm::vec4                    diffuseModulator;
    float                        backLight;
    float                        _unknown2;
    float                        startFadeOutDistanceEmissiveSq;
    jc::Vertex::SPackedAttribute packed;
    uint32_t                     flags;
};

static_assert(sizeof(GeneralJC3Attributes) == 0x58, "GeneralJC3Attributes alignment is wrong!");

namespace jc::RenderBlocks
{
static constexpr uint8_t GENERALJC3_VERSION = 6;

struct GeneralJC3 {
    uint8_t              version;
    GeneralJC3Attributes attributes;
};
}; // namespace jc::RenderBlocks
#pragma pack(pop)

namespace jc3
{
class RenderBlockGeneralJC3 : public IRenderBlock
{
  private:
    enum {
        DISABLE_BACKFACE_CULLING   = 0x1,
        TRANSPARENCY_ALPHABLENDING = 0x2,
        DYNAMIC_EMISSIVE           = 0x4,
    };

    struct cbVertexInstanceConsts {
        glm::mat4 viewProjection;
        glm::mat4 world;
        char      _pad[0x20];
        char      _pad2[0x10];
        glm::vec4 colour;
        glm::vec4 _unknown[3];
        glm::vec4 _thing;
    } m_cbVertexInstanceConsts;

    struct cbVertexMaterialConsts {
        float     DepthBias;
        float     EmissiveStartFadeDistSq;
        float     _unknown;
        float     _unknown2;
        glm::vec2 uv0Extent;
        glm::vec2 uv1Extent;
    } m_cbVertexMaterialConsts;

    struct cbFragmentMaterialConsts {
        float     specularGloss;
        float     reflectivity;
        float     _unknown;
        float     specularFresnel;
        glm::vec4 diffuseModulator;
        float     _unknown2;
        float     _unknown3;
        float     _unknown4;
        float     _unknown5;
    } m_cbFragmentMaterialConsts;

    struct cbFragmentInstanceConsts {
        glm::vec4 colour;
    } m_cbFragmentInstanceConsts;

    jc::RenderBlocks::GeneralJC3     m_Block;
    VertexBuffer_t*                  m_VertexBufferData        = nullptr;
    std::array<ConstantBuffer_t*, 2> m_VertexShaderConstants   = {nullptr};
    std::array<ConstantBuffer_t*, 2> m_FragmentShaderConstants = {nullptr};

  public:
    RenderBlockGeneralJC3() = default;
    virtual ~RenderBlockGeneralJC3();

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockGeneralJC3";
    }

    virtual uint32_t GetTypeHash() const override final;

    virtual bool IsOpaque() override final
    {
        return ~m_Block.attributes.flags & TRANSPARENCY_ALPHABLENDING;
    }

    virtual void Create() override final;

    virtual void Read(std::istream& stream) override final
    {
        using namespace jc::Vertex;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));

        if (m_Block.version != jc::RenderBlocks::GENERALJC3_VERSION) {
            __debugbreak();
        }

        // read the materials
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

                // TODO: u1,v1
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

                // TODO: u1,v1
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
