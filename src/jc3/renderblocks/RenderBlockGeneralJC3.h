#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>
#include <gtc/type_ptr.hpp>

#pragma pack(push, 1)
struct GeneralJC3Attributes
{
    float depthBias;
    float specularGloss;
    float reflectivity;
    float emmissive;
    float diffuseWrap;
    float specularFresnel;
    glm::vec4 diffuseModulator;
    float _unknown;
    float _unknown2;
    float _unknown3;
    JustCause3::Vertex::SPackedAttribute packed;
    uint32_t flags;
};

namespace JustCause3::RenderBlocks
{
    struct GeneralJC3
    {
        uint8_t version;
        GeneralJC3Attributes attributes;
    };
};
#pragma pack(pop)

class RenderBlockGeneralJC3 : public IRenderBlock
{
private:
    struct cbVertexInstanceConsts
    {
        glm::mat4 viewProjection;       // 0000 - 0040 (0 -> 4)
        glm::mat4 world;                // 0040 - 0080 (4 -> 8)
        char _pad[0x20];                // 0080 - 00A0 (8 -> 10)
        char _pad2[0x10];               // 00A0 - 00B0 (10 -> 11)
        glm::vec4 colour;               // 00B0 - 00C0 (11 -> 12)
        glm::vec4 _unknown[3];          // 00C0 - 00F0 (12 -> 15)
        glm::vec4 _thing;
    } m_cbVertexInstanceConsts;

    struct cbVertexMaterialConsts
    {
        glm::vec4 data;
        glm::vec2 uv0Extent;
        glm::vec2 uv1Extent;
    } m_cbVertexMaterialConsts;

    struct cbFragmentMaterialConsts
    {
        float specularGloss;
        float reflectivity;
        float _unknown;
        float specularFresnel;
        glm::vec4 diffuseModulator;
        float _unknown2;
        float _unknown3;
        float _unknown4;
        float _unknown5;
    } m_cbFragmentMaterialConsts;

    struct cbFragmentInstanceConsts
    {
        glm::vec4 colour;
    } m_cbFragmentInstanceConsts;

    JustCause3::RenderBlocks::GeneralJC3 m_Block;
    VertexBuffer_t* m_VertexBufferData = nullptr;
    std::array<ConstantBuffer_t*, 2> m_VertexShaderConstants = { nullptr };
    std::array<ConstantBuffer_t*, 2> m_FragmentShaderConstants = { nullptr };
    SamplerState_t* m_SamplerStateNormalMap = nullptr;
    VertexDeclaration_t* m_VertexDeclaration = nullptr;

public:
    RenderBlockGeneralJC3() = default;
    virtual ~RenderBlockGeneralJC3()
    {
        OutputDebugStringA("~RenderBlockGeneralJC3\n");

        Renderer::Get()->DestroyBuffer(m_VertexBufferData);
        Renderer::Get()->DestroyVertexDeclaration(m_VertexDeclaration);
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[0]);
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[1]);
        Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants[0]);
        Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants[1]);
        Renderer::Get()->DestroySamplerState(m_SamplerStateNormalMap);
    }

    virtual const char* GetTypeName() override final { return "RenderBlockGeneralJC3"; }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader("generaljc3");
        m_PixelShader = ShaderManager::Get()->GetPixelShader("generaljc3");

        // create the input desc
        if (m_Block.attributes.packed.format != 1) {
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  0,                              D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   0,  DXGI_FORMAT_R32G32B32A32_FLOAT,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };

            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(), "RenderBlockGeneralJC3 (Unpacked vertices)");
        }
        else {
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  0,                              D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     1,  0,                              D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };

            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(), "RenderBlockGeneralJC3 (Packed vertices)");
        }

        // create the constant buffer
        m_VertexShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbVertexInstanceConsts, "RenderBlockGeneralJC3 cbVertexInstanceConsts");
        m_VertexShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(m_cbVertexMaterialConsts, "RenderBlockGeneralJC3 m_cbVertexMaterialConsts");
        m_FragmentShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbFragmentMaterialConsts, "RenderBlockGeneralJC3 cbFragmentMaterialConsts");
        m_FragmentShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(m_cbFragmentInstanceConsts, "RenderBlockGeneralJC3 cbFragmentInstanceConsts");

        // create the sampler states
        {
            SamplerStateCreationParams_t params;
            params.m_AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_MinMip = 0.0f;
            params.m_MaxMip = 13.0f;

            m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockGeneralJC3");
            // m_SamplerStateNormalMap = Renderer::Get()->CreateSamplerState(params, "RenderBlockGeneralJC3 - NormalMap");
        }

        // assert(m_SamplerState);
        // assert(m_SamplerStateNormalMap);
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace JustCause3::Vertex;

        // read the block header
        stream.read((char *)&m_Block, sizeof(m_Block));

        // read the materials
        ReadMaterials(stream);

        // read the vertex buffers
        if (m_Block.attributes.packed.format == 1) {
            std::vector<PackedVertexPosition> vertices;
            ReadVertexBuffer<PackedVertexPosition>(stream, &m_VertexBuffer, &vertices);

            std::vector<GeneralShortPacked> vertices_data;
            ReadVertexBuffer<GeneralShortPacked>(stream, &m_VertexBufferData, &vertices_data);

            for (auto i = 0; i < vertices.size(); ++i) {
                auto& vertex = vertices[i];
                auto& data = vertices_data[i];

                m_Vertices.emplace_back(unpack(vertex.x));
                m_Vertices.emplace_back(unpack(vertex.y));
                m_Vertices.emplace_back(unpack(vertex.z));
                m_UVs.emplace_back(unpack(data.u0));
                m_UVs.emplace_back(unpack(data.v0));
            }
        }
        else {
#ifdef DEBUG
            OutputDebugString("RenderBlockGeneralJC3 is using unpacked vertices!\n");
            __debugbreak();
#endif
        }

        // read index buffer
        ReadIndexBuffer(stream, &m_IndexBuffer);
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        IRenderBlock::Setup(context);

        // setup the constant buffer
        {
            const auto scale = m_Block.attributes.packed.scale;
            auto world = glm::scale(glm::mat4(1), { scale, scale, scale });

            // set vertex shader constants
            m_cbVertexInstanceConsts.viewProjection = context->m_viewProjectionMatrix;
            m_cbVertexInstanceConsts.world = world;
            m_cbVertexInstanceConsts.colour = glm::vec4(1, 1, 1, 1);
            m_cbVertexInstanceConsts._thing = glm::vec4(0, 1, 2, 1);
            m_cbVertexMaterialConsts.data = { m_Block.attributes.depthBias, m_Block.attributes._unknown3, 0, 0 };
            m_cbVertexMaterialConsts.uv0Extent = m_Block.attributes.packed.uv0Extent;
            m_cbVertexMaterialConsts.uv1Extent = m_Block.attributes.packed.uv1Extent;
            
            // set fragment shader constants
            m_cbFragmentMaterialConsts.specularGloss = m_Block.attributes.specularGloss;
            m_cbFragmentMaterialConsts.reflectivity = m_Block.attributes.reflectivity;
            m_cbFragmentMaterialConsts.specularFresnel = m_Block.attributes.specularFresnel;
            m_cbFragmentMaterialConsts.diffuseModulator = m_Block.attributes.diffuseModulator;
            m_cbFragmentInstanceConsts.colour = glm::vec4(1, 1, 1, 0);
        }

        // set the input layout
        context->m_DeviceContext->IASetInputLayout(m_VertexDeclaration->m_Layout);

        // set the constant buffers
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 1, m_cbVertexInstanceConsts);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 2, m_cbVertexMaterialConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[0], 1, m_cbFragmentMaterialConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[1], 2, m_cbFragmentInstanceConsts);

        // set the culling mode
        context->m_Renderer->SetCullMode((!(m_Block.attributes.flags & 1)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);

        // TODO: Upacked vertices condition

        // set the 2nd vertex buffers
        uint32_t offset = 0;
        context->m_DeviceContext->IASetVertexBuffers(1, 1, &m_VertexBufferData->m_Buffer, &m_VertexBufferData->m_ElementStride, &offset);

        // set the normal map sampler
        // context->m_DeviceContext->PSSetSamplers(1, 1, &m_SamplerStateNormalMap->m_SamplerState);

        context->m_DeviceContext->PSSetSamplers(1, 1, &m_SamplerState->m_SamplerState);
        context->m_DeviceContext->PSSetSamplers(2, 1, &m_SamplerState->m_SamplerState);
        context->m_DeviceContext->PSSetSamplers(3, 1, &m_SamplerState->m_SamplerState);
    }

    virtual void DrawUI() override final
    {
        ImGui::SliderFloat("Scale", &m_Block.attributes.packed.scale, 0.1f, 10.0f);
        ImGui::SliderFloat("Depth Bias", &m_Block.attributes.depthBias, 0.0f, 10.0f);
        ImGui::SliderFloat("Specular Gloss", &m_Block.attributes.specularGloss, 0.0f, 10.0f);
        ImGui::SliderFloat("Reflectivity", &m_Block.attributes.reflectivity, 0.0f, 10.0f);
        ImGui::SliderFloat("Emissive", &m_Block.attributes.emmissive, 0.0f, 10.0f);
        ImGui::SliderFloat("Diffuse Wrap", &m_Block.attributes.diffuseWrap, 0.0f, 10.0f);
        ImGui::SliderFloat("Specular Fresnel", &m_Block.attributes.specularFresnel, 0.0f, 10.0f);
        ImGui::SliderFloat4("Diffuse Modulator", glm::value_ptr(m_Block.attributes.diffuseModulator), 0.0f, 1.0f);
        ImGui::SliderFloat("Unknown #1", &m_Block.attributes._unknown, 0.0f, 10.0f);
        ImGui::SliderFloat("Unknown #2", &m_Block.attributes._unknown2, 0.0f, 10.0f);
        ImGui::SliderFloat("Unknown #3", &m_Block.attributes._unknown3, 0.0f, 10.0f);
    }
};