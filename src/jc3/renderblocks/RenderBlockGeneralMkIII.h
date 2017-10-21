#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct GeneralMkIIIAttributes
{
    char pad[0x24];
    JustCause3::Vertex::SPackedAttribute packed;
    char pad2[0x4];
    uint32_t flags;
    char pad3[0x1C];
};

namespace JustCause3::RenderBlocks
{
    struct GeneralMkIII
    {
        uint8_t version;
        GeneralMkIIIAttributes attributes;
    };
};
#pragma pack(pop)

class RenderBlockGeneralMkIII : public IRenderBlock
{
private:
    JustCause3::RenderBlocks::GeneralMkIII m_Block;
    VertexBuffer_t* m_VertexBufferData = nullptr;

public:
    RenderBlockGeneralMkIII() = default;
    virtual ~RenderBlockGeneralMkIII()
    {
        Renderer::Get()->DestroyBuffer(m_VertexBufferData);
    }

    virtual const char* GetTypeName() override final { return "RenderBlockGeneralMkIII"; }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = GET_VERTEX_SHADER(general_jc3);
        m_PixelShader = GET_PIXEL_SHADER(general_jc3);

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
    }

    virtual void Read(fs::path& filename, std::istream& stream) override final
    {
        // read the block header
        stream.read((char *)&m_Block, sizeof(m_Block));

        // set vertex shader constants
        m_Constants.scale = m_Block.attributes.packed.scale;
        m_Constants.uvExtent = m_Block.attributes.packed.uv0Extent;

        // read some constant buffer data
        char unknown[280];
        stream.read((char *)&unknown, 280);

        // read the materials
        ReadMaterials(filename, stream);

        // read the vertex buffers
        if (m_Block.attributes.flags & 0x20) {
            // read something that is 16 bytes
            __debugbreak();
        }
        else {
            ReadVertexBuffer<JustCause3::Vertex::PackedVertexPosition>(stream, &m_Vertices);
        }

        // read the vertex buffer data
        ReadVertexBuffer<JustCause3::Vertex::GeneralShortPacked>(stream, &m_VertexBufferData);

        // read skin batches
        if (m_Block.attributes.flags & 0x8020) {
            ReadSkinBatch(stream);
        }

        // read index buffer
        ReadIndexBuffer(stream, &m_Indices);
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        IRenderBlock::Setup(context);

        Renderer::Get()->SetCullMode((!(m_Block.attributes.flags & 1)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);

        // set the 2nd vertex buffers
        uint32_t offset = 0;
        context->m_DeviceContext->IASetVertexBuffers(1, 1, &m_VertexBufferData->m_Buffer, &m_VertexBufferData->m_ElementStride, &offset);
    }
};