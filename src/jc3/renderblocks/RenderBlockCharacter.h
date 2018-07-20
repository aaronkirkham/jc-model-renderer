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
    struct cbLocalConstants
    {
        glm::mat4 world;
        glm::mat4 worldViewProjection;
        glm::vec4 scale;
        // glm::mat3x4 data[70];
    } m_cbLocalConstants;

    JustCause3::RenderBlocks::Character m_Block;
    ConstantBuffer_t* m_ConstantBuffer = nullptr;
    int64_t m_Stride = 0;

    int64_t GetStride() const
    {
        static int32_t strides[] = { 0x18, 0x1C, 0x20, 0x20, 0x24, 0x28 };
        return strides[3 * ((m_Block.attributes.flags >> 1) & 1) + ((m_Block.attributes.flags >> 5) & 1) + ((m_Block.attributes.flags >> 4) & 1)];
    }

public:
    RenderBlockCharacter() = default;
    virtual ~RenderBlockCharacter()
    {
        Renderer::Get()->DestroyBuffer(m_ConstantBuffer);
    }

    virtual const char* GetTypeName() override final { return "RenderBlockCharacter"; }

    virtual void Create() override final
    {
        if (m_Stride == 0x18) {
            // load shaders
            m_VertexShader = ShaderManager::Get()->GetVertexShader("character");
            m_PixelShader = ShaderManager::Get()->GetPixelShader("_DEBUG");

            // create the element input desc
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  0,                              D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   4,  DXGI_FORMAT_R16G16_SNORM,           0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   6,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };

            // create the vertex declaration
            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 5, m_VertexShader.get(), "RenderBlockCharacter Stride 0x18");
        }
        else {
            __debugbreak();
        }

        // create the constant buffer
        m_ConstantBuffer = Renderer::Get()->CreateConstantBuffer(m_cbLocalConstants, "RenderBlockCharacter cbLocalConstants");

        // create the sampler states
        {
            SamplerStateCreationParams_t params;
            params.m_AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_MinMip = 12.0f;
            params.m_MaxMip = 13.0f;

            m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockCharacter");
        }
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace JustCause3::Vertex;
        using namespace JustCause3::Vertex::RenderBlockCharacter;

        // read the block header
        stream.read((char *)&m_Block, sizeof(m_Block));

        // read the materials
        ReadMaterials(stream);

        // get the vertices stride
        m_Stride = GetStride();

        // read vertex data
        if (m_Stride == 0x18) {
            std::vector<PackedCharacterPos4Bones1UVs> vertices;
            ReadVertexBuffer<PackedCharacterPos4Bones1UVs>(stream, &m_VertexBuffer, &vertices);
        }
        else if (m_Stride == 0x1C) {
            __debugbreak();
        }
        else if (m_Stride == 0x20) {
            __debugbreak();
        }
        else if (m_Stride == 0x24) {
            __debugbreak();
        }
        else if (m_Stride == 0x28) {
            __debugbreak();
        }

        // read skin batch
        ReadSkinBatch(stream);

        // read index buffer
        ReadIndexBuffer(stream, &m_IndexBuffer);
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        IRenderBlock::Setup(context);

        // setup the constant buffer
        {
            const auto scale = m_Block.attributes.scale;
            auto world = glm::mat4(1);

            // set vertex shader constants
            m_cbLocalConstants.world = world;
            m_cbLocalConstants.worldViewProjection = context->m_ProjectionMatrix * context->m_ViewMatrix * world;
            m_cbLocalConstants.scale = glm::vec4(scale, 0, 0, 0);
        }

        // TODO: conditions for different vertex layouts

        // set the input layout
        context->m_DeviceContext->IASetInputLayout(m_VertexDeclaration->m_Layout);

        // set the constant buffers
        Renderer::Get()->SetVertexShaderConstants(m_ConstantBuffer, 1, m_cbLocalConstants);
        // Renderer::Get()->SetPixelShaderConstants(m_ConstantBuffer, 2, m_Constants);

        Renderer::Get()->SetCullMode((!(m_Block.attributes.flags & 1)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        DrawSkinBatches(context);
    }

    virtual void DrawUI() override final
    {
        ImGui::SliderFloat("Scale", &m_Block.attributes.scale, 0.1f, 10.0f);
    }
};