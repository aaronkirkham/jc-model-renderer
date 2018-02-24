#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct BuildingJC3Attributes
{
    char pad[0x68];
    JustCause3::Vertex::SPackedAttribute packed;
    char pad2[0x1C];
};

namespace JustCause3::RenderBlocks
{
    struct BuildingJC3
    {
        uint8_t version;
        GeneralJC3Attributes attributes;
    };
};
#pragma pack(pop)

class RenderBlockBuildingJC3 : public IRenderBlock
{
private:
    struct BuildingJC3Constants
    {
        float m_Scale = 1.0f;
        glm::vec2 m_UVExtent = { 0, 0 };
        char pad[4];
    } m_Constants;

    JustCause3::RenderBlocks::BuildingJC3 m_Block;
    VertexBuffer_t* m_VertexBufferData = nullptr;
    ConstantBuffer_t* m_ConstantBuffer = nullptr;

public:
    RenderBlockBuildingJC3() = default;
    virtual ~RenderBlockBuildingJC3()
    {
        Renderer::Get()->DestroyBuffer(m_VertexBufferData);
        Renderer::Get()->DestroyBuffer(m_ConstantBuffer);
    }

    virtual const char* GetTypeName() override final { return "RenderBlockBuildingJC3"; }

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

        // create the constant buffer
        m_ConstantBuffer = Renderer::Get()->CreateConstantBuffer(m_Constants);
    }

    virtual void Read(fs::path& filename, std::istream& stream) override final
    {
        // read the block header
        stream.read((char *)&m_Block, sizeof(m_Block));

        // set vertex shader constants
        m_Constants.m_Scale = m_Block.attributes.packed.scale;
        m_Constants.m_UVExtent = m_Block.attributes.packed.uv0Extent;

        // read the materials
        ReadMaterials(filename, stream);

        // read the vertex buffers
        if (m_Block.attributes.packed.format == 1) {
            ReadVertexBuffer<JustCause3::Vertex::PackedVertexPosition>(stream, &m_VertexBuffer);
            ReadVertexBuffer<JustCause3::Vertex::GeneralShortPacked>(stream, &m_VertexBufferData);
        }
        else {
            //ReadVertexBuffer<JustCause3::Vertex::Unpacked>(stream, &m_Vertices);
        }

        // read index buffer
        ReadIndexBuffer(stream, &m_IndexBuffer);
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        IRenderBlock::Setup(context);

        // set the constant buffers
        Renderer::Get()->SetVertexShaderConstants(m_ConstantBuffer, 2, m_Constants);
        Renderer::Get()->SetPixelShaderConstants(m_ConstantBuffer, 2, m_Constants);

        Renderer::Get()->SetCullMode((!(m_Block.attributes.flags & 1)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);

        // set the 2nd vertex buffers
        uint32_t offset = 0;
        context->m_DeviceContext->IASetVertexBuffers(1, 1, &m_VertexBufferData->m_Buffer, &m_VertexBufferData->m_ElementStride, &offset);
    }

    virtual void DrawUI() override final
    {
        ImGui::Text("hello");
    }
};