#pragma once

#include "irenderblock.h"

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
static constexpr uint8_t CARLIGHT_VERSION = 1;

struct CarLight {
    uint8_t            version;
    CarLightAttributes attributes;
};
}; // namespace JustCause3::RenderBlocks
#pragma pack(pop)

class RenderBlockCarLight : public IRenderBlock
{
  private:
    enum {
        DISABLE_BACKFACE_CULLING = 0x1,
        IS_DEFORM                = 0x4,
        IS_SKINNED               = 0x8,
    };

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

    JustCause3::RenderBlocks::CarLight  m_Block;
    JustCause3::CDeformTable            m_DeformTable;
    std::vector<JustCause3::CSkinBatch> m_SkinBatches;
    VertexBuffer_t*                     m_VertexBufferData        = nullptr;
    std::array<ConstantBuffer_t*, 3>    m_VertexShaderConstants   = {nullptr};
    ConstantBuffer_t*                   m_FragmentShaderConstants = nullptr;

  public:
    RenderBlockCarLight() = default;
    virtual ~RenderBlockCarLight()
    {
        // delete the skin batch lookup
        for (auto& batch : m_SkinBatches) {
            SAFE_DELETE(batch.m_BatchLookup);
        }

        // destroy shader constants
        Renderer::Get()->DestroyBuffer(m_VertexBufferData);
        Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants);

        for (auto& vsc : m_VertexShaderConstants) {
            Renderer::Get()->DestroyBuffer(vsc);
        }
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
            D3D11_SAMPLER_DESC params{};
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

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));

        if (m_Block.version != JustCause3::RenderBlocks::CARLIGHT_VERSION) {
            __debugbreak();
        }

        // read the deform table
        ReadDeformTable(stream, &m_DeformTable);

        // read materials
        ReadMaterials(stream);

        // read vertex buffers
        if (m_Block.attributes.flags & IS_SKINNED) {
            m_VertexBuffer     = ReadVertexBuffer<UnpackedVertexWithNormal1>(stream);
            m_VertexBufferData = ReadVertexBuffer<UnpackedNormals>(stream);
        } else if (m_Block.attributes.flags & IS_DEFORM) {
            m_VertexBuffer     = ReadVertexBuffer<VertexDeformPos>(stream);
            m_VertexBufferData = ReadVertexBuffer<VertexDeformNormal2>(stream);
        } else {
            m_VertexBuffer     = ReadVertexBuffer<UnpackedVertexPosition>(stream);
            m_VertexBufferData = ReadVertexBuffer<UnpackedNormals>(stream);
        }

        // read skin batches
        if (m_Block.attributes.flags & IS_SKINNED) {
            ReadSkinBatch(stream, &m_SkinBatches);
        }

        // read index buffer
        m_IndexBuffer = ReadIndexBuffer(stream);
    }

    virtual void Write(std::ostream& stream) override final
    {
        // write the block attributes
        stream.write((char*)&m_Block, sizeof(m_Block));

        // write the deform table
        WriteDeformTable(stream, &m_DeformTable);

        // write the matierls
        WriteMaterials(stream);

        // write the vertex buffer
        WriteBuffer(stream, m_VertexBuffer);
        WriteBuffer(stream, m_VertexBufferData);

        // write skin batches
        if (m_Block.attributes.flags & IS_SKINNED) {
            WriteSkinBatch(stream, &m_SkinBatches);
        }

        // write the index buffer
        WriteBuffer(stream, m_IndexBuffer);
    }

    virtual void SetData(vertices_t* vertices, uint16s_t* indices) override final
    {
        //
    }

    virtual std::tuple<vertices_t, uint16s_t> GetData() override final
    {
        using namespace JustCause3::Vertex;

        vertices_t vertices;
        uint16s_t  indices = m_IndexBuffer->CastData<uint16_t>();

        if (m_Block.attributes.flags & IS_SKINNED) {
            const auto& vb     = m_VertexBuffer->CastData<UnpackedVertexWithNormal1>();
            const auto& vbdata = m_VertexBufferData->CastData<UnpackedNormals>();

            assert(vb.size() == vbdata.size());
            vertices.reserve(vb.size());

            for (auto i = 0; i < vb.size(); ++i) {
                vertex_t v;
                v.pos    = {vb[i].x, vb[i].y, vb[i].z};
                v.uv     = {vbdata[i].u0, vbdata[i].v0};
                v.normal = unpack_normal(vbdata[i].n);
                vertices.emplace_back(std::move(v));
            }
        } else if (m_Block.attributes.flags & IS_DEFORM) {
            const auto& vb     = m_VertexBuffer->CastData<VertexDeformPos>();
            const auto& vbdata = m_VertexBufferData->CastData<VertexDeformNormal2>();

            assert(vb.size() == vbdata.size());
            vertices.reserve(vb.size());

            for (auto i = 0; i < vb.size(); ++i) {
                vertex_t v;
                v.pos    = {vb[i].x, vb[i].y, vb[i].z};
                v.uv     = {vbdata[i].u0, vbdata[i].v0};
                v.normal = unpack_normal(vbdata[i].n);
                vertices.emplace_back(std::move(v));
            }
        } else {
            const auto& vb     = m_VertexBuffer->CastData<UnpackedVertexPosition>();
            const auto& vbdata = m_VertexBufferData->CastData<UnpackedNormals>();

            assert(vb.size() == vbdata.size());
            vertices.reserve(vb.size());

            for (auto i = 0; i < vb.size(); ++i) {
                vertex_t v;
                v.pos    = {vb[i].x, vb[i].y, vb[i].z};
                v.uv     = {vbdata[i].u0, vbdata[i].v0};
                v.normal = unpack_normal(vbdata[i].n);
                vertices.emplace_back(std::move(v));
            }
        }

        return {vertices, indices};
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

        // set the textures
        for (int i = 0; i < m_Textures.size(); ++i) {
            IRenderBlock::BindTexture(i, m_SamplerState);
        }

        // set the constant buffers
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 6, m_cbRBIInfo);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 1, m_cbInstanceConsts);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[2], 2, m_cbDeformConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants, 2, m_cbMaterialConsts);

        // set the culling mode
        context->m_Renderer->SetCullMode((!(m_Block.attributes.flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK
                                                                                                  : D3D11_CULL_NONE);

        // set the 2nd and 3rd vertex buffers
        context->m_Renderer->SetVertexStream(m_VertexBufferData, 1);
        context->m_Renderer->SetVertexStream(m_VertexBufferData, 2);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;

        if (m_Block.attributes.flags & IS_SKINNED) {
            IRenderBlock::DrawSkinBatches(context, m_SkinBatches);
        } else {
            IRenderBlock::Draw(context);
        }
    }

    virtual void DrawContextMenu() override final
    {
        // clang-format off
        static std::array flag_labels = {
            "Disable Backface Culling",     "",                             "Is Deformable",                "Is Skinned",
            "",                             "",                             "",                             ""
        };
        // clang-format on

        ImGuiCustom::DropDownFlags(m_Block.attributes.flags, flag_labels);
    }

    virtual void DrawUI() override final
    {
        ImGui::Text(ICON_FA_COGS "  Attributes");

        ImGui::SliderFloat("Specular Gloss", &m_Block.attributes.specularGloss, 0, 1);
        ImGui::SliderFloat("Reflectivity", &m_Block.attributes.reflectivity, 0, 1);
        ImGui::SliderFloat("Specular Fresnel", &m_Block.attributes.specularFresnel, 0, 1);
        ImGui::ColorEdit3("Diffuse Modulator", glm::value_ptr(m_Block.attributes.diffuseModulator));
        ImGui::SliderFloat2("Tiling", glm::value_ptr(m_Block.attributes.tilingUV), 0, 10);

        // Textures
        ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
        ImGui::Columns(3, nullptr, false);
        {
            IRenderBlock::DrawTexture("DiffuseMap", 0);
            IRenderBlock::DrawTexture("NormalMap", 1);
            IRenderBlock::DrawTexture("PropertyMap", 2);
            // 3 unknown
            IRenderBlock::DrawTexture("NormalDetailMap", 4);
            IRenderBlock::DrawTexture("EmissiveMap", 5);
        }
        ImGui::EndColumns();
    }
};
