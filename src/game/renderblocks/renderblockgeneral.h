#pragma once

#include "irenderblock.h"

#pragma pack(push, 1)
struct GeneralAttributes {
    float                        depthBias;
    float                        specularGloss;
    jc::Vertex::SPackedAttribute packed;
    uint32_t                     flags;
    glm::vec3                    boundingBoxMin;
    glm::vec3                    boundingBoxMax;
};

namespace jc::RenderBlocks
{
static constexpr uint8_t GENERAL_VERSION = 6;

struct General {
    uint8_t           version;
    GeneralAttributes attributes;
};
}; // namespace jc::RenderBlocks
#pragma pack(pop)

class RenderBlockGeneral : public IRenderBlock
{
  private:
    enum { DISABLE_BACKFACE_CULLING = 0x1, TRANSPARENCY_ALPHABLENDING = 0x2, TRANSPARENCY_ALPHATEST = 0x16 };

    struct cbVertexInstanceConsts {
        glm::mat4   ViewProjection;
        glm::mat4   World;
        glm::vec4   _unknown[3];
        glm::vec4   _thing;
        glm::mat4x3 _thing2;
    } m_cbVertexInstanceConsts;

    struct cbVertexMaterialConsts {
        glm::vec4 DepthBias;
        glm::vec2 uv0Extent;
        glm::vec2 uv1Extent;
    } m_cbVertexMaterialConsts;

    struct cbFragmentMaterialConsts {
        float specularGloss;
        float _unknown   = 1.0f;
        float _unknown2  = 0.0f;
        float _unknown3  = 1.0f;
        float _unknown4  = 0.0f;
        float _unknown5  = 0.0f;
        float _unknown6  = 0.0f;
        float _unknown7  = 0.0f;
        float _unknown8  = 0.0f;
        float _unknown9  = 0.0f;
        float _unknown10 = 0.0f;
        float _unknown11 = 0.0f;
    } m_cbFragmentMaterialConsts;

    jc::RenderBlocks::General        m_Block;
    VertexBuffer_t*                  m_VertexBufferData       = nullptr;
    std::array<ConstantBuffer_t*, 2> m_VertexShaderConstants  = {nullptr};
    ConstantBuffer_t*                m_FragmentShaderConstant = nullptr;

  public:
    RenderBlockGeneral() = default;
    virtual ~RenderBlockGeneral()
    {
        Renderer::Get()->DestroyBuffer(m_VertexBufferData);

        for (auto& vsc : m_VertexShaderConstants)
            Renderer::Get()->DestroyBuffer(vsc);

        Renderer::Get()->DestroyBuffer(m_FragmentShaderConstant);
    }

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockGeneral";
    }

    virtual uint32_t GetTypeHash() const override final
    {
        return RenderBlockFactory::RB_GENERAL;
    }

    virtual bool IsOpaque() override final
    {
        return ~m_Block.attributes.flags & TRANSPARENCY_ALPHABLENDING;
    }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader("general");
        m_PixelShader  = ShaderManager::Get()->GetPixelShader("general");

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
                                                                           "RenderBlockGeneral (unpacked)");
        } else {
            // clang-format off
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };
            // clang-format on

            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(),
                                                                           "RenderBlockGeneral (packed)");
        }

        // create the constant buffer
        m_VertexShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbVertexInstanceConsts,
                                                                           "RenderBlockGeneral cbVertexInstanceConsts");
        m_VertexShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(m_cbVertexMaterialConsts,
                                                                           "RenderBlockGeneral cbVertexMaterialConsts");
        m_FragmentShaderConstant   = Renderer::Get()->CreateConstantBuffer(m_cbFragmentMaterialConsts,
                                                                         "RenderBlockGeneral cbFragmentMaterialConsts");
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace jc::Vertex;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));

        if (m_Block.version != jc::RenderBlocks::GENERAL_VERSION) {
            __debugbreak();
        }

        // read the block materials
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

    virtual void SetData(vertices_t* vertices, uint16s_t* indices, materials_t* materials) override final
    {
        //
    }

    virtual std::tuple<vertices_t, uint16s_t> GetData() override final
    {
        using namespace jc::Vertex;

        vertices_t vertices;
        uint16s_t  indices = m_IndexBuffer->CastData<uint16_t>();

        if (m_Block.attributes.packed.format != 1) {
            const auto& vb = m_VertexBuffer->CastData<UnpackedVertexPosition2UV>();
            vertices.reserve(vb.size());

            for (const auto& vertex : vb) {
                vertex_t v;
                v.pos    = {vertex.x, vertex.y, vertex.z};
                v.uv     = glm::vec2{unpack(vertex.u0), unpack(vertex.v0)} * m_Block.attributes.packed.uv0Extent;
                v.normal = unpack_normal(vertex.n);
                vertices.emplace_back(std::move(v));
            }
        } else {
            const auto& vb     = m_VertexBuffer->CastData<PackedVertexPosition>();
            const auto& vbdata = m_VertexBufferData->CastData<GeneralShortPacked>();
            vertices.reserve(vb.size());

            for (auto i = 0; i < vb.size(); ++i) {
                auto& vertex = vb[i];
                auto& data   = vbdata[i];

                vertex_t v;
                v.pos    = {unpack(vertex.x), unpack(vertex.y), unpack(vertex.z)};
                v.uv     = glm::vec2{unpack(data.u0), unpack(data.v0)} * m_Block.attributes.packed.uv0Extent;
                v.normal = unpack_normal(data.n);

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
            const auto scale = m_Block.attributes.packed.scale * m_ScaleModifier;
            const auto world = glm::scale(glm::mat4(1), {scale, scale, scale});

            // set vertex shader constants
            m_cbVertexInstanceConsts.ViewProjection = world * context->m_viewProjectionMatrix;
            // m_cbVertexInstanceConsts.World          = world;
            // m_cbVertexInstanceConsts.colour         = glm::vec4(1, 1, 1, 1);
            // m_cbVertexInstanceConsts._thing         = glm::vec4(0, 1, 2, 1);
            m_cbVertexInstanceConsts._thing    = glm::vec4(0, 0, 0, 1);
            m_cbVertexInstanceConsts._thing2   = world;
            m_cbVertexMaterialConsts.DepthBias = {m_Block.attributes.depthBias, 0, 0, 0};
            m_cbVertexMaterialConsts.uv0Extent = m_Block.attributes.packed.uv0Extent;
            m_cbVertexMaterialConsts.uv1Extent = m_Block.attributes.packed.uv1Extent;

            // set fragment shader constants
            m_cbFragmentMaterialConsts.specularGloss = m_Block.attributes.specularGloss;
        }

        // set the textures
        for (int i = 0; i < m_Textures.size(); ++i) {
            IRenderBlock::BindTexture(i, m_SamplerState);
        }

        // set the constant buffers
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 1, m_cbVertexInstanceConsts);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 2, m_cbVertexMaterialConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstant, 2, m_cbFragmentMaterialConsts);

        // set the culling mode
        context->m_Renderer->SetCullMode((!(m_Block.attributes.flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK
                                                                                                  : D3D11_CULL_NONE);

        // if we are using packed vertices, set the 2nd vertex buffer
        if (m_Block.attributes.packed.format == 1) {
            context->m_Renderer->SetVertexStream(m_VertexBufferData, 1);
        }
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;

        IRenderBlock::Draw(context);
    }

    virtual void DrawContextMenu() override final
    {
        // clang-format off
        static std::array flag_labels = {
            "Disable Backface Culling",     "Transparency Alpha Blending",  "",								"",
            "Transparency Alpha Testing",   "",                             "",                             ""
        };
        // clang-format on

        ImGuiCustom::DropDownFlags(m_Block.attributes.flags, flag_labels);
    }

    virtual void DrawUI() override final
    {
        ImGui::Text(ICON_FA_COGS "  Attributes");

        ImGui::SliderFloat("Scale", &m_ScaleModifier, 0.1f, 10.0f);
        ImGui::SliderFloat("Depth Bias", &m_Block.attributes.depthBias, 0.0f, 10.0f);
        ImGui::SliderFloat("Specular Gloss", &m_Block.attributes.specularGloss, 0.0f, 10.0f);

        // Textures
        ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
        ImGui::Columns(3, nullptr, false);
        {
            IRenderBlock::DrawTexture("DiffuseMap", 0);
            IRenderBlock::DrawTexture("NormalMap", 1);
            IRenderBlock::DrawTexture("PropertiesMap", 2);
            IRenderBlock::DrawTexture("AOBlendMap", 3);
        }
        ImGui::EndColumns();
    }
};
