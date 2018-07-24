#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct GeneralMkIIIAttributes
{
    float depthBias;
    char _pad[0x8];
    float emissiveTODScale;
    float emissiveStartFadeDistSq;
    char _pad2[0x10];
    JustCause3::Vertex::SPackedAttribute packed;
    char _pad3[0x4];
    uint32_t flags;
    char _pad4[0x1C];
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
    struct RBIInfo
    {
        glm::mat4 ModelWorldMatrix;
        glm::mat4 ModelWorldMatrixPrev;     // [unused]
        glm::vec4 ModelDiffuseColor;
        glm::vec4 ModelAmbientColor;        // [unused]
        glm::vec4 ModelSpecularColor;       // [unused]
        glm::vec4 ModelEmissiveColor;
        glm::vec4 ModelDebugColor;          // [unused]
    } m_cbRBIInfo;

    struct InstanceAttributes
    {
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

    struct cbSkinningConsts
    {
        glm::vec4 MatrixPalette[2];
    } m_cbSkinningConsts;

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
        if (m_Block.attributes.flags & 0x20) {
            __debugbreak();
        }

        // create the element input desc
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   2,  DXGI_FORMAT_R16G16B16A16_SNORM,     1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   3,  DXGI_FORMAT_R32G32B32_FLOAT,        1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };

        // create the vertex declaration
        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(), "RenderBlockGeneralMkIII");

        // create the constant buffers
        m_VertexShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbRBIInfo, "RenderBlockGeneralMkIII RBIInfo");
        m_VertexShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(m_cbInstanceAttributes, "RenderBlockGeneralMkIII InstanceAttributes");

        // identity the palette data
        for (int i = 0; i < 2; ++i) {
            m_cbSkinningConsts.MatrixPalette[i] = glm::vec4(1);
        }

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

            OutputDebugStringA("RenderBlockGeneralMkIII has skin batches, need to do matrix palette data stuff...\n");
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

            // set vertex shader constants
            m_cbRBIInfo.ModelWorldMatrix = world;
            m_cbInstanceAttributes.UVScale = { m_Block.attributes.packed.uv0Extent, m_Block.attributes.packed.uv1Extent };
            m_cbInstanceAttributes.DepthBias = m_Block.attributes.depthBias;
            m_cbInstanceAttributes.QuantizationScale = scale;
            m_cbInstanceAttributes.EmissiveTODScale = m_Block.attributes.emissiveTODScale;
            m_cbInstanceAttributes.EmissiveStartFadeDistSq = m_Block.attributes.emissiveStartFadeDistSq;

            // set fragment shader constants
            //
        }

        // set the constant buffers
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 12, m_cbRBIInfo);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 2, m_cbInstanceAttributes);
        // context->m_Renderer->SetPixelShaderConstants(m_ConstantBuffer, 2, m_Constants);

        context->m_Renderer->SetCullMode((!(m_Block.attributes.flags & 1)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);

        // set the 2nd vertex buffers
        context->m_Renderer->SetVertexStream(m_VertexBufferData, 1);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible) return;

        // needs matrix palette stuff..
        const auto flags = m_Block.attributes.flags;
        if (_bittest((const long *)&flags, 0xF)) {
            for (const auto& batch : m_SkinBatches) {
                // TODO: upload matrix palette data

                context->m_Renderer->DrawIndexed(batch.m_Offset, batch.m_Size, m_IndexBuffer);
            }
        }
        else if (flags & 0x20) {
            __debugbreak();

            // TODO: upload matrix palette data
        }
        else {
            IRenderBlock::Draw(context);
        }
    }

    virtual void DrawUI() override final
    {
        // flags & 2 = something with transparency?

        static std::array flag_labels = {
            "Disable Culling", "", "", "", "", "Has Packed Vertices", "", "",
            "", "", "", "", "", "", "", "Has Skin Batches",

            "", "", "", "", "", "", "", "",
            "", "", "", "", "", "", "", ""
        };

        ImGuiCustom::BitFieldTooltip("", &m_Block.attributes.flags, flag_labels);

        ImGui::SliderFloat("Scale", &m_Block.attributes.packed.scale, 0.1f, 10.0f);
    }
};