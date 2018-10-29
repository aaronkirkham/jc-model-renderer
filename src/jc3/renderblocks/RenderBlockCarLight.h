#pragma once

#include <gtc/type_ptr.hpp>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct CarLightAttributes {
    float     _unused;
    float     specularGloss;
    float     reflectivity;
    char      _pad[16];
    float     specularFresnel;
    glm::vec4 diffuseModulator;
    glm::vec2 tilingUV;
    uint32_t  flags;
};

static_assert(sizeof(CarLightAttributes) == 0x3C, "CarLightAttributes alignment is wrong!");

namespace JustCause3::RenderBlocks
{
struct CarLight {
    uint8_t            version;
    CarLightAttributes attributes;
};
}; // namespace JustCause3::RenderBlocks
#pragma pack(pop)

class RenderBlockCarLight : public IRenderBlock
{
  private:
    struct RBIInfo {
        glm::mat4 ModelWorldMatrix;
        glm::mat4 ModelWorldMatrixPrev; // [unused]
        glm::vec4 ModelDiffuseColor = glm::vec4(1);
        glm::vec4 ModelAmbientColor;  // [unused]
        glm::vec4 ModelSpecularColor; // [unused]
        glm::vec4 ModelEmissiveColor; // [unused]
        glm::vec4 ModelDebugColor;    // [unused]
    } m_cbRBIInfo;

    struct InstanceConsts {
        float IsDeformed = 0.0f;
        float _pad[3]; // [unused]
    } m_cbInstanceConsts;

    struct DeformConsts {
        glm::vec4 ControlPoints[216];
    } m_cbDeformConsts;

    struct MaterialConsts {
        float     SpecularGloss;
        float     Reflectivity;
        float     SpecularFresnel;
        float     _unknown;
        glm::vec4 DiffuseModulator;
        glm::vec4 TilingUV;
        glm::vec4 Colour = glm::vec4(1);
    } m_cbMaterialConsts;

    JustCause3::RenderBlocks::CarLight m_Block;
    JustCause3::CDeformTable           m_DeformTable;
    VertexBuffer_t*                    m_VertexBufferData        = nullptr;
    std::array<ConstantBuffer_t*, 3>   m_VertexShaderConstants   = {nullptr};
    ConstantBuffer_t*                  m_FragmentShaderConstants = nullptr;

  public:
    RenderBlockCarLight() = default;
    virtual ~RenderBlockCarLight()
    {
        Renderer::Get()->DestroyBuffer(m_VertexBufferData);

        for (auto& vsc : m_VertexShaderConstants)
            Renderer::Get()->DestroyBuffer(vsc);

        Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants);
    }

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockCarLight";
    }

    virtual uint32_t GetTypeHash() const override final
    {
        return RenderBlockFactory::RB_CARLIGHT;
    }

    virtual bool IsOpaque() override final
    {
        return true;
    }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader("carpaintmm");
        m_PixelShader  = ShaderManager::Get()->GetPixelShader("carlight");

        // create the element input desc
        // clang-format off
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   1,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   2,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   3,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   4,  DXGI_FORMAT_R32G32_FLOAT,           2,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };
        // clang-format on

        // create the vertex declaration
        m_VertexDeclaration =
            Renderer::Get()->CreateVertexDeclaration(inputDesc, 5, m_VertexShader.get(), "RenderBlockCarLight");

        // create the constant buffers
        m_VertexShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbRBIInfo, "RenderBlockCarLight RBIInfo");
        m_VertexShaderConstants[1] =
            Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "RenderBlockCarLight InstanceConsts");
        m_VertexShaderConstants[2] =
            Renderer::Get()->CreateConstantBuffer(m_cbDeformConsts, "RenderBlockCarLight DeformConsts");
        m_FragmentShaderConstants =
            Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts, "RenderBlockCarLight MaterialConsts");

        // create the sampler states
        {
            D3D11_SAMPLER_DESC params;
            ZeroMemory(&params, sizeof(params));
            params.Filter         = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            params.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
            params.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
            params.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
            params.MipLODBias     = 0.0f;
            params.MaxAnisotropy  = 1;
            params.ComparisonFunc = D3D11_COMPARISON_NEVER;
            params.MinLOD         = 0.0f;
            params.MaxLOD         = 13.0f;

            m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockCarLight");
        }

        //
        memset(&m_cbDeformConsts, 0, sizeof(m_cbDeformConsts));
        memset(&m_cbMaterialConsts, 0, sizeof(m_cbMaterialConsts));
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace JustCause3::Vertex;

        // read block data
        stream.read((char*)&m_Block, sizeof(m_Block));

        // read the deform table
        ReadDeformTable(stream, &m_DeformTable);

        // read materials
        ReadMaterials(stream);

        // read vertex buffers
        if (m_Block.attributes.flags & 8) {
            std::vector<UnpackedVertexWithNormal1> vertices;
            ReadVertexBuffer<UnpackedVertexWithNormal1>(stream, &m_VertexBuffer, &vertices);

            std::vector<UnpackedNormals> vertices_data;
            ReadVertexBuffer<UnpackedNormals>(stream, &m_VertexBufferData, &vertices_data);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(vertex.x);
                m_Vertices.emplace_back(vertex.y);
                m_Vertices.emplace_back(vertex.z);
            }

            for (const auto& data : vertices_data) {
                m_UVs.emplace_back(data.u0);
                m_UVs.emplace_back(data.v0);
            }

            // read skin batches
            ReadSkinBatch(stream);
        } else if (m_Block.attributes.flags & 4) {
            std::vector<VertexDeformPos> vertices;
            ReadVertexBuffer<VertexDeformPos>(stream, &m_VertexBuffer, &vertices);

            std::vector<VertexDeformNormal2> vertices_data;
            ReadVertexBuffer<VertexDeformNormal2>(stream, &m_VertexBufferData, &vertices_data);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(vertex.x);
                m_Vertices.emplace_back(vertex.y);
                m_Vertices.emplace_back(vertex.z);
            }

            for (const auto& data : vertices_data) {
                m_UVs.emplace_back(data.u0);
                m_UVs.emplace_back(data.v0);
            }
        } else {
            std::vector<UnpackedVertexPosition> vertices;
            ReadVertexBuffer<UnpackedVertexPosition>(stream, &m_VertexBuffer, &vertices);

            std::vector<UnpackedNormals> vertices_data;
            ReadVertexBuffer<UnpackedNormals>(stream, &m_VertexBufferData, &vertices_data);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(vertex.x);
                m_Vertices.emplace_back(vertex.y);
                m_Vertices.emplace_back(vertex.z);
            }

            for (const auto& data : vertices_data) {
                m_UVs.emplace_back(data.u0);
                m_UVs.emplace_back(data.v0);
            }
        }

        // read index buffer
        ReadIndexBuffer(stream, &m_IndexBuffer);
    }

    virtual void Write(std::ostream& stream) override final
    {
        //
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;

        IRenderBlock::Setup(context);

        // setup the constant buffer
        {
            static const auto world = glm::mat4(1);

            // set vertex shader constants
            m_cbRBIInfo.ModelWorldMatrix = glm::scale(world, {1, 1, 1});

            // set fragment shaders constants
            m_cbMaterialConsts.SpecularGloss    = m_Block.attributes.specularGloss;
            m_cbMaterialConsts.Reflectivity     = m_Block.attributes.reflectivity;
            m_cbMaterialConsts.SpecularFresnel  = m_Block.attributes.specularFresnel;
            m_cbMaterialConsts.DiffuseModulator = m_Block.attributes.diffuseModulator;
            m_cbMaterialConsts.TilingUV         = {m_Block.attributes.tilingUV, 1, 0};
        }

        // set the sampler state
        for (int i = 0; i < 10; ++i) {
            context->m_Renderer->SetSamplerState(m_SamplerState, i);
        }

        // set the constant buffers
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 6, m_cbRBIInfo);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 1, m_cbInstanceConsts);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[2], 2, m_cbDeformConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants, 2, m_cbMaterialConsts);

        // set the culling mode
        context->m_Renderer->SetCullMode(static_cast<D3D11_CULL_MODE>(~(m_Block.attributes.flags >> 3) & 2 | 1));

        // set the 2nd and 3rd vertex buffers
        context->m_Renderer->SetVertexStream(m_VertexBufferData, 1);
        context->m_Renderer->SetVertexStream(m_VertexBufferData, 2);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;

        if (m_Block.attributes.flags & 8) {
            IRenderBlock::DrawSkinBatches(context);
        } else {
            IRenderBlock::Draw(context);
        }
    }

    virtual void DrawUI() override final
    {
        static std::array flag_labels = {"", "", "", "Is Skinned", "Disable Culling",
                                         "", "", "", "",           "",
                                         "", "", "", "",           "",
                                         "", "", "", "",           "",
                                         "", "", "", "",           "",
                                         "", "", "", "",           "",
                                         "", ""};

        ImGuiCustom::BitFieldTooltip("Flags", &m_Block.attributes.flags, flag_labels);

        ImGui::SliderFloat("Specular Gloss", &m_Block.attributes.specularGloss, 0, 1);
        ImGui::SliderFloat("Reflectivity", &m_Block.attributes.reflectivity, 0, 1);
        ImGui::SliderFloat("Specular Fresnel", &m_Block.attributes.specularFresnel, 0, 1);
        ImGui::ColorEdit3("Diffuse Modulator", glm::value_ptr(m_Block.attributes.diffuseModulator));
        ImGui::SliderFloat2("Tiling", glm::value_ptr(m_Block.attributes.tilingUV), 0, 10);
    }
};
