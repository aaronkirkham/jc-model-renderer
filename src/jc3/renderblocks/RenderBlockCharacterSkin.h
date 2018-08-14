#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct CharacterSkinAttributes
{
    uint32_t flags;
    float scale;
    char pad[0x30];
};

namespace JustCause3::RenderBlocks
{
    struct CharacterSkin
    {
        uint8_t version;
        CharacterSkinAttributes attributes;
    };
};
#pragma pack(pop)

class RenderBlockCharacterSkin : public IRenderBlock
{
private:
    struct cbLocalConsts
    {
        glm::mat4 World;
        glm::mat4 WorldViewProjection;
        glm::vec4 Scale;
        glm::mat3x4 MatrixPalette[70];
    } m_cbLocalConsts;

    struct cbInstanceConsts
    {
        glm::vec4 MotionBlur = glm::vec4(0);
    } m_cbInstanceConsts;

    struct cbMaterialConsts
    {
        glm::vec4 EyeGloss = glm::vec4(0);
        glm::vec4 _unknown_;
        glm::vec4 _unknown[3];
    } m_cbMaterialConsts;

    JustCause3::RenderBlocks::CharacterSkin m_Block;
    ConstantBuffer_t* m_VertexShaderConstants = nullptr;
    std::array<ConstantBuffer_t*, 2> m_FragmentShaderConstants = { nullptr };
    int64_t m_Stride = 0;

    int64_t GetStride() const
    {
        static const int32_t strides[] = { 0x18, 0x1C, 0x20, 0x20, 0x24, 0x28 };
        return strides[3 * ((m_Block.attributes.flags >> 2) & 1) + ((m_Block.attributes.flags >> 1) & 1) + ((m_Block.attributes.flags >> 4) & 1)];
    }

public:
    RenderBlockCharacterSkin() = default;
    virtual ~RenderBlockCharacterSkin()
    {
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants);

        for (auto& fsc : m_FragmentShaderConstants)
            Renderer::Get()->DestroyBuffer(fsc);
    }

    virtual const char* GetTypeName() override final { return "RenderBlockCharacterSkin"; }
    virtual bool IsOpaque() override final { return true; }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader("characterskin");
        m_PixelShader = ShaderManager::Get()->GetPixelShader("characterskin");

        // create the element input desc
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   1,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   4,  DXGI_FORMAT_R16G16_SNORM,           0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   6,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };

        // create the vertex declaration
        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 5, m_VertexShader.get(), "RenderBlockCharacterSkin");

        // create the constant buffer
        m_VertexShaderConstants = Renderer::Get()->CreateConstantBuffer(m_cbLocalConsts, "RenderBlockCharacterSkin cbLocalConsts");
        m_FragmentShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "RenderBlockCharacterSkin cbInstanceConsts");
        m_FragmentShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts, "RenderBlockCharacterSkin cbMaterialConsts");

        // identity the palette data
        for (int i = 0; i < 70; ++i) {
            m_cbLocalConsts.MatrixPalette[i] = glm::mat3x4(1);
        }

        //
        memset(&m_cbMaterialConsts._unknown, 0, sizeof(m_cbMaterialConsts._unknown));

#if 0
        // create the sampler states
        {
            SamplerStateParams_t params;
            params.m_AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            params.m_MinMip = 12.0f;
            params.m_MaxMip = 13.0f;

            m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockCharacterSkin");
        }
#endif
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace JustCause3::Vertex;
        using namespace JustCause3::Vertex::RenderBlockCharacterSkin;

        // read the block header
        stream.read((char *)&m_Block, sizeof(m_Block));

        // read the materials
        ReadMaterials(stream);

        // get the vertices stride
        m_Stride = GetStride();

#ifdef DEBUG
        int game_using_vertex_decl = 3 * ((m_Block.attributes.flags >> 2) & 1) + ((m_Block.attributes.flags >> 1) & 1) + ((m_Block.attributes.flags >> 4) & 1);
        game_using_vertex_decl = game_using_vertex_decl;
#endif

        // read vertex data
        // TODO: need to implement the different vertex types depending on the flags above (GetStride).
        // The stride should be the size of the struct we read from.
        {
            std::vector<PackedCharacterSkinPos4Bones1UVs> vertices;
            ReadVertexBuffer<PackedCharacterSkinPos4Bones1UVs>(stream, &m_VertexBuffer, &vertices);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(unpack(vertex.x));
                m_Vertices.emplace_back(unpack(vertex.y));
                m_Vertices.emplace_back(unpack(vertex.z));
                m_UVs.emplace_back(unpack(vertex.u0));
                m_UVs.emplace_back(unpack(vertex.v0));
            }
        }

        // read skin batch
        ReadSkinBatch(stream);

        // read index buffer
        ReadIndexBuffer(stream, &m_IndexBuffer);
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        if (!m_Visible) return;

        IRenderBlock::Setup(context);

        // setup the constant buffer
        {
            const auto scale = m_Block.attributes.scale;
            static auto world = glm::mat4(1);

            // set vertex shader constants
            m_cbLocalConsts.World = world;
            m_cbLocalConsts.WorldViewProjection = world * context->m_viewProjectionMatrix;
            m_cbLocalConsts.Scale = glm::vec4(m_Block.attributes.scale * m_ScaleModifier);

            // set fragment shader constants
            //
        }

        // set the constant buffers
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants, 1, m_cbLocalConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[0], 1, m_cbInstanceConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[1], 2, m_cbMaterialConsts);

        // set the culling mode
        context->m_Renderer->SetCullMode((!(m_Block.attributes.flags & 1)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);

        // setup blending
        if ((m_Block.attributes.flags >> 5) & 1) {
            context->m_Renderer->SetBlendingEnabled(false);
            context->m_Renderer->SetAlphaEnabled(false);
        }

        context->m_Renderer->SetBlendingEnabled(true);
        context->m_Renderer->SetBlendingFunc(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_ONE);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible) return;

        IRenderBlock::DrawSkinBatches(context);
    }

    virtual void DrawUI() override final
    {
        static std::array flag_labels = {
            "Disable Culling", "Use Wrinkle Map", "", "", "Use Feature", "Use Alpha Mask", "", "",
            "", "", "", "", "", "", "", "",
            "", "", "", "", "", "", "", "",
            "", "", "", "", "", "", "", ""
        };

        ImGuiCustom::BitFieldTooltip("Flags", &m_Block.attributes.flags, flag_labels);

        ImGui::SliderFloat("Scale", &m_ScaleModifier, 0.0f, 20.0f);
    }
};