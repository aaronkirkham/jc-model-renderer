#pragma once

#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct WindowAttributes {
    union {
        struct {
            float SpecGloss;
            float SpecFresnel;
            float DiffuseRougness;
            float TintPower;
            float MinAlpha;
            float UVScale;
        };

        float data[6];
    };

    glm::vec3 _unknown;
    uint32_t  flags;
};

static_assert(sizeof(WindowAttributes) == 0x28, "WindowAttributes alignment is wrong!");

namespace JustCause3::RenderBlocks
{
static constexpr uint8_t WINDOW_VERSION = 1;

struct Window {
    uint8_t          version;
    WindowAttributes attributes;
};
}; // namespace JustCause3::RenderBlocks
#pragma pack(pop)

class RenderBlockWindow : public IRenderBlock
{
  private:
    enum {
        ENABLE_BACKFACE_CULLING = 0x1,
        SIMPLE                  = 0x2,
    };

    struct cbInstanceConsts {
        glm::mat4 World;
        glm::mat4 WorldViewProjection;
    } m_cbInstanceConsts;

    struct cbMaterialConsts {
        float     SpecGloss;
        float     SpecFresnel;
        float     DiffuseRougness;
        float     TintPower;
        float     MinAlpha;
        float     UVScale;
        glm::vec2 unused_;                 // [unused]
        float     DamageMaskWeight = 1.0f; // [unused]
        float     DamageRampWeight = 0.5f; // [unused]
        float     Alpha            = 1.0f;
    } m_cbMaterialConsts;

    JustCause3::RenderBlocks::Window m_Block;
    ConstantBuffer_t*                m_VertexShaderConstants   = nullptr;
    ConstantBuffer_t*                m_FragmentShaderConstants = nullptr;

    SamplerState_t* _test = nullptr;

  public:
    RenderBlockWindow() = default;
    virtual ~RenderBlockWindow()
    {
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants);
        Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants);
        Renderer::Get()->DestroySamplerState(_test);
    }

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockWindow";
    }

    virtual uint32_t GetTypeHash() const override final
    {
        return RenderBlockFactory::RB_WINDOW;
    }

    virtual bool IsOpaque() override final
    {
        return false;
    }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader("window");
        m_PixelShader  = ShaderManager::Get()->GetPixelShader("window");

        // create the element input desc
        // clang-format off
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   0,  DXGI_FORMAT_R32G32B32A32_FLOAT,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   1,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };
        // clang-format on

        // create the vertex declaration
        m_VertexDeclaration =
            Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(), "RenderBlockWindow");

        // create the constant buffers
        m_VertexShaderConstants =
            Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "RenderBlockWindow InstanceConsts");
        m_FragmentShaderConstants =
            Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts, 3, "RenderBlockWindow MaterialConsts");

        // create the sampler states
        {
            D3D11_SAMPLER_DESC params{};
            params.Filter         = D3D11_FILTER_ANISOTROPIC;
            params.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
            params.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
            params.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
            params.MipLODBias     = 0.0f;
            params.MaxAnisotropy  = 2;
            params.ComparisonFunc = D3D11_COMPARISON_NEVER;
            params.MinLOD         = 0.0f;
            params.MaxLOD         = 13.0f;

            m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockWindow");

            // TODO: find out what the game is actually doing
            // couldn't find anything related to slot 15 in the RenderBlockWindow stuff
            params.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
            _test         = Renderer::Get()->CreateSamplerState(params, "RenderBlockWindow _test");
        }
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace JustCause3::Vertex;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));

        if (m_Block.version != JustCause3::RenderBlocks::WINDOW_VERSION) {
            __debugbreak();
        }

        // read materials
        ReadMaterials(stream);

        // read vertex buffer
        m_VertexBuffer = ReadVertexBuffer<UnpackedVertexPosition2UV>(stream);

        // read index buffer
        m_IndexBuffer = ReadIndexBuffer(stream);
    }

    virtual void Write(std::ostream& stream) override final
    {
        // write the block attributes
        stream.write((char*)&m_Block, sizeof(m_Block));

        // write the materials
        WriteMaterials(stream);

        // write the vertex buffer
        WriteBuffer(stream, m_VertexBuffer);

        // write the index buffer
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

        const auto& vb = m_VertexBuffer->CastData<UnpackedVertexPosition2UV>();

        for (const auto& vertex : vb) {
            vertices.emplace_back(vertex.x);
            vertices.emplace_back(vertex.y);
            vertices.emplace_back(vertex.z);
            uvs.emplace_back(vertex.u0);
            uvs.emplace_back(vertex.v0);

            // TODO: u1,v1
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
            static const auto world = glm::mat4(1);

            // set vertex shader constants
            m_cbInstanceConsts.World               = world;
            m_cbInstanceConsts.WorldViewProjection = world * context->m_viewProjectionMatrix;

            // set fragment shaders constants
            m_cbMaterialConsts.SpecGloss       = m_Block.attributes.SpecGloss;
            m_cbMaterialConsts.SpecFresnel     = m_Block.attributes.SpecFresnel;
            m_cbMaterialConsts.DiffuseRougness = m_Block.attributes.DiffuseRougness;
            m_cbMaterialConsts.TintPower       = m_Block.attributes.TintPower;
            m_cbMaterialConsts.MinAlpha        = m_Block.attributes.MinAlpha;
            m_cbMaterialConsts.UVScale         = m_Block.attributes.UVScale;
        }

        // set the textures
        IRenderBlock::BindTexture(0, m_SamplerState);

        if (!(m_Block.attributes.flags & SIMPLE)) {
            IRenderBlock::BindTexture(1, 2, m_SamplerState);
            IRenderBlock::BindTexture(2, 3, m_SamplerState);
        }

        // TODO: damage

        // set the sampler state
        // context->m_Renderer->SetSamplerState(m_SamplerState, 0);
        // context->m_Renderer->SetSamplerState(_test, 15);

#if 0
        // set lighting textures
        const auto gbuffer_diffuse = context->m_Renderer->GetGBufferSRV(0);
        context->m_DeviceContext->PSSetShaderResources(15, 1, &gbuffer_diffuse);
#endif

        // enable blending
        context->m_Renderer->SetBlendingEnabled(true);
        context->m_Renderer->SetBlendingFunc(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_DEST_ALPHA,
                                             D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_ONE);
        context->m_Renderer->SetBlendingEq(D3D11_BLEND_OP_ADD, D3D11_BLEND_OP_ADD);

        // enable depth
        context->m_Renderer->SetDepthEnabled(true);

        // set the constant buffers
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants, 1, m_cbInstanceConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants, 1, m_cbMaterialConsts);

        // set the culling mode
        if (m_Block.attributes.flags & ENABLE_BACKFACE_CULLING) {
            context->m_Renderer->SetCullMode(D3D11_CULL_BACK);
        }
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;

        if (!(m_Block.attributes.flags & ENABLE_BACKFACE_CULLING)) {
            context->m_Renderer->SetCullMode(D3D11_CULL_FRONT);
            IRenderBlock::Draw(context);
            context->m_Renderer->SetCullMode(D3D11_CULL_BACK);
        }

        IRenderBlock::Draw(context);
    }

    virtual void DrawUI() override final
    {
        // clang-format off
        static std::array flag_labels = {
            "Enable Backface Culling",      "Simple",                       "",                             "",
            "",                             "",                             "",                             ""
        };
        // clang-format on

        ImGui::Text(ICON_FA_COGS "  Attributes");

        ImGuiCustom::BitFieldTooltip("Flags", &m_Block.attributes.flags, flag_labels);

        ImGui::SliderFloat("Specular Gloss", &m_Block.attributes.SpecGloss, 0, 1);
        ImGui::SliderFloat("Specular Fresnel", &m_Block.attributes.SpecFresnel, 0, 1);
        ImGui::SliderFloat("Diffuse Rougness", &m_Block.attributes.DiffuseRougness, 0, 1);
        ImGui::SliderFloat("Tint Power", &m_Block.attributes.TintPower, 0, 10);
        ImGui::SliderFloat("Min Alpha", &m_Block.attributes.MinAlpha, 0, 1);
        ImGui::SliderFloat("UV Scale", &m_Block.attributes.UVScale, 0, 1);
        ImGui::SliderFloat("Alpha", &m_cbMaterialConsts.Alpha, 0, 1);

        // Textures
        ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
        ImGui::Columns(3, nullptr, false);
        {
            UI::Get()->RenderBlockTexture("DiffuseMap", m_Textures[0]);

            if (!(m_Block.attributes.flags & SIMPLE)) {
                UI::Get()->RenderBlockTexture("NormalMap", m_Textures[1]);
                UI::Get()->RenderBlockTexture("PropertyMap", m_Textures[2]);
            }

            // damage
            UI::Get()->RenderBlockTexture("DamagePointNormalMap", m_Textures[3]);
            UI::Get()->RenderBlockTexture("DamagePointPropertyMap", m_Textures[4]);
        }
        ImGui::EndColumns();
    }
};
