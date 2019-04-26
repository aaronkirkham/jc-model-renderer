#pragma once

#include "irenderblock.h"

namespace jc4
{
class RenderBlockCharacter : public IRenderBlock
{
  private:
    struct cbLocalConsts {
        glm::mat4 WorldViewProjection;
        glm::mat4 World;
    } m_cbLocalConsts;

    struct cbSkinningConsts {
        glm::mat3x4 MatrixPalette[70];
    } m_cbSkinningConsts;

    struct cbInstanceConsts {
        glm::vec2 TilingUV = glm::vec2(1, 1);
        glm::vec2 Unknown;
        glm::vec4 Scale = glm::vec4(1, 1, 1, 1);
    } m_cbInstanceConsts;

    struct cbUnknownCB1 {
        glm::vec4 Unknown       = {0, 0, 0, 0};
        glm::vec4 SnowFactor    = glm::vec4(0, 0, 0, 0); // this is really just a float with 12 bytes of padding
        glm::vec4 Unknown2[5]   = {{0, 0, 0, 0}};
        glm::vec4 DiffuseColour = glm::vec4(0, 0, 0, 1);
        glm::vec4 Unknown3[4]   = {{0, 0, 0, 0}};
    } m_cbUnknownCB1;

    struct cbUnknownCB4 {
        glm::vec4 Unknown[5];
    } m_cbUnknownCB4;

    struct cbUnknownCB6 {
        glm::vec4 Unknown[46];
    } m_cbUnknownCB6;

    std::array<ConstantBuffer_t*, 3> m_VertexShaderConstants = {nullptr};
    std::array<ConstantBuffer_t*, 3> m_PixelShaderConstants  = {nullptr};

  public:
    RenderBlockCharacter() = default;
    virtual ~RenderBlockCharacter()
    {
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[0]);
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[1]);
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[2]);
        Renderer::Get()->DestroyBuffer(m_PixelShaderConstants[0]);
        Renderer::Get()->DestroyBuffer(m_PixelShaderConstants[1]);
        Renderer::Get()->DestroyBuffer(m_PixelShaderConstants[2]);
    }

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockCharacter";
    }

    virtual uint32_t GetTypeHash() const override final
    {
        return RenderBlockFactory::RB_CHARACTER;
    }

    virtual bool IsOpaque() override final
    {
        // TODO
        // Find out where the flags are stored. They must be on the modelc "Attributes" reference that we stil cannot
        // parse within ADF (0xDEFE88ED type)

        /*
            _flags = *(v2 + 0x258);
            if ( _bittest(&_flags, 0x10u) || _flags & 8 )
                result = 0;
        */

        return true;
    }

    virtual void Create(FileBuffer* vertices, FileBuffer* indices) override final
    {
        using namespace jc::Vertex::RenderBlockCharacter;

        // TODO: use the correct buffer strides from the meshc.

        // init buffers
        m_VertexBuffer = Renderer::Get()->CreateBuffer(vertices->data(), (vertices->size() / sizeof(Packed4Bones1UV)),
                                                       sizeof(Packed4Bones1UV), VERTEX_BUFFER);
        m_IndexBuffer  = Renderer::Get()->CreateBuffer(indices->data(), (indices->size() / sizeof(uint16_t)),
                                                      sizeof(uint16_t), INDEX_BUFFER);

        // TODO: get stride! (we should pass IBuffer_t* to the Create() function and read the element stride from it)
        {
            m_PixelShader  = ShaderManager::Get()->GetPixelShader("character");
            m_VertexShader = ShaderManager::Get()->GetVertexShader("character");

            // create the element input desc
            // clang-format off
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   4,  DXGI_FORMAT_R16G16_SNORM,           0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   6,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };
            // clang-format on

            // create the vertex declaration
            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 5, m_VertexShader.get(),
                                                                           "RenderBlockCharacter (4bones1uv)");
        }

        // create vertex shader constants
        m_VertexShaderConstants[0] =
            Renderer::Get()->CreateConstantBuffer(m_cbLocalConsts, "RenderBlockCharacter cbLocalConsts");
        m_VertexShaderConstants[1] =
            Renderer::Get()->CreateConstantBuffer(m_cbSkinningConsts, "RenderBlockCharacter cbSkinningConsts");
        m_VertexShaderConstants[2] =
            Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "RenderBlockCharacter cbInstanceConsts");

        // create pixel shader constants
        m_PixelShaderConstants[0] =
            Renderer::Get()->CreateConstantBuffer(m_cbUnknownCB1, "RenderBlockCharacter cbUnknownCB1");
        m_PixelShaderConstants[1] =
            Renderer::Get()->CreateConstantBuffer(m_cbUnknownCB4, "RenderBlockCharacter cbUnknownCB4");
        m_PixelShaderConstants[2] =
            Renderer::Get()->CreateConstantBuffer(m_cbUnknownCB6, "RenderBlockCharacter cbUnknownCB6");

        // init the skinning palette data
        for (auto i = 0; i < 70; ++i) {
            m_cbSkinningConsts.MatrixPalette[i] = glm::mat3x4(1);
        }

        //
        memset(&m_cbUnknownCB4, 0, sizeof(m_cbUnknownCB4));
        memset(&m_cbUnknownCB6, 0, sizeof(m_cbUnknownCB6));

        // create sampler state
        D3D11_SAMPLER_DESC params{};
        params.Filter         = D3D11_FILTER_ANISOTROPIC;
        params.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
        params.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
        params.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
        params.MipLODBias     = 0.0f;
        params.MaxAnisotropy  = 8;
        params.ComparisonFunc = D3D11_COMPARISON_NEVER;
        params.MinLOD         = 0.0f;
        params.MaxLOD         = 13.0f;
        m_SamplerState        = Renderer::Get()->CreateSamplerState(params, "RenderBlockCharacter");
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        if (!m_Visible) {
            return;
        }

        IRenderBlock::Setup(context);

        static auto world = glm::mat4(1);

        // set vertex shader constants
        m_cbLocalConsts.WorldViewProjection = world * context->m_viewProjectionMatrix;
        m_cbLocalConsts.World               = world;
        m_cbInstanceConsts.Scale            = glm::vec4(1 * m_ScaleModifier);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 2, m_cbLocalConsts);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 3, m_cbSkinningConsts);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[2], 4, m_cbInstanceConsts);

        // set pixel shader constants
        context->m_Renderer->SetPixelShaderConstants(m_PixelShaderConstants[0], 1, m_cbUnknownCB1);
        context->m_Renderer->SetPixelShaderConstants(m_PixelShaderConstants[1], 4, m_cbUnknownCB4);
        context->m_Renderer->SetPixelShaderConstants(m_PixelShaderConstants[2], 6, m_cbUnknownCB6);

        // set textures
        IRenderBlock::BindTexture(0, m_SamplerState);
        IRenderBlock::BindTexture(1, m_SamplerState);
        IRenderBlock::BindTexture(2, m_SamplerState);
    }

    virtual void DrawContextMenu() override final {}

    virtual void DrawUI() override final
    {
        ImGui::Text(ICON_FA_COGS "  Attributes");
        ImGui::SliderFloat("Scale", &m_ScaleModifier, 0.0f, 20.0f);

        if (ImGui::CollapsingHeader("cbUnknownCB1")) {
            /*ImGui::SliderFloat4("cb1_0", glm::value_ptr(m_cbUnknownCB1.Unknown[0]), 0.0f, 1.0f);*/
            ImGui::SliderFloat("Snow Factor", glm::value_ptr(m_cbUnknownCB1.SnowFactor), 0.0f, 1.0f);
            /*ImGui::SliderFloat4("cb1_2", glm::value_ptr(m_cbUnknownCB1.Unknown[2]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb1_3", glm::value_ptr(m_cbUnknownCB1.Unknown[3]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb1_4", glm::value_ptr(m_cbUnknownCB1.Unknown[4]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb1_5", glm::value_ptr(m_cbUnknownCB1.Unknown[5]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb1_6", glm::value_ptr(m_cbUnknownCB1.Unknown[6]), 0.0f, 1.0f);*/
            ImGui::ColorEdit4("Diffuse Colour", glm::value_ptr(m_cbUnknownCB1.DiffuseColour));
            /*ImGui::SliderFloat4("cb1_8", glm::value_ptr(m_cbUnknownCB1.Unknown[8]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb1_9", glm::value_ptr(m_cbUnknownCB1.Unknown[9]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb1_10", glm::value_ptr(m_cbUnknownCB1.Unknown[10]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb1_11", glm::value_ptr(m_cbUnknownCB1.Unknown[11]), 0.0f, 1.0f);*/
        }

        if (ImGui::CollapsingHeader("cbUnknownCB4")) {
            ImGui::SliderFloat4("cb4_0", glm::value_ptr(m_cbUnknownCB4.Unknown[0]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb4_1", glm::value_ptr(m_cbUnknownCB4.Unknown[1]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb4_2", glm::value_ptr(m_cbUnknownCB4.Unknown[2]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb4_3", glm::value_ptr(m_cbUnknownCB4.Unknown[3]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb4_4", glm::value_ptr(m_cbUnknownCB4.Unknown[4]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb4_5", glm::value_ptr(m_cbUnknownCB4.Unknown[5]), 0.0f, 1.0f);
        }

        if (ImGui::CollapsingHeader("cbUnknownCB6")) {
            ImGui::SliderFloat4("cb6_0", glm::value_ptr(m_cbUnknownCB6.Unknown[0]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_1", glm::value_ptr(m_cbUnknownCB6.Unknown[1]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_2", glm::value_ptr(m_cbUnknownCB6.Unknown[2]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_3", glm::value_ptr(m_cbUnknownCB6.Unknown[3]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_4", glm::value_ptr(m_cbUnknownCB6.Unknown[4]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_5", glm::value_ptr(m_cbUnknownCB6.Unknown[5]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_6", glm::value_ptr(m_cbUnknownCB6.Unknown[6]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_7", glm::value_ptr(m_cbUnknownCB6.Unknown[7]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_8", glm::value_ptr(m_cbUnknownCB6.Unknown[8]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_9", glm::value_ptr(m_cbUnknownCB6.Unknown[9]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_10", glm::value_ptr(m_cbUnknownCB6.Unknown[10]), 0.0f, 1.0f);
            ImGui::SliderFloat4("cb6_11", glm::value_ptr(m_cbUnknownCB6.Unknown[11]), 0.0f, 1.0f);
        }

        // Textures
        ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
        ImGui::Columns(3, nullptr, false);
        {
            for (auto i = 0; i < m_Textures.size(); ++i) {
                const auto name = "Texture-" + std::to_string(i);
                IRenderBlock::DrawUI_Texture(name, i);
            }
        }
        ImGui::EndColumns();
    }

    virtual void SetData(vertices_t* vertices, uint16s_t* indices, materials_t* materials) override final
    {
        //
    }

    virtual std::tuple<vertices_t, uint16s_t> GetData() override final
    {
        using namespace jc::Vertex;
        using namespace jc::Vertex::RenderBlockCharacter;

        vertices_t vertices;
        uint16s_t  indices = m_IndexBuffer->CastData<uint16_t>();

        // TODO: use the correct stride, when we load it correctly!

        const auto& vb = m_VertexBuffer->CastData<Packed4Bones1UV>();
        vertices.reserve(vb.size());
        for (const auto& vertex : vb) {
            vertex_t v{};
            v.pos = glm::vec3{unpack(vertex.x), unpack(vertex.y), unpack(vertex.z)};
            v.uv  = glm::vec2{unpack(vertex.u0), unpack(vertex.v0)};
            vertices.emplace_back(std::move(v));
        }

        return {vertices, indices};
    }
};
} // namespace jc4
