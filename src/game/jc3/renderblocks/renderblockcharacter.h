#pragma once

#include "irenderblock.h"

extern bool g_IsJC4Mode;

#pragma pack(push, 1)
struct CharacterAttributes {
    uint32_t  m_Flags;
    float     m_Scale;
    glm::vec2 m_DetailTilingFactorUV;
    glm::vec2 m_DecalBlendFactors;
    float     _unknown;
    float     m_SpecularGloss;
    float     m_Transmission;
    float     m_SpecularFresnel;
    float     m_DiffuseRoughness;
    float     m_DiffuseWrap;
    float     _unknown2;
    float     m_DirtFactor;
    float     m_Emissive;
    char      pad2[0x30];
};

static_assert(sizeof(CharacterAttributes) == 0x6C, "CharacterAttributes alignment is wrong!");

namespace jc::RenderBlocks
{
static constexpr uint8_t CHARACTER_VERSION = 9;

struct Character {
    uint8_t             m_Version;
    CharacterAttributes m_Attributes;
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

    static_assert(sizeof(cbLocalConsts) == 0xDB0);

    struct cbInstanceConsts {
        float     _unknown              = 0.0f;
        float     _unknown2             = 0.0f;
        float     EnableHairDebug       = 0.0f;
        float     EnableCameraHairLight = 0.0f;
        glm::vec4 DiffuseColour         = glm::vec4(0, 0, 0, 1);
        float     _unknown3             = 0.0f;
        float     _unknown4             = 0.0f;
        float     _unknown5             = 0.0f;
        float     SnowFactor            = 0.0f;
    } m_cbInstanceConsts;

    static_assert(sizeof(cbInstanceConsts) == 0x30);

    struct cbMaterialConsts {
        glm::vec2 DetailTilingFactorUV;
        glm::vec2 DecalBlendFactors;
        float     _unknown;
        float     SpecularGloss;
        float     Transmission;
        float     SpecularFresnel;
        float     DiffuseWrap;
        float     DiffuseRoughness;
        float     DirtFactor;
        float     Emissive;
        glm::vec4 unknown[7];
    } m_cbMaterialConsts;

    static_assert(sizeof(cbMaterialConsts) == 0xA0);

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
        const auto flags = m_Block.m_Attributes.m_Flags;
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

        if (m_Block.m_Version != jc::RenderBlocks::CHARACTER_VERSION) {
            __debugbreak();
        }

        // read the materials
        ReadMaterials(stream);

        // get the vertices stride
        const auto flags = m_Block.m_Attributes.m_Flags;
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

    rb_textures_t GetTextures() override final
    {
        rb_textures_t result;
        result.push_back({"diffuse", m_Textures[0]});

        const auto flags = m_Block.m_Attributes.m_Flags;
        if ((flags & BODY_PART) == GEAR || (flags & BODY_PART) == HAIR) {
            result.push_back({"normal", m_Textures[1]});
        }

        return result;
    }
};
} // namespace jc3
