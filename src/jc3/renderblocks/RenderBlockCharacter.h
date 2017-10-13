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
    VertexDeclaration_t* m_VertexDeclaration = nullptr;

public:
    RenderBlockCharacter() = default;
    virtual ~RenderBlockCharacter()
    {
        Renderer::Get()->DestroyVertexDeclaration(m_VertexDeclaration);
    }

    virtual const char* GetTypeName() override final { return "RenderBlockCharacter"; }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader("Character");
        m_PixelShader = ShaderManager::Get()->GetPixelShader("Character");

        // create the element input desc
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R16G16B16A16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        // create the vertex declaration
        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 1, m_VertexShader.get());
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
            ReadVertexBuffer<JustCause3::Vertex::RenderBlockCharacter::PackedCharacterPos4Bones1UVs>(stream, &m_Vertices);
#if 0
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
#endif
        }

        // read skin batch
        ReadSkinBatch(stream);

        // read index buffer
        ReadIndexBuffer(stream, &m_Indices);
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        // enable the vertex and pixel shaders
        context->m_DeviceContext->IASetInputLayout(m_VertexDeclaration->m_Layout);
        context->m_DeviceContext->VSSetShader(m_VertexShader->m_Shader, nullptr, 0);
        context->m_DeviceContext->PSSetShader(m_PixelShader->m_Shader, nullptr, 0);

        context->m_CullFace = static_cast<D3D11_CULL_MODE>(2 * (~LOBYTE(m_Block.attributes.flags) & 1) | 1);

        // set the 1st vertex buffers
        uint32_t offset = 0;
        context->m_DeviceContext->IASetVertexBuffers(0, 1, &m_Vertices->m_Buffer, &m_Vertices->m_ElementStride, &offset);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        IRenderBlock::Draw(context);

        context->m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        if (m_Indices) {
            Renderer::Get()->DrawIndexed(0, m_Indices->m_ElementCount, m_Indices);
        }
        else {
            Renderer::Get()->Draw(0, m_Vertices->m_ElementCount / 3);
        }
    }
};