#pragma once

#include <StdInc.h>
#include <gtc/type_ptr.hpp>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct GeneralJC3Attributes {
    float                                depthBias;
    float                                specularGloss;
    float                                reflectivity;
    float                                emmissive;
    float                                diffuseWrap;
    float                                specularFresnel;
    glm::vec4                            diffuseModulator;
    float                                backLight;
    float                                _unknown2;
    float                                startFadeOutDistanceEmissiveSq;
    JustCause3::Vertex::SPackedAttribute packed;
    uint32_t                             flags;
};

static_assert(sizeof(GeneralJC3Attributes) == 0x58, "GeneralJC3Attributes alignment is wrong!");

namespace JustCause3::RenderBlocks
{
static constexpr uint8_t GENERALJC3_VERSION = 6;

struct GeneralJC3 {
    uint8_t              version;
    GeneralJC3Attributes attributes;
};
}; // namespace JustCause3::RenderBlocks
#pragma pack(pop)

class RenderBlockGeneralJC3 : public IRenderBlock
{
  private:
    enum {
        DISABLE_BACKFACE_CULLING   = 0x1,
        TRANSPARENCY_ALPHABLENDING = 0x2,
        DYNAMIC_EMISSIVE           = 0x4,
    };

    struct cbVertexInstanceConsts {
        glm::mat4 viewProjection;
        glm::mat4 world;
        char      _pad[0x20];
        char      _pad2[0x10];
        glm::vec4 colour;
        glm::vec4 _unknown[3];
        glm::vec4 _thing;
    } m_cbVertexInstanceConsts;

    struct cbVertexMaterialConsts {
        float     DepthBias;
        float     EmissiveStartFadeDistSq;
        float     _unknown;
        float     _unknown2;
        glm::vec2 uv0Extent;
        glm::vec2 uv1Extent;
    } m_cbVertexMaterialConsts;

    struct cbFragmentMaterialConsts {
        float     specularGloss;
        float     reflectivity;
        float     _unknown;
        float     specularFresnel;
        glm::vec4 diffuseModulator;
        float     _unknown2;
        float     _unknown3;
        float     _unknown4;
        float     _unknown5;
    } m_cbFragmentMaterialConsts;

    struct cbFragmentInstanceConsts {
        glm::vec4 colour;
    } m_cbFragmentInstanceConsts;

    JustCause3::RenderBlocks::GeneralJC3 m_Block;
    VertexBuffer_t*                      m_VertexBufferData        = nullptr;
    std::array<ConstantBuffer_t*, 2>     m_VertexShaderConstants   = {nullptr};
    std::array<ConstantBuffer_t*, 2>     m_FragmentShaderConstants = {nullptr};

  public:
    RenderBlockGeneralJC3() = default;
    virtual ~RenderBlockGeneralJC3()
    {
        Renderer::Get()->DestroyBuffer(m_VertexBufferData);

        for (auto& vsc : m_VertexShaderConstants)
            Renderer::Get()->DestroyBuffer(vsc);

        for (auto& fsc : m_FragmentShaderConstants)
            Renderer::Get()->DestroyBuffer(fsc);
    }

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockGeneralJC3";
    }

    virtual uint32_t GetTypeHash() const override final
    {
        return RenderBlockFactory::RB_GENERALJC3;
    }

    virtual bool IsOpaque() override final
    {
        return ~m_Block.attributes.flags & TRANSPARENCY_ALPHABLENDING;
    }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader("generaljc3");
        m_PixelShader  = ShaderManager::Get()->GetPixelShader("generaljc3");

        // create the input desc
        if (m_Block.attributes.packed.format != 1) {
            // clang-format off
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   0,  DXGI_FORMAT_R32G32B32A32_FLOAT,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };
            // clang-format on

            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(),
                                                                           "RenderBlockGeneralJC3 (unpacked)");
        } else {
            // clang-format off
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };
            // clang-format on

            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(),
                                                                           "RenderBlockGeneralJC3 (packed)");
        }

        // create the constant buffer
        m_VertexShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(
            m_cbVertexInstanceConsts, "RenderBlockGeneralJC3 cbVertexInstanceConsts");
        m_VertexShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(
            m_cbVertexMaterialConsts, "RenderBlockGeneralJC3 m_cbVertexMaterialConsts");
        m_FragmentShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(
            m_cbFragmentMaterialConsts, "RenderBlockGeneralJC3 cbFragmentMaterialConsts");
        m_FragmentShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(
            m_cbFragmentInstanceConsts, "RenderBlockGeneralJC3 cbFragmentInstanceConsts");
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace JustCause3::Vertex;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));

        if (m_Block.version != JustCause3::RenderBlocks::GENERALJC3_VERSION) {
            __debugbreak();
        }

        // read the materials
        ReadMaterials(stream);

        // read the vertex buffers
        if (m_Block.attributes.packed.format != 1) {
            m_VertexBuffer = ReadVertexBuffer<UnpackedVertexPosition2UV>(stream);
        } else {
            m_VertexBuffer     = ReadVertexBuffer<PackedVertexPosition>(stream);
            m_VertexBufferData = ReadVertexBuffer<GeneralShortPacked>(stream);
        }

        // read index buffer
        m_IndexBuffer = ReadIndexBuffer(stream);
    }

    virtual void Write(std::ostream& stream) override final
    {
        // write the block attributes
        stream.write((char*)&m_Block, sizeof(m_Block));

        // write the materials
        WriteMaterials(stream);

        // write vertex buffers
        if (m_Block.attributes.packed.format != 1) {
            WriteBuffer(stream, m_VertexBuffer);
        } else {
            WriteBuffer(stream, m_VertexBuffer);
            WriteBuffer(stream, m_VertexBufferData);
        }

        // write index buffer
        WriteBuffer(stream, m_IndexBuffer);
    }

    virtual void SetData(floats_t* vertices, uint16s_t* indices, floats_t* uvs) override final
    {
        //
    }

    virtual std::tuple<floats_t, uint16s_t, floats_t> GetData() override final
    {
        using namespace JustCause3::Vertex;

        floats_t  vertices;
        uint16s_t indices = m_IndexBuffer->CastData<uint16_t>();
        floats_t  uvs;

        if (m_Block.attributes.packed.format != 1) {
            const auto& vb = m_VertexBuffer->CastData<UnpackedVertexPosition2UV>();

            for (const auto& vertex : vb) {
                vertices.emplace_back(vertex.x);
                vertices.emplace_back(vertex.y);
                vertices.emplace_back(vertex.z);
                uvs.emplace_back(vertex.u0);
                uvs.emplace_back(vertex.v0);

                // TODO: u1,v1
            }
        } else {
            const auto& vb     = m_VertexBuffer->CastData<PackedVertexPosition>();
            const auto& vbdata = m_VertexBufferData->CastData<GeneralShortPacked>();

            for (auto i = 0; i < vb.size(); ++i) {
                auto& vertex = vb[i];
                auto& data   = vbdata[i];

                vertices.emplace_back(unpack(vertex.x) * m_Block.attributes.packed.scale);
                vertices.emplace_back(unpack(vertex.y) * m_Block.attributes.packed.scale);
                vertices.emplace_back(unpack(vertex.z) * m_Block.attributes.packed.scale);
                uvs.emplace_back(unpack(data.u0) * m_Block.attributes.packed.uv0Extent.x);
                uvs.emplace_back(unpack(data.v0) * m_Block.attributes.packed.uv0Extent.y);

                // TODO: u1,v1
            }
        }

        return {vertices, indices, uvs};
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;

        IRenderBlock::Setup(context);

        // setup the constant buffer
        {
            const auto scale = m_Block.attributes.packed.scale * m_ScaleModifier;

            // set vertex shader constants
            m_cbVertexInstanceConsts.viewProjection          = context->m_viewProjectionMatrix;
            m_cbVertexInstanceConsts.world                   = glm::scale(glm::mat4(1), {scale, scale, scale});
            m_cbVertexInstanceConsts.colour                  = glm::vec4(1, 1, 1, 1);
            m_cbVertexInstanceConsts._thing                  = glm::vec4(0, 1, 2, 1);
            m_cbVertexMaterialConsts.DepthBias               = m_Block.attributes.depthBias;
            m_cbVertexMaterialConsts.EmissiveStartFadeDistSq = m_Block.attributes.startFadeOutDistanceEmissiveSq;
            m_cbVertexMaterialConsts._unknown                = 0;
            m_cbVertexMaterialConsts._unknown2               = 0;
            m_cbVertexMaterialConsts.uv0Extent               = m_Block.attributes.packed.uv0Extent;
            m_cbVertexMaterialConsts.uv1Extent               = m_Block.attributes.packed.uv1Extent;

            // set fragment shader constants
            m_cbFragmentMaterialConsts.specularGloss    = m_Block.attributes.specularGloss;
            m_cbFragmentMaterialConsts.reflectivity     = m_Block.attributes.reflectivity;
            m_cbFragmentMaterialConsts.specularFresnel  = m_Block.attributes.specularFresnel;
            m_cbFragmentMaterialConsts.diffuseModulator = m_Block.attributes.diffuseModulator;
            m_cbFragmentInstanceConsts.colour           = glm::vec4(1, 1, 1, 0);
        }

        // set the textures
        for (int i = 0; i < m_Textures.size(); ++i) {
            IRenderBlock::BindTexture(i, m_SamplerState);
        }

        // set the constant buffers
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 1, m_cbVertexInstanceConsts);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 2, m_cbVertexMaterialConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[0], 1, m_cbFragmentMaterialConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[1], 2, m_cbFragmentInstanceConsts);

        // set the culling mode
        context->m_Renderer->SetCullMode((!(m_Block.attributes.flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK
                                                                                                  : D3D11_CULL_NONE);

        // if we are using packed vertices, set the 2nd vertex buffer
        if (m_Block.attributes.packed.format == 1) {
            context->m_Renderer->SetVertexStream(m_VertexBufferData, 1);
        }
    }

    virtual void DrawUI() override final
    {
        // clang-format off
        static std::array flag_labels = {
            "Disable Backface Culling",     "Transparency Alpha Blending",  "Dynamic Emissive",             "",
            "",                             "",                             "",                             ""
        };
        // clang-format on

        ImGui::Text(ICON_FA_COGS "  Attributes");

        ImGuiCustom::BitFieldTooltip("Flags", &m_Block.attributes.flags, flag_labels);

        ImGui::SliderFloat("Scale", &m_ScaleModifier, 0.1f, 10.0f);
        ImGui::SliderFloat("Depth Bias", &m_Block.attributes.depthBias, 0.0f, 10.0f);
        ImGui::SliderFloat("Specular Gloss", &m_Block.attributes.specularGloss, 0.0f, 10.0f);
        ImGui::SliderFloat("Reflectivity", &m_Block.attributes.reflectivity, 0.0f, 10.0f);
        ImGui::SliderFloat("Emissive", &m_Block.attributes.emmissive, 0.0f, 10.0f);
        ImGui::SliderFloat("Diffuse Wrap", &m_Block.attributes.diffuseWrap, 0.0f, 10.0f);
        ImGui::SliderFloat("Specular Fresnel", &m_Block.attributes.specularFresnel, 0.0f, 10.0f);
        ImGui::ColorEdit3("Diffuse Modulator", glm::value_ptr(m_Block.attributes.diffuseModulator));
        ImGui::SliderFloat("Back Light", &m_Block.attributes.backLight, 0.0f, 10.0f);
        ImGui::SliderFloat("Unknown #2", &m_Block.attributes._unknown2, 0.0f, 10.0f);
        ImGui::InputFloat("Fade Out Distance Emissive", &m_Block.attributes.startFadeOutDistanceEmissiveSq);

        // Textures
        ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
        ImGui::Columns(3, nullptr, false);
        {
            UI::Get()->RenderBlockTexture("DiffuseMap", m_Textures[0]);
            UI::Get()->RenderBlockTexture("NormalMap", m_Textures[1]);
            UI::Get()->RenderBlockTexture("PropertiesMap", m_Textures[2]);
            UI::Get()->RenderBlockTexture("AOBlendMap", m_Textures[3]);
        }
        ImGui::EndColumns();
    }
};