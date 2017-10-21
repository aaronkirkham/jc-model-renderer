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

namespace JustCause3::RenderBlocks
{
    struct Character
    {
        uint8_t version;
        CharacterAttributes attributes;
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
        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader("Character");
        m_PixelShader = ShaderManager::Get()->GetPixelShader("Character");

        // create the element input desc
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R16G16B16A16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R16G16_SINT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        // create the vertex declaration
        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 2, m_VertexShader.get());

        // create the sampler states
        {
            SamplerStateCreationParams_t params;
            params.m_AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_MinMip = 12.0f;
            params.m_MaxMip = 13.0f;

            m_SamplerState = Renderer::Get()->CreateSamplerState(params);
        }
    }

    virtual void Read(fs::path& filename, std::istream& stream) override final
    {
        // read the block header
        stream.read((char *)&m_Block, sizeof(m_Block));

        // set vertex shader constants
        m_Constants.scale = m_Block.attributes.scale;

        // read the materials
        ReadMaterials(filename, stream);

        // read vertex data
        // TODO: need to implement the different vertex types depending on the flags above (GetStride).
        // The stride should be the size of the struct we read from.
        {
            ReadVertexBuffer<JustCause3::Vertex::RenderBlockCharacter::PackedCharacterPos4Bones1UVs>(stream, &m_Vertices);
        }

        // read skin batch
        ReadSkinBatch(stream);

        // read index buffer
        ReadIndexBuffer(stream, &m_Indices);
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        IRenderBlock::Setup(context);

        Renderer::Get()->SetCullMode((!(m_Block.attributes.flags & 1)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        DrawSkinBatches(context);
    }
};