#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct CharacterAttributes
{
    uint32_t flags;
    float scale;
    char pad[0x64];
};

namespace JustCause3
{
    namespace RenderBlocks
    {
        struct Character
        {
            uint8_t version;
            CharacterAttributes attributes;
        };
    };
};
#pragma pack(pop)

class RenderBlockCharacter : public IRenderBlock
{
private:
    JustCause3::RenderBlocks::Character m_Block;

public:
    RenderBlockCharacter() = default;
    virtual ~RenderBlockCharacter() = default;

    virtual const char* GetTypeName() override final { return "RenderBlockCharacter"; }

    virtual void Create() override final
    {
    }

    virtual void Read(fs::path& filename, std::ifstream& file) override final
    {
#if 0
        // read the block header
        file.read((char *)&m_Block, sizeof(m_Block));

        // read the materials
        ReadMaterials(filename, file);

        // read vertex data
        // TODO: need to implement the different vertex types depending on the flags above (GetStride).
        // The stride should be the size of the struct we read from.
        {
            std::vector<JustCause3::Vertex::RenderBlockCharacter::PackedCharacterPos4Bones1UVs> vb_data;
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
        }

        // read skin batch
        ReadSkinBatch(file);

        // read index buffer
        ReadIndexBuffer(file);
#endif
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        context->m_CullFace = static_cast<D3D11_CULL_MODE>(2 * (~LOBYTE(m_Block.attributes.flags) & 1) | 1);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
#if 0
        assert(m_VertexBuffer->Created());
        assert(m_IndexBuffer->Created());

        m_VertexShader->Activate();
        m_PixelShader->Activate();

        // bind the textures
        for (uint32_t i = 0; i < m_Textures.size(); ++i) {
            auto texture = m_Textures[i];
            if (texture && texture->IsLoaded()) {
                texture->Use(i);
            }
        }

        m_VertexBuffer->Use(0);

        Renderer::Get()->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for (auto& batch : m_SkinBatches) {
            Renderer::Get()->DrawIndexed(batch.offset, batch.size / 3, m_IndexBuffer.get());
        }
#endif
    }
};