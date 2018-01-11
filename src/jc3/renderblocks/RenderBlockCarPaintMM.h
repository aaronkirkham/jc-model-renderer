#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct CarPaintMMAttributes
{
    uint32_t flags;
    float unknown;
};

namespace JustCause3::RenderBlocks
{
    struct CarPaintMM
    {
        uint8_t version;
        CarPaintMMAttributes attributes;
    };
};
#pragma pack(pop)

class RenderBlockCarPaintMM : public IRenderBlock
{
private:
    struct CarPaintMMConstants
    {
        BOOL m_HasLayeredUVs = FALSE;
        char pad[12];
    } m_Constants;

    JustCause3::RenderBlocks::CarPaintMM m_Block;
    VertexBuffer_t* m_VertexBufferData = nullptr;
    VertexBuffer_t* m_VertexBufferUVData = nullptr;
    ConstantBuffer_t* m_ConstantBuffer = nullptr;

public:
    RenderBlockCarPaintMM() = default;
    virtual ~RenderBlockCarPaintMM()
    {
        Renderer::Get()->DestroyBuffer(m_VertexBufferData);
        Renderer::Get()->DestroyBuffer(m_VertexBufferUVData);
        Renderer::Get()->DestroyBuffer(m_ConstantBuffer);
    }

    virtual const char* GetTypeName() override final { return "RenderBlockCarPaintMM"; }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = GET_VERTEX_SHADER(car_paint_mm);
        m_PixelShader = GET_PIXEL_SHADER(car_paint_mm);

        // create the element input desc
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 2, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        // create the vertex declaration
        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 6, m_VertexShader.get());

        // create the constant buffer
        m_ConstantBuffer = Renderer::Get()->CreateConstantBuffer(m_Constants);

        // create the sampler states
        {
            SamplerStateCreationParams_t params;
            m_SamplerState = Renderer::Get()->CreateSamplerState(params);
        }
    }

    virtual void Read(fs::path& filename, std::istream& stream) override final
    {
        // read the block header
        stream.read((char *)&m_Block, sizeof(m_Block));

        // read material params
        char unk[316];
        stream.read((char *)&unk, 316);

        // read some constant buffer data
        char unknown[76];
        stream.read((char *)&unknown, 76);

        // read the deform table
        {
            auto ReadDeformTable = [&] {
                uint32_t unknown;
                for (auto i = 0; i < 256; ++i) {
                    stream.read((char*)&unknown, sizeof(unknown));
                }
            };

            ReadDeformTable();
        }

        // read the materials
        ReadMaterials(filename, stream);

        // read the vertex buffers
        if (_bittest((const long *)&m_Block.attributes.flags, 13)) {
            ReadVertexBuffer<JustCause3::Vertex::UnpackedVertexWithNormal1>(stream, &m_Vertices);
            ReadVertexBuffer<JustCause3::Vertex::UnpackedNormals>(stream, &m_VertexBufferData);
            ReadSkinBatch(stream);
            //__debugbreak();
        }
        else if (_bittest((const long *)&m_Block.attributes.flags, 12)) {
            ReadVertexBuffer<JustCause3::Vertex::VertexDeformPos>(stream, &m_Vertices);
            ReadVertexBuffer<JustCause3::Vertex::VertexDeformNormal2>(stream, &m_VertexBufferData);
            //__debugbreak();
        }
        else {
            ReadVertexBuffer<JustCause3::Vertex::UnpackedVertexPosition>(stream, &m_Vertices);
            ReadVertexBuffer<JustCause3::Vertex::UnpackedNormals>(stream, &m_VertexBufferData);
            //__debugbreak();
        }

        // read the layered uv data if needed
        if (m_Block.attributes.flags & 0x60) {
            ReadVertexBuffer<JustCause3::Vertex::UnpackedUV>(stream, &m_VertexBufferUVData);
            //__debugbreak();
        }

        // read index buffer
        ReadIndexBuffer(stream, &m_Indices);
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        IRenderBlock::Setup(context);

        // do we have layered uvs?
        if (m_Block.attributes.flags & 0x20) {
            m_Constants.m_HasLayeredUVs = TRUE;
        }

        // set the constant buffers
        Renderer::Get()->SetVertexShaderConstants(m_ConstantBuffer, 2, m_Constants);
        Renderer::Get()->SetPixelShaderConstants(m_ConstantBuffer, 2, m_Constants);

        // (~(this->m_Attributes.m_Flags >> 6) & 2 | 1)
        //Renderer::Get()->SetCullMode((!(m_Block.attributes.flags & 1)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);

        // set the 2nd vertex buffers
        uint32_t offset = 0;
        context->m_DeviceContext->IASetVertexBuffers(1, 1, &m_VertexBufferData->m_Buffer, &m_VertexBufferData->m_ElementStride, &offset);

        // set the 3rd vertex buffers
        if (m_VertexBufferUVData) {
            context->m_DeviceContext->IASetVertexBuffers(2, 1, &m_VertexBufferUVData->m_Buffer, &m_VertexBufferUVData->m_ElementStride, &offset);
        }
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (m_Block.attributes.flags & 0x2000) {
            IRenderBlock::DrawSkinBatches(context);
        }
        else {
            IRenderBlock::Draw(context);
        }
    }
};