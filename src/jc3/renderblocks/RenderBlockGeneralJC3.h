#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct GeneralJC3Attributes
{
    char pad[0x34];
    JustCause3::Vertex::SPackedAttribute packed;
    uint32_t flags;
};

namespace JustCause3
{
    namespace RenderBlocks
    {
        struct GeneralJC3
        {
            uint8_t version;
            GeneralJC3Attributes attributes;
        };
    };
};
#pragma pack(pop)

class RenderBlockGeneralJC3 : public IRenderBlock
{
private:
    JustCause3::RenderBlocks::GeneralJC3 m_Block;
    VertexBuffer_t* m_VertexBufferInt16 = nullptr;
    VertexDeclaration_t* m_VertexDeclaration = nullptr;
    SamplerState_t* m_SamplerStateNormalMap = nullptr;

public:
    RenderBlockGeneralJC3() = default;
    virtual ~RenderBlockGeneralJC3()
    {
        OutputDebugStringA("~RenderBlockGeneralJC3\n");

        Renderer::Get()->DestroySamplerState(m_SamplerStateNormalMap);
        Renderer::Get()->DestroyVertexDeclaration(m_VertexDeclaration);
        Renderer::Get()->DestroyBuffer(m_VertexBufferInt16);
    }

    virtual const char* GetTypeName() override final { return "RenderBlockGeneralJC3"; }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader("GeneralJC3");
        m_PixelShader = ShaderManager::Get()->GetPixelShader("GeneralJC3");

        // create the element input desc
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R16G16B16A16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R16G16B16A16_SINT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        // create the vertex declaration
        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 5, m_VertexShader.get());

        // create the sampler states
        {
            SamplerStateCreationParams_t params;
            params.m_AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_MinMip = 0.0f;
            params.m_MaxMip = 13.0f;

            m_SamplerState = Renderer::Get()->CreateSamplerState(params);
            m_SamplerStateNormalMap = Renderer::Get()->CreateSamplerState(params);
        }

        assert(m_VertexShader);
        assert(m_PixelShader);
        assert(m_VertexDeclaration);
        assert(m_SamplerState);
        assert(m_SamplerStateNormalMap);
    }

    virtual void Read(fs::path& filename, std::istream& stream) override final
    {
        // read the block header
        stream.read((char *)&m_Block, sizeof(m_Block));

        // set vertex shader constants
        m_Constants.scale = m_Block.attributes.packed.scale;
        m_Constants.uvExtent = m_Block.attributes.packed.uv0Extent;

        // read the materials
        ReadMaterials(filename, stream);

        // do we have a normal map loaded?
        // TODO: REMOVE THIS ONCE WE CAN LOAD GENERIC SHARED RESOURCES FROM OTHER ARCHIVES
        for (auto& texture : m_Textures) {
            auto filename = texture->GetPath().filename().string();
            if (filename.find("_nrm") != std::string::npos) {
                m_Constants.hasNormalMap = TRUE;
            }
        }

        // read the vertex buffers
        if (m_Block.attributes.packed.format == 1) {
            ReadVertexBuffer<JustCause3::Vertex::Packed>(stream, &m_Vertices);
            ReadVertexBuffer<JustCause3::Vertex::GeneralShortPacked>(stream, &m_VertexBufferInt16);
        }
        else {
            // TODO
        }

        // read index buffer
        ReadIndexBuffer(stream, &m_Indices);
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        // enable the vertex and pixel shaders
        context->m_DeviceContext->IASetInputLayout(m_VertexDeclaration->m_Layout);
        context->m_DeviceContext->VSSetShader(m_VertexShader->m_Shader, nullptr, 0);
        context->m_DeviceContext->PSSetShader(m_PixelShader->m_Shader, nullptr, 0);

        Renderer::Get()->SetCullMode((!(m_Block.attributes.flags & 1)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);

        // enable textures
        for (uint32_t i = 0; i < m_Textures.size(); ++i) {
            auto texture = m_Textures[i];
            if (texture && texture->IsLoaded()) {
                texture->Use(i);
            }
        }

        // set the 1st vertex buffers
        uint32_t offset = 0;
        context->m_DeviceContext->IASetVertexBuffers(0, 1, &m_Vertices->m_Buffer, &m_Vertices->m_ElementStride, &offset);

        // set the 2nd vertex buffers
        context->m_DeviceContext->IASetVertexBuffers(1, 1, &m_VertexBufferInt16->m_Buffer, &m_VertexBufferInt16->m_ElementStride, &offset);

        // set the samplers
        context->m_DeviceContext->PSSetSamplers(0, 1, &m_SamplerState->m_SamplerState);
        context->m_DeviceContext->PSSetSamplers(1, 1, &m_SamplerStateNormalMap->m_SamplerState);
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