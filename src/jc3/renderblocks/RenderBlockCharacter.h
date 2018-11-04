#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct CharacterAttributes {
    uint32_t flags;
    float    scale;
    char     pad[0x14];
    float    specularGloss;
    float    transmission;
    float    specularFresnel;
    float    diffuseRoughness;
    float    diffuseWrap;
    float    _unknown;
    float    dirtFactor;
    float    emissive;
    char     pad2[0x30];
};

static_assert(sizeof(CharacterAttributes) == 0x6C, "CharacterAttributes alignment is wrong!");

namespace JustCause3::RenderBlocks
{
struct Character {
    uint8_t             version;
    CharacterAttributes attributes;
};
}; // namespace JustCause3::RenderBlocks
#pragma pack(pop)

class RenderBlockCharacter : public IRenderBlock
{
  private:
    enum {
        DISABLE_BACKFACE_CULLING   = 0x1,
        EIGHT_BONES                = 0x2,
        ALPHA_MASK                 = 0x4,
        TRANSPARENCY_ALPHABLENDING = 0x8,
        USE_FEATURE_MAP            = 0x10,
        USE_WRINKLE_MAP            = 0x20,
        USE_CAMERA_LIGHTING        = 0x40,
    };

    enum CharacterAttributeFlags {
        GEAR      = 0x0,
        EYES      = 0x1000,
        HAIR      = 0x2000,
        BODY_PART = 0x3000,
    };

    struct cbLocalConsts {
        glm::mat4   World;
        glm::mat4   WorldViewProjection;
        glm::vec4   Scale;
        glm::mat3x4 MatrixPalette[70];
    } m_cbLocalConsts;

    struct cbInstanceConsts {
        glm::vec4 _unknown      = glm::vec4(0);
        glm::vec4 DiffuseColour = glm::vec4(0, 0, 0, 1);
        glm::vec4 _unknown2     = glm::vec4(0); // .w is some kind of snow factor???
    } m_cbInstanceConsts;

    struct cbMaterialConsts {
        glm::vec4 unknown[10];
    } m_cbMaterialConsts;

    JustCause3::RenderBlocks::Character m_Block;
    ConstantBuffer_t*                   m_VertexShaderConstants   = nullptr;
    std::array<ConstantBuffer_t*, 2>    m_FragmentShaderConstants = {nullptr};
    int32_t                             m_Stride                  = 0;

  public:
    RenderBlockCharacter() = default;
    virtual ~RenderBlockCharacter()
    {
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants);

        for (auto& fsc : m_FragmentShaderConstants)
            Renderer::Get()->DestroyBuffer(fsc);
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
        const auto flags = m_Block.attributes.flags;
        return ((flags & BODY_PART) == GEAR || (flags & BODY_PART) == HAIR) && !(flags & TRANSPARENCY_ALPHABLENDING);
    }

    virtual void Create() override final
    {
        m_PixelShader = ShaderManager::Get()->GetPixelShader("character");

        switch (m_Stride) {
                // 4bones1uv
            case 0: {
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
                break;
            }

                // 4bones2uvs
            case 1: {
                m_VertexShader = ShaderManager::Get()->GetVertexShader("character2uvs");

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
                                                                               "RenderBlockCharacter (4bones2uvs)");
                break;
            }

                // 4bones3uvs
            case 2: {
                m_VertexShader = ShaderManager::Get()->GetVertexShader("character3uvs");

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
                                                                               "RenderBlockCharacter (4bones3uvs)");
                break;
            }

                // 8bones1uv
            case 3: {
                m_VertexShader = ShaderManager::Get()->GetVertexShader("character8");

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
                                                                               "RenderBlockCharacter (8bones1uv)");
                break;
            }

                // 8bones2uvs
            case 4: {
                m_VertexShader = ShaderManager::Get()->GetVertexShader("character82uvs");

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
                                                                               "RenderBlockCharacter (8bones3uvs)");
                break;
            }

                // 8bones3uvs
            case 5: {
                m_VertexShader = ShaderManager::Get()->GetVertexShader("character83uvs");

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
                                                                               "RenderBlockCharacter (8bones3uvs)");
                break;
            }
        }

        // create the constant buffer
        m_VertexShaderConstants =
            Renderer::Get()->CreateConstantBuffer(m_cbLocalConsts, "RenderBlockCharacter cbLocalConsts");
        m_FragmentShaderConstants[0] =
            Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "RenderBlockCharacter cbInstanceConsts");
        m_FragmentShaderConstants[1] =
            Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts, "RenderBlockCharacter cbMaterialConsts");

        // identity the palette data
        for (int i = 0; i < 70; ++i) {
            m_cbLocalConsts.MatrixPalette[i] = glm::mat3x4(1);
        }

        // reset fragment shader material consts
        memset(&m_cbMaterialConsts, 0, sizeof(m_cbMaterialConsts));

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

            m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockCharacter");
        }
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace JustCause3::Vertex;
        using namespace JustCause3::Vertex::RenderBlockCharacter;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));

        // read the materials
        ReadMaterials(stream);

        // get the vertices stride
        const auto flags = m_Block.attributes.flags;
        m_Stride         = (3 * (flags & EIGHT_BONES) + (flags & USE_WRINKLE_MAP) + (flags & USE_FEATURE_MAP));

        // read vertex data
        m_VertexBuffer = ReadBuffer(stream, VERTEX_BUFFER, VertexStrides[m_Stride]);

        // read skin batches
        ReadSkinBatch(stream);

        // read index buffer
        m_IndexBuffer = ReadIndexBuffer(stream);
    }

    virtual void Write(std::ostream& stream) override final
    {
        // write the block attributes
        stream.write((char*)&m_Block, sizeof(m_Block));

        // write the materials
        WriteMaterials(stream);

        // write the vertex data
        WriteBuffer(stream, m_VertexBuffer);

        // write skin batches
        WriteSkinBatch(stream);

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
        using namespace JustCause3::Vertex::RenderBlockCharacter;

        floats_t  vertices;
        uint16s_t indices = m_IndexBuffer->CastData<uint16_t>();
        floats_t  uvs;

        switch (m_Stride) {
            // 4bones1uv, 4bones2uvs, 4bones3uvs
            case 0:
            case 1:
            case 2: {
                // TODO: once multiple UVs are supported, change this!
                const auto& vb = m_VertexBuffer->CastData<Packed4Bones1UV>();
                for (const auto& vertex : vb) {
                    vertices.emplace_back(unpack(vertex.x));
                    vertices.emplace_back(unpack(vertex.y));
                    vertices.emplace_back(unpack(vertex.z));
                    uvs.emplace_back(unpack(vertex.u0));
                    uvs.emplace_back(unpack(vertex.v0));
                }

                break;
            }

            // 8bones1uv, 8bones2uvs, 8bones3uvs
            case 3:
            case 4:
            case 5: {
                // TODO: once multiple UVs are supported, change this!
                const auto& vb = m_VertexBuffer->CastData<Packed8Bones1UV>();
                for (const auto& vertex : vb) {
                    vertices.emplace_back(unpack(vertex.x));
                    vertices.emplace_back(unpack(vertex.y));
                    vertices.emplace_back(unpack(vertex.z));
                    uvs.emplace_back(unpack(vertex.u0));
                    uvs.emplace_back(unpack(vertex.v0));
                }

                break;
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
        context->m_Renderer->SetSamplerState(m_SamplerState, 9);

        // set the constant buffers
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants, 1, m_cbLocalConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[0], 1, m_cbInstanceConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[1], 2, m_cbMaterialConsts);

        // set the culling mode
        context->m_Renderer->SetCullMode((!(m_Block.attributes.flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK
                                                                                                  : D3D11_CULL_NONE);

        // toggle alpha mask
        if (m_Block.attributes.flags & ALPHA_MASK) {
            context->m_Renderer->SetAlphaEnabled(true);
            context->m_Renderer->SetBlendingEnabled(false);
        } else {
            context->m_Renderer->SetAlphaEnabled(false);
        }

        // setup blending
        switch (m_Block.attributes.flags & BODY_PART) {
            case GEAR: {
                if (m_Block.attributes.flags & TRANSPARENCY_ALPHABLENDING) {
                    context->m_Renderer->SetBlendingEnabled(true);
                    context->m_Renderer->SetBlendingFunc(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE,
                                                         D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_ONE);
                } else {
                    context->m_Renderer->SetBlendingEnabled(false);
                }
                break;
            }

            case EYES: {
                context->m_Renderer->SetBlendingEnabled(true);
                context->m_Renderer->SetBlendingFunc(D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_SRC_ALPHA,
                                                     D3D11_BLEND_ONE);
                break;
            }

            case HAIR: {
                if (m_Block.attributes.flags & TRANSPARENCY_ALPHABLENDING) {
                    context->m_Renderer->SetBlendingEnabled(true);
                    context->m_Renderer->SetBlendingFunc(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE,
                                                         D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_ONE);
                } else {
                    context->m_Renderer->SetBlendingEnabled(false);
                }
                break;
            }
        }
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;

        DrawSkinBatches(context);
    }

    virtual void DrawUI() override final
    {
        // clang-format off
        static std::array flag_labels = {
            "Disable Backface Culling",     "Eight Bones",                  "Use Alpha Mask",               "Transparency Alpha Blending",
            "Use Feature Map",              "Use Wrinkle Map",              "Use Camera Lighting",          "",
        };
        // clang-format on

        ImGuiCustom::BitFieldTooltip("Flags", &m_Block.attributes.flags, flag_labels);

        ImGui::SliderFloat("Scale", &m_ScaleModifier, 0.0f, 20.0f);

        ImGui::SliderFloat4("Unknown #1", glm::value_ptr(m_cbInstanceConsts._unknown), 0, 1);
        ImGui::ColorEdit4("Diffuse Colour", glm::value_ptr(m_cbInstanceConsts.DiffuseColour));
        ImGui::SliderFloat4("Unknown #2", glm::value_ptr(m_cbInstanceConsts._unknown2), 0, 1);
    }
};
