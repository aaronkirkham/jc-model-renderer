#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct CharacterSkinAttributes
{
    uint32_t flags;
    float scale;
    char pad[0x30];
};

namespace JustCause3
{
    namespace RenderBlocks
    {
        struct CharacterSkin
        {
            uint8_t version;
            CharacterSkinAttributes attributes;
        };
    };
};
#pragma pack(pop)

class RenderBlockCharacterSkin : public IRenderBlock
{
private:
    JustCause3::RenderBlocks::CharacterSkin m_Block;

public:
    RenderBlockCharacterSkin() = default;
    virtual ~RenderBlockCharacterSkin() = default;

    virtual const char* GetTypeName() override final { return "RenderBlockCharacterSkin"; }

    virtual void Create() override final
    {
    }

    virtual void Read(fs::path& filename, std::istream& stream) override final
    {
        // read the block header
        stream.read((char *)&m_Block, sizeof(m_Block));

        // read the materials
        ReadMaterials(filename, stream);

        // read vertex data
        // TODO: need to implement the different vertex types depending on the flags above (GetStride).
        // The stride should be the size of the struct we read from.
        {
            ReadVertexBuffer<JustCause3::Vertex::RenderBlockCharacterSkin::PackedCharacterSkinPos4Bones1UVs>(stream, &m_Vertices);

#if 0
            std::vector<JustCause3::Vertex::RenderBlockCharacterSkin::PackedCharacterSkinPos4Bones1UVs> vb_data;
            ReadVertexBuffer(file, &vb_data);

            std::vector<Vertex> vertices;
            vertices.reserve(vb_data.size());

            for (auto& vertex : vb_data) {
                Vertex v;
                v.position = { JustCause3::Vertex::expand(vertex.x), JustCause3::Vertex::expand(vertex.y), JustCause3::Vertex::expand(vertex.z) };
                v.uv = { JustCause3::Vertex::expand(vertex.u0), JustCause3::Vertex::expand(vertex.v0) };

                v.position *= m_Block.attributes.scale;

                vertices.emplace_back(v);
            }

            m_VertexBuffer->Create(vertices);
#endif
        }

        // read skin batch
        ReadSkinBatch(stream);

        // read index buffer
        ReadIndexBuffer(stream, &m_Indices);
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        context->m_CullFace = static_cast<D3D11_CULL_MODE>(2 * (~LOBYTE(m_Block.attributes.flags) & 1) | 1);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
    }
};