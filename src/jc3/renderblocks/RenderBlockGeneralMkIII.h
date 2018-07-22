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
    struct RBIInfo {
        glm::mat4 ModelWorldMatrix;
        glm::mat4 ModelWorldMatrixPrev;     // [unused]
        glm::vec4 ModelDiffuseColor;
        glm::vec4 ModelAmbientColor;        // [unused]
        glm::vec4 ModelSpecularColor;       // [unused]
        glm::vec4 ModelEmissiveColor;
        glm::vec4 ModelDebugColor;          // [unused]
    } m_cbRBIInfo;

    struct InstanceAttributes {
        glm::vec4 UVScale;
        glm::vec4 RippleAngle;              // [unused]
        glm::vec4 FlashDirection;           // [unused]
        float DepthBias;
        float QuantizationScale;
        float EmissiveStartFadeDistSq;
        float EmissiveTODScale;
        float RippleSpeed;                  // [unused]
        float RippleMagnitude;              // [unused]
        float PixelScale;                   // [unused]
        float OutlineThickness;             // [unused]
    } m_cbInstanceAttributes;

    JustCause3::RenderBlocks::GeneralMkIII m_Block;
    VertexBuffer_t* m_VertexBufferData = nullptr;
    std::array<ConstantBuffer_t*, 2> m_VertexShaderConstants = { nullptr };

public:
    RenderBlockGeneralMkIII() = default;
    virtual ~RenderBlockGeneralMkIII()
    {
        Renderer::Get()->DestroyBuffer(m_VertexBufferData);
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[0]);
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[1]);
    }

    virtual const char* GetTypeName() override final { return "RenderBlockGeneralMkIII"; }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader("generalmkiii");
        m_PixelShader = ShaderManager::Get()->GetPixelShader("generalmkiii");

        // TODO: if (m_Block.attributes.flags & 0x20) use R32G32B32A32

        // create the element input desc
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_UINT,      0,  0,                              D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   2,  DXGI_FORMAT_R16G16B16A16_UINT,      1,  0,                              D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   3,  DXGI_FORMAT_R32G32B32_FLOAT,        1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };

        // create the vertex declaration
        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(), "RenderBlockGeneralMkIII");

        // create the constant buffers
        m_VertexShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbRBIInfo, "RenderBlockGeneralMkIII RBIInfo");
        m_VertexShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(m_cbInstanceAttributes, "RenderBlockGeneralMkIII InstanceAttributes");

        // create the sampler states
        {
            // SamplerStateCreationParams_t params;
            // m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockGeneralMkIII");
        }
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace JustCause3::Vertex;

        // read the block header
        stream.read((char *)&m_Block, sizeof(m_Block));

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
        if (!m_Visible) return;

        IRenderBlock::Setup(context);

        // setup the constant buffer
        {
            const auto scale = m_Block.attributes.packed.scale;
            auto world = glm::scale(glm::mat4(1), { scale, scale, scale });

            m_cbRBIInfo.ModelWorldMatrix = world * context->m_viewProjectionMatrix;
        }

        // set the input layout
        context->m_DeviceContext->IASetInputLayout(m_VertexDeclaration->m_Layout);

        // set the constant buffers
        Renderer::Get()->SetVertexShaderConstants(m_VertexShaderConstants[0], 12, m_cbRBIInfo);
        // Renderer::Get()->SetVertexShaderConstants(m_ConstantBuffer, 2, m_Constants);
        // Renderer::Get()->SetPixelShaderConstants(m_ConstantBuffer, 2, m_Constants);

        Renderer::Get()->SetCullMode((!(m_Block.attributes.flags & 1)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);

        // set the 2nd vertex buffers
        uint32_t offset = 0;
        context->m_DeviceContext->IASetVertexBuffers(1, 1, &m_VertexBufferData->m_Buffer, &m_VertexBufferData->m_ElementStride, &offset);
    }

#if 0
    virtual void Draw(RenderContext_t* context) override final
    {
    }
#endif

    virtual void DrawUI() override final
    {
    }
};