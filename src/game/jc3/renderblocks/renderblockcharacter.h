#pragma once

#include "irenderblock.h"

extern bool g_IsJC4Mode;

#pragma pack(push, 1)
struct CharacterAttributes {
    uint32_t  flags;
    float     scale;
    glm::vec2 _unknown;
    glm::vec2 _unknown2;
    float     _unknown3;
    float     specularGloss;
    float     transmission;
    float     specularFresnel;
    float     diffuseRoughness;
    float     diffuseWrap;
    float     _unknown4;
    float     dirtFactor;
    float     emissive;
    char      pad2[0x30];
};

static_assert(sizeof(CharacterAttributes) == 0x6C, "CharacterAttributes alignment is wrong!");

namespace jc::RenderBlocks
{
static constexpr uint8_t CHARACTER_VERSION = 9;

struct Character {
    uint8_t             version;
    CharacterAttributes attributes;
};
}; // namespace jc::RenderBlocks
#pragma pack(pop)

namespace jc3
{
class RenderBlockCharacter : public jc3::IRenderBlock
{
  private:
    enum {
        DISABLE_BACKFACE_CULLING   = 0x1,
        EIGHT_BONES                = 0x2,
        ALPHA_MASK                 = 0x4,
        TRANSPARENCY_ALPHABLENDING = 0x8,
        USE_FEATURE_MAP            = 0x10,
        USE_WRINKLE_MAP            = 0x20,
        USE_CAMERA_LIGHTING        = 0x40,
        USE_EYE_REFLECTION         = 0x80,
    };

    enum CharacterAttributeFlags {
        GEAR      = 0x0,
        EYES      = 0x1000,
        HAIR      = 0x2000,
        BODY_PART = 0x3000,
    };

    struct cbLocalConsts {
        glm::mat4   World;
        glm::mat4   WorldViewProjection;
        glm::vec4   Scale;
        glm::mat3x4 MatrixPalette[70];
    } m_cbLocalConsts;

    struct cbInstanceConsts {
        glm::vec4 _unknown      = glm::vec4(0);
        glm::vec4 DiffuseColour = glm::vec4(0, 0, 0, 1);
        glm::vec4 _unknown2     = glm::vec4(0); // .w is some kind of snow factor???
    } m_cbInstanceConsts;

    struct cbMaterialConsts {
        glm::vec4 unknown[10];
    } m_cbMaterialConsts;

    jc::RenderBlocks::Character      m_Block;
    std::vector<jc3::CSkinBatch>     m_SkinBatches;
    ConstantBuffer_t*                m_VertexShaderConstants = nullptr;
    std::array<ConstantBuffer_t*, 2> m_PixelShaderConstants  = {nullptr};
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
        const auto flags = m_Block.attributes.flags;
        return ((flags & BODY_PART) == GEAR || (flags & BODY_PART) == HAIR) && !(flags & TRANSPARENCY_ALPHABLENDING);
    }

    void CreateBuffers(std::vector<uint8_t>* vertex_buffer, std::vector<uint8_t>* index_buffer);

    virtual void Create() override final;

    virtual void Read(std::istream& stream) override final
    {
        using namespace jc::Vertex;
        using namespace jc::Vertex::RenderBlockCharacter;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));

        if (m_Block.version != jc::RenderBlocks::CHARACTER_VERSION) {
            __debugbreak();
        }

        // read the materials
        ReadMaterials(stream);

        // get the vertices stride
        const auto flags = m_Block.attributes.flags;
        m_Stride         = (3 * ((flags >> 1) & 1) + ((flags >> 4) & 1) + ((flags >> 5) & 1));

        // read vertex data
        m_VertexBuffer = ReadBuffer(stream, VERTEX_BUFFER, VertexStrides[m_Stride]);

        // read skin batches
        ReadSkinBatch(stream, &m_SkinBatches);

        // read index buffer
        m_IndexBuffer = ReadIndexBuffer(stream);
    }

    virtual void Write(std::ostream& stream) override final
    {
        // write the block attributes
        stream.write((char*)&m_Block, sizeof(m_Block));

        // write the materials
        WriteMaterials(stream);

        // write the vertex data
        WriteBuffer(stream, m_VertexBuffer);

        // write skin batches
        WriteSkinBatch(stream, &m_SkinBatches);

        // write index buffer
        WriteBuffer(stream, m_IndexBuffer);
    }

    virtual void Setup(RenderContext_t* context) override final;

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible) {
            return;
        }

        IRenderBlock::DrawSkinBatches(context, m_SkinBatches);
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
        using namespace jc::Vertex::RenderBlockCharacter;

        vertices_t vertices;
        uint16s_t  indices = m_IndexBuffer->CastData<uint16_t>();

        switch (m_Stride) {
            // 4bones1uv, 4bones2uvs, 4bones3uvs
            case 0:
            case 1:
            case 2: {
                // TODO: once multiple UVs are supported, change this!
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

            // 8bones1uv, 8bones2uvs, 8bones3uvs
            case 3:
            case 4:
            case 5: {
                // TODO: once multiple UVs are supported, change this!
                const auto& vb = m_VertexBuffer->CastData<Packed8Bones1UV>();
                vertices.reserve(vb.size());
                for (const auto& vertex : vb) {
                    vertex_t v{};
                    v.pos = glm::vec3{unpack(vertex.x), unpack(vertex.y), unpack(vertex.z)};
                    v.uv  = glm::vec2{unpack(vertex.u0), unpack(vertex.v0)};
                    vertices.emplace_back(std::move(v));
                }

                break;
            }
        }

        return {vertices, indices};
    }
};
} // namespace jc3
