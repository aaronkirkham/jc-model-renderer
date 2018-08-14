#pragma once

#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct WindowAttributes
{
    char pad[0x24];
    uint32_t flags;
};

namespace JustCause3::RenderBlocks
{
    struct Window
    {
        uint8_t version;
        WindowAttributes attributes;
    };
};
#pragma pack(pop)

class RenderBlockWindow : public IRenderBlock
{
private:
    struct cbInstanceConsts
    {
        glm::mat4 World;
        glm::mat4 WorldViewProjection;
    } m_cbInstanceConsts;

    struct cbMaterialConsts
    {
        float SpecGloss;
        float SpecFresnel;
        float DiffuseRougness;
        float TintPower;
        float MinAlpha;
        float UVScale;
        glm::vec2 unused_;                  // [unused]
        float DamageMaskWeight;             // [unused]
        float DamageRampWeight;             // [unused]
        float Alpha;
    } m_cbMaterialConsts;

    JustCause3::RenderBlocks::Window m_Block;
    ConstantBuffer_t* m_VertexShaderConstants = nullptr;
    ConstantBuffer_t* m_FragmentShaderConstants = nullptr;

public:
    RenderBlockWindow() = default;
    virtual ~RenderBlockWindow()
    {
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants);
        Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants);
    }

    virtual const char* GetTypeName() override final { return "RenderBlockWindow"; }
    virtual bool IsOpaque() override final { return true; }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader("window");
        m_PixelShader = ShaderManager::Get()->GetPixelShader("window");

        // create the element input desc
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   0,  DXGI_FORMAT_R32G32B32A32_FLOAT,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };

        // create the vertex declaration
        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(), "RenderBlockWindow");

        // create the constant buffers
        m_VertexShaderConstants = Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "RenderBlockWindow InstanceConsts");
        m_FragmentShaderConstants = Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts, "RenderBlockWindow MaterialConsts");

        // create the sampler states
        {
            SamplerStateParams_t params;
            params.m_AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_MinMip = 0.0f;
            params.m_MaxMip = 13.0f;

            m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockWindow");
        }

        //
        memset(&m_cbMaterialConsts, 0, sizeof(m_cbMaterialConsts));
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace JustCause3::Vertex;

        // read block data
        stream.read((char *)&m_Block, sizeof(m_Block));

        // read materials
        ReadMaterials(stream);

        // read vertex buffer
        {
            std::vector<UnpackedVertexPosition2UV> vertices;
            ReadVertexBuffer<UnpackedVertexPosition2UV>(stream, &m_VertexBuffer, &vertices);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(vertex.x);
                m_Vertices.emplace_back(vertex.y);
                m_Vertices.emplace_back(vertex.z);
                m_UVs.emplace_back(vertex.u0);
                m_UVs.emplace_back(vertex.v0);
            }
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
            static const auto world = glm::mat4(1);

            // set vertex shader constants
            m_cbInstanceConsts.World = world;
            m_cbInstanceConsts.WorldViewProjection = world * context->m_viewProjectionMatrix;
        }

        // set the sampler state
        context->m_Renderer->SetSamplerState(m_SamplerState, 0);

        // enable blending
        context->m_Renderer->SetBlendingEnabled(true);
        context->m_Renderer->SetBlendingFunc(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_DEST_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_ONE);
        context->m_Renderer->SetBlendingEq(D3D11_BLEND_OP_ADD, D3D11_BLEND_OP_ADD);

        // enable depth
        context->m_Renderer->SetDepthEnabled(true);

        // set the constant buffers
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants, 1, m_cbInstanceConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants, 1, m_cbMaterialConsts);

        // set the culling mode
        if (m_Block.attributes.flags & 1) {
            context->m_Renderer->SetCullMode(D3D11_CULL_BACK);
        }
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible) return;

        if (!(m_Block.attributes.flags & 1)) {
            context->m_Renderer->SetCullMode(D3D11_CULL_FRONT);
            IRenderBlock::Draw(context);
            context->m_Renderer->SetCullMode(D3D11_CULL_BACK);
        }

        IRenderBlock::Draw(context);
    }

    virtual void DrawUI() override final
    {
        static std::array flag_labels = {
            "Disable Culling", "", "", "", "", "", "", "",
            "", "", "", "", "", "", "", "",
            "", "", "", "", "", "", "", "",
            "", "", "", "", "", "", "", ""
        };

        ImGuiCustom::BitFieldTooltip("Flags", &m_Block.attributes.flags, flag_labels);

        ImGui::SliderFloat("Specular Gloss", &m_cbMaterialConsts.SpecGloss, 0, 1);
        ImGui::SliderFloat("Specular Fresnel", &m_cbMaterialConsts.SpecFresnel, 0, 1);
        ImGui::SliderFloat("Diffuse Rougness", &m_cbMaterialConsts.DiffuseRougness, 0, 1);
        ImGui::SliderFloat("Tint Power", &m_cbMaterialConsts.TintPower, 0, 1);
        ImGui::SliderFloat("Min Alpha", &m_cbMaterialConsts.MinAlpha, 0, 1);
        ImGui::SliderFloat("UV Scale", &m_cbMaterialConsts.UVScale, 0, 1);
        ImGui::SliderFloat("Alpha", &m_cbMaterialConsts.Alpha, 0, 1);
    }
};