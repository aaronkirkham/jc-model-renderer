#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct CharacterSkinAttributes {
    uint32_t flags;
    float    scale;
    char     pad[0x30];
};

namespace JustCause3::RenderBlocks
{
struct CharacterSkin {
    uint8_t                 version;
    CharacterSkinAttributes attributes;
};
}; // namespace JustCause3::RenderBlocks
#pragma pack(pop)

class RenderBlockCharacterSkin : public IRenderBlock
{
  private:
    struct cbLocalConsts {
        glm::mat4   World;
        glm::mat4   WorldViewProjection;
        glm::vec4   Scale;
        glm::mat3x4 MatrixPalette[70];
    } m_cbLocalConsts;

    struct cbInstanceConsts {
        glm::vec4 MotionBlur = glm::vec4(0);
    } m_cbInstanceConsts;

    struct cbMaterialConsts {
        glm::vec4 EyeGloss = glm::vec4(0);
        glm::vec4 _unknown_;
        glm::vec4 _unknown[3];
    } m_cbMaterialConsts;

    JustCause3::RenderBlocks::CharacterSkin m_Block;
    ConstantBuffer_t*                       m_VertexShaderConstants   = nullptr;
    std::array<ConstantBuffer_t*, 2>        m_FragmentShaderConstants = {nullptr};
    int32_t                                 m_Stride                  = 0;

  public:
    RenderBlockCharacterSkin() = default;
    virtual ~RenderBlockCharacterSkin()
    {
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants);

        for (auto& fsc : m_FragmentShaderConstants)
            Renderer::Get()->DestroyBuffer(fsc);
    }

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockCharacterSkin";
    }

    virtual uint32_t GetTypeHash() const override final
    {
        return RenderBlockFactory::RB_CHARACTERSKIN;
    }

    virtual bool IsOpaque() override final
    {
        return true;
    }

    virtual void Create() override final
    {
        m_PixelShader = ShaderManager::Get()->GetPixelShader("characterskin");

        switch (m_Stride) {
                // 4bones1uv
            case 0: {
                m_VertexShader = ShaderManager::Get()->GetVertexShader("characterskin");

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
                                                                               "RenderBlockCharacterSkin (4bones1uv)");
                break;
            }

                // 4bones2uvs
            case 1: {
                m_VertexShader = ShaderManager::Get()->GetVertexShader("characterskin2uvs");

                // create the element input desc
                // clang-format off
                D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                    { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   1,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   4,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   6,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                };
                // clang-format on

                // create the vertex declaration
                m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 5, m_VertexShader.get(),
                                                                               "RenderBlockCharacterSkin (4bones2uvs)");
                break;
            }

                // 4bones3uvs
            case 2: {
                m_VertexShader = ShaderManager::Get()->GetVertexShader("characterskin3uvs");

                // create the element input desc
                // clang-format off
                D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                    { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   1,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   4,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   5,  DXGI_FORMAT_R16G16_SNORM,           0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   6,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                };
                // clang-format on

                // create the vertex declaration
                m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 6, m_VertexShader.get(),
                                                                               "RenderBlockCharacterSkin (4bones3uvs)");
                break;
            }

                // 8bones1uv
            case 3: {
                m_VertexShader = ShaderManager::Get()->GetVertexShader("characterskin8");

                // create the element input desc
                // clang-format off
                D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                    { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   1,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   2,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   3,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   4,  DXGI_FORMAT_R16G16_SNORM,           0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   6,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                };
                // clang-format on

                // create the vertex declaration
                m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 7, m_VertexShader.get(),
                                                                               "RenderBlockCharacterSkin (8bones1uv)");
                break;
            }

                // 8bones2uvs
            case 4: {
                m_VertexShader = ShaderManager::Get()->GetVertexShader("characterskin82uvs");

                // create the element input desc
                // clang-format off
                D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                    { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   1,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   2,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   3,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   4,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   6,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                };
                // clang-format on

                // create the vertex declaration
                m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 7, m_VertexShader.get(),
                                                                               "RenderBlockCharacterSkin (8bones3uvs)");
                break;
            }

                // 8bones3uvs
            case 5: {
                m_VertexShader = ShaderManager::Get()->GetVertexShader("characterskin83uvs");

                // create the element input desc
                // clang-format off
                D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                    { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   1,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   2,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   3,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   4,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   5,  DXGI_FORMAT_R16G16_SNORM,           0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   6,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                };
                // clang-format on

                // create the vertex declaration
                m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 8, m_VertexShader.get(),
                                                                               "RenderBlockCharacterSkin (8bones3uvs)");
                break;
            }
        }

        // create the constant buffer
        m_VertexShaderConstants =
            Renderer::Get()->CreateConstantBuffer(m_cbLocalConsts, "RenderBlockCharacterSkin cbLocalConsts");
        m_FragmentShaderConstants[0] =
            Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "RenderBlockCharacterSkin cbInstanceConsts");
        m_FragmentShaderConstants[1] =
            Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts, "RenderBlockCharacterSkin cbMaterialConsts");

        // identity the palette data
        for (int i = 0; i < 70; ++i) {
            m_cbLocalConsts.MatrixPalette[i] = glm::mat3x4(1);
        }

        //
        memset(&m_cbMaterialConsts._unknown, 0, sizeof(m_cbMaterialConsts._unknown));

        // create the sampler states
        {
            D3D11_SAMPLER_DESC params;
            ZeroMemory(&params, sizeof(params));
            params.Filter         = D3D11_FILTER_ANISOTROPIC;
            params.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
            params.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
            params.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
            params.MipLODBias     = 0.0f;
            params.MaxAnisotropy  = 8;
            params.ComparisonFunc = D3D11_COMPARISON_NEVER;
            params.MinLOD         = 0.0f;
            params.MaxLOD         = 13.0f;

            m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockCharacterSkin");
        }
    }

    virtual void Read(std::istream& stream) override final
    {
        // using namespace JustCause3::Vertex;
        using namespace JustCause3::Vertex::RenderBlockCharacter;

        // read the block header
        stream.read((char*)&m_Block, sizeof(m_Block));

        // read the materials
        ReadMaterials(stream);

        // get the vertices stride
        m_Stride = (3 * ((m_Block.attributes.flags >> 2) & 1) + ((m_Block.attributes.flags >> 1) & 1)
                    + ((m_Block.attributes.flags >> 4) & 1));

        // read vertex data
        ReadVertexBuffer(stream, &m_VertexBuffer, VertexStrides[m_Stride]);
        // InitVerticesForExporters(&m_VertexBuffer->m_Data, m_Stride, &m_Vertices, &m_UVs);

        // read skin batch
        ReadSkinBatch(stream);

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
            const auto  scale = m_Block.attributes.scale;
            static auto world = glm::mat4(1);

            // set vertex shader constants
            m_cbLocalConsts.World               = world;
            m_cbLocalConsts.WorldViewProjection = world * context->m_viewProjectionMatrix;
            m_cbLocalConsts.Scale               = glm::vec4(m_Block.attributes.scale * m_ScaleModifier);

            // set fragment shader constants
            //
        }

        // set the sampler states
        context->m_Renderer->SetSamplerState(m_SamplerState, 0);
        context->m_Renderer->SetSamplerState(m_SamplerState, 1);
        context->m_Renderer->SetSamplerState(m_SamplerState, 2);
        context->m_Renderer->SetSamplerState(m_SamplerState, 3);
        context->m_Renderer->SetSamplerState(m_SamplerState, 4);

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
        context->m_Renderer->SetBlendingFunc(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA,
                                             D3D11_BLEND_ONE);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;

        IRenderBlock::DrawSkinBatches(context);
    }

    virtual void DrawUI() override final
    {
        static std::array flag_labels = {"Disable Culling",
                                         "Use Wrinkle Map",
                                         "",
                                         "",
                                         "Use Feature",
                                         "Use Alpha Mask",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "",
                                         ""};

        ImGuiCustom::BitFieldTooltip("Flags", &m_Block.attributes.flags, flag_labels);

        ImGui::SliderFloat("Scale", &m_ScaleModifier, 0.0f, 20.0f);
    }

    /*
    void InitVerticesForExporters(std::vector<uint8_t>* data, int32_t stride, std::vector<float>* vertices,
    std::vector<float>* uvs)
    {
        switch (stride) {
        case 0: {
            //Packed4Bones1UV

            std::vector<Packed4Bones1UV> vertexdata(data->data(), data->data() + (data->size() /
    sizeof(Packed4Bones1UV))); for (const auto& vertex : vertexdata) { vertices->emplace_back(unpack(vertex.x));
                vertices->emplace_back(unpack(vertex.y));
                vertices->emplace_back(unpack(vertex.z));
                uvs->emplace_back(unpack(vertex.u0));
                uvs->emplace_back(unpack(vertex.v0));
            }

            break;
        }
        case 1: {
            //Packed4Bones2UVs
            break;
        }
        case 2: {
            //Packed4Bones3UVs
            break;
        }
        case 3: {
            //Packed8Bones1UV
            break;
        }
        case 4: {
            //Packed8Bones2UVs
            break;
        }
        case 5: {
            //Packed8Bones3UVs
            break;
        }
        }
    }
    */
};
