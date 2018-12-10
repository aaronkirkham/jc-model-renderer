#pragma once

#include "irenderblock.h"

#pragma pack(push, 1)
struct PropAttributes {
    float                        depthBias;
    char                         _pad[0xFC];
    jc::Vertex::SPackedAttribute packed;
    uint32_t                     flags;
};

namespace jc::RenderBlocks
{
static constexpr uint8_t PROP_VERSION = 3;

struct Prop {
    uint8_t        version;
    PropAttributes attributes;
};
}; // namespace jc::RenderBlocks
#pragma pack(pop)

class RenderBlockProp : public IRenderBlock
{
  private:
    enum { DISABLE_BACKFACE_CULLING = 1, TRANSPARENCY_ALPHABLENDING = 2, SUPPORT_DECALS = 4, SUPPORT_OVERLAY = 8 };

    struct cbVertexInstanceConsts {
        glm::mat4   ViewProjection;
        glm::mat4   World;
        glm::mat4x3 Unknown = glm::mat4x3(1);
        glm::vec4   Colour;
        glm::vec4   Unknown2;
    } m_cbVertexInstanceConsts;

    struct cbVertexMaterialConsts {
        glm::vec4 DepthBias;
        glm::vec2 uv0Extent;
        glm::vec2 uv1Extent;
    } m_cbVertexMaterialConsts;

    struct cbFragmentMaterialConsts {
        glm::vec4 _pad[16];
    } m_cbFragmentMaterialConsts;

    jc::RenderBlocks::Prop           m_Block;
    VertexBuffer_t*                  m_VertexBufferData        = nullptr;
    std::array<ConstantBuffer_t*, 2> m_VertexShaderConstants   = {nullptr};
    ConstantBuffer_t*                m_FragmentShaderConstants = nullptr;

  public:
    RenderBlockProp() = default;
    virtual ~RenderBlockProp()
    {
        Renderer::Get()->DestroyBuffer(m_VertexBufferData);

        for (auto& vsc : m_VertexShaderConstants)
            Renderer::Get()->DestroyBuffer(vsc);

        Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants);
    }

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockProp";
    }

    virtual uint32_t GetTypeHash() const override final
    {
        return RenderBlockFactory::RB_PROP;
    }

    virtual bool IsOpaque() override final
    {
        const auto flags = m_Block.attributes.flags;
        return !(flags & TRANSPARENCY_ALPHABLENDING) && !(flags & SUPPORT_OVERLAY);
    }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader("prop");
        m_PixelShader  = ShaderManager::Get()->GetPixelShader("prop");

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
                                                                           "RenderBlockProp (unpacked)");
        } else {
            // clang-format off
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };
            // clang-format on

            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(),
                                                                           "RenderBlockProp (packed)");
        }

        // create the constant buffer
        m_VertexShaderConstants[0] =
            Renderer::Get()->CreateConstantBuffer(m_cbVertexInstanceConsts, "RenderBlockProp cbVertexInstanceConsts");
        m_VertexShaderConstants[1] =
            Renderer::Get()->CreateConstantBuffer(m_cbVertexMaterialConsts, "RenderBlockProp cbVertexMaterialConsts");
        m_FragmentShaderConstants = Renderer::Get()->CreateConstantBuffer(m_cbFragmentMaterialConsts,
                                                                          "RenderBlockProp cbFragmentMaterialConsts");

        // TEMP
        memset(&m_cbFragmentMaterialConsts, 0, sizeof(m_cbFragmentMaterialConsts));
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace jc::Vertex;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));

        if (m_Block.version != jc::RenderBlocks::PROP_VERSION) {
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
            m_cbVertexInstanceConsts.ViewProjection = context->m_viewProjectionMatrix;
            m_cbVertexInstanceConsts.World          = world;
            m_cbVertexInstanceConsts.Colour         = {0, 0, 0, 1};
            m_cbVertexMaterialConsts.DepthBias      = {m_Block.attributes.depthBias, 0, 0, 0};
            m_cbVertexMaterialConsts.uv0Extent      = m_Block.attributes.packed.uv0Extent;
            m_cbVertexMaterialConsts.uv1Extent      = m_Block.attributes.packed.uv1Extent;

            // set fragment shader constants
            //
        }

        // set the diffuse texture
        IRenderBlock::BindTexture(0, m_SamplerState);

        if (!(m_Block.attributes.flags & SUPPORT_OVERLAY)) {
            // set the culling mode
            context->m_Renderer->SetCullMode(
                (!(m_Block.attributes.flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);

            // set the normals & properties textures
            IRenderBlock::BindTexture(1, m_SamplerState);
            IRenderBlock::BindTexture(2, m_SamplerState);

            // set the decal textures
            if (m_Block.attributes.flags & SUPPORT_DECALS) {
                IRenderBlock::BindTexture(3, 4, m_SamplerState);
                IRenderBlock::BindTexture(4, 5, m_SamplerState);
                IRenderBlock::BindTexture(5, 6, m_SamplerState);
            }
        }

        // set the constant buffers
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 1, m_cbVertexInstanceConsts);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 2, m_cbVertexMaterialConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants, 1, m_cbFragmentMaterialConsts);

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
            "Disable Backface Culling",     "Transparency Alpha Blending",  "Support Decals",				"Support Overlay",
        };
        // clang-format on

        ImGuiCustom::DropDownFlags(m_Block.attributes.flags, flag_labels);
    }

    virtual void DrawUI() override final
    {
        ImGui::Text(ICON_FA_COGS "  Attributes");

        ImGui::SliderFloat("Scale", &m_ScaleModifier, 0.1f, 10.0f);
        ImGui::SliderFloat("Depth Bias", &m_Block.attributes.depthBias, 0.0f, 10.0f);

        // Textures
        ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
        ImGui::Columns(3, nullptr, false);
        {
            IRenderBlock::DrawTexture("DiffuseMap", 0);

            if (!(m_Block.attributes.flags & SUPPORT_OVERLAY)) {
                IRenderBlock::DrawTexture("NormalMap", 1);
                IRenderBlock::DrawTexture("PropertiesMap", 2);

                if (m_Block.attributes.flags & SUPPORT_DECALS) {
                    IRenderBlock::DrawTexture("DecalDiffuseMap", 3);
                    IRenderBlock::DrawTexture("DecalNormalMap", 4);
                    IRenderBlock::DrawTexture("DecalPropertiesMap", 5);
                }
            }
        }
        ImGui::EndColumns();
    }
};
