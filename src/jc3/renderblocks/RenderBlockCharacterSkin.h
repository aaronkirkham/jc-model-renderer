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

namespace JustCause3::RenderBlocks
{
    struct CharacterSkin
    {
        uint8_t version;
        CharacterSkinAttributes attributes;
    };
};
#pragma pack(pop)

class RenderBlockCharacterSkin : public IRenderBlock
{
private:
    struct CharacterSkinConstants
    {
        float m_Scale = 1.0f;
        char pad[12];
    } m_Constants;

    JustCause3::RenderBlocks::CharacterSkin m_Block;
    ConstantBuffer_t* m_ConstantBuffer = nullptr;

public:
    RenderBlockCharacterSkin() = default;
    virtual ~RenderBlockCharacterSkin()
    {
        Renderer::Get()->DestroyBuffer(m_ConstantBuffer);
    }

    virtual const char* GetTypeName() override final { return "RenderBlockCharacterSkin"; }

    virtual void Create() override final
    {
#if 0
        // load shaders
        m_VertexShader = GET_VERTEX_SHADER(character);
        m_PixelShader = GET_PIXEL_SHADER(character);

        // create the element input desc
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R16G16B16A16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R16G16_SINT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        // create the vertex declaration
        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 2, m_VertexShader.get(), "RenderBlockCharacterSkin");

        // create the constant buffer
        m_ConstantBuffer = Renderer::Get()->CreateConstantBuffer(m_Constants, "RenderBlockCharacterSkin");

        // create the sampler states
        {
            SamplerStateCreationParams_t params;
            params.m_AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_MinMip = 12.0f;
            params.m_MaxMip = 13.0f;

            m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockCharacterSkin");
        }
#endif
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace JustCause3::Vertex;
        using namespace JustCause3::Vertex::RenderBlockCharacterSkin;

        // read the block header
        stream.read((char *)&m_Block, sizeof(m_Block));

        // read the materials
        ReadMaterials(stream);

        // read vertex data
        // TODO: need to implement the different vertex types depending on the flags above (GetStride).
        // The stride should be the size of the struct we read from.
        {
            std::vector<PackedCharacterSkinPos4Bones1UVs> vertices;
            ReadVertexBuffer<PackedCharacterSkinPos4Bones1UVs>(stream, &m_VertexBuffer, &vertices);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(unpack(vertex.x));
                m_Vertices.emplace_back(unpack(vertex.y));
                m_Vertices.emplace_back(unpack(vertex.z));
                m_UVs.emplace_back(unpack(vertex.u0));
                m_UVs.emplace_back(unpack(vertex.v0));
            }
        }

        // read skin batch
        ReadSkinBatch(stream);

        // read index buffer
        ReadIndexBuffer(stream, &m_IndexBuffer);
    }

    virtual void Setup(RenderContext_t* context) override final
    {
#if 0
        IRenderBlock::Setup(context);

        // set shader constants
        m_Constants.m_Scale = m_Block.attributes.scale;

        // set the constant buffers
        Renderer::Get()->SetVertexShaderConstants(m_ConstantBuffer, 2, m_Constants);
        Renderer::Get()->SetPixelShaderConstants(m_ConstantBuffer, 2, m_Constants);

        Renderer::Get()->SetCullMode((!(m_Block.attributes.flags & 1)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);
#endif
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        // DrawSkinBatches(context);
    }

    virtual void DrawUI() override final
    {
        // ImGui::SliderFloat("Scale", &m_Block.attributes.scale, 0.1f, 10.0f);
    }
};