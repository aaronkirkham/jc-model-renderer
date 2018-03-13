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
    struct GeneralMkIIIConstants
    {
        float m_Scale = 1.0f;
        glm::vec2 m_UVExtent = { 0, 0 };
        char pad[4];
    } m_Constants;

    JustCause3::RenderBlocks::GeneralMkIII m_Block;
    VertexBuffer_t* m_VertexBufferData = nullptr;
    ConstantBuffer_t* m_ConstantBuffer = nullptr;

public:
    RenderBlockGeneralMkIII() = default;
    virtual ~RenderBlockGeneralMkIII()
    {
        Renderer::Get()->DestroyBuffer(m_VertexBufferData);
        Renderer::Get()->DestroyBuffer(m_ConstantBuffer);
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

        // create the constant buffer
        m_ConstantBuffer = Renderer::Get()->CreateConstantBuffer(m_Constants);

        // create the sampler states
        {
            SamplerStateCreationParams_t params;
            m_SamplerState = Renderer::Get()->CreateSamplerState(params);
        }
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace JustCause3::Vertex;

        // read the block header
        stream.read((char *)&m_Block, sizeof(m_Block));

        // set vertex shader constants
        m_Constants.m_Scale = m_Block.attributes.packed.scale;
        m_Constants.m_UVExtent = m_Block.attributes.packed.uv0Extent;

        // read some constant buffer data
        char unknown[280];
        stream.read((char *)&unknown, 280);

        // read the materials
        ReadMaterials(stream);

        // read the vertex buffers
        if (m_Block.attributes.flags & 0x20) {
            std::vector<VertexUnknown> vertices;
            ReadVertexBuffer<VertexUnknown>(stream, &m_VertexBuffer, &vertices);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(vertex.x);
                m_Vertices.emplace_back(vertex.y);
                m_Vertices.emplace_back(vertex.z);
            }
        }
        else {
            std::vector<PackedVertexPosition> vertices;
            ReadVertexBuffer<PackedVertexPosition>(stream, &m_VertexBuffer, &vertices);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(unpack(vertex.x));
                m_Vertices.emplace_back(unpack(vertex.y));
                m_Vertices.emplace_back(unpack(vertex.z));
            }
        }

        // read the vertex buffer data
        std::vector<GeneralShortPacked> vertices_data;
        ReadVertexBuffer<GeneralShortPacked>(stream, &m_VertexBufferData, &vertices_data);

        for (const auto& data : vertices_data) {
            m_UVs.emplace_back(unpack(data.u0));
            m_UVs.emplace_back(unpack(data.v0));
        }

        // read skin batches
        if (m_Block.attributes.flags & 0x8020) {
            ReadSkinBatch(stream);
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