#pragma once

#include "irenderblock.h"

extern bool g_IsJC4Mode;

#pragma pack(push, 1)
struct CharacterAttributes {
    uint32_t  flags;
    float     scale;
    glm::vec2 _unknown;
    glm::vec2 _unknown2;
    float     _unknown3;
    float     specularGloss;
    float     transmission;
    float     specularFresnel;
    float     diffuseRoughness;
    float     diffuseWrap;
    float     _unknown4;
    float     dirtFactor;
    float     emissive;
    char      pad2[0x30];
};

static_assert(sizeof(CharacterAttributes) == 0x6C, "CharacterAttributes alignment is wrong!");

namespace jc::RenderBlocks
{
static constexpr uint8_t CHARACTER_VERSION = 9;

struct Character {
    uint8_t             version;
    CharacterAttributes attributes;
};
}; // namespace jc::RenderBlocks
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
        USE_EYE_REFLECTION         = 0x80,
    };

    enum CharacterAttributeFlags {
        GEAR      = 0x0,
        EYES      = 0x1000,
        HAIR      = 0x2000,
        BODY_PART = 0x3000,
    };

    struct jc3_cbLocalConsts {
        glm::mat4   World;
        glm::mat4   WorldViewProjection;
        glm::vec4   Scale;
        glm::mat3x4 MatrixPalette[70];
    } m_jc3_cbLocalConsts;

    struct jc3_cbInstanceConsts {
        glm::vec4 _unknown      = glm::vec4(0);
        glm::vec4 DiffuseColour = glm::vec4(0, 0, 0, 1);
        glm::vec4 _unknown2     = glm::vec4(0); // .w is some kind of snow factor???
    } m_jc3_cbInstanceConsts;

    struct jc3_cbMaterialConsts {
        glm::vec4 unknown[10];
    } m_jc3_cbMaterialConsts;

    struct jc4_cbLocalConsts {
        glm::mat4 WorldViewProjection;
        glm::mat4 World;
    } m_jc4_cbLocalConsts;

    struct jc4_cbSkinningConsts {
        glm::mat3x4 MatrixPalette[70];
    } m_jc4_cbSkinningConsts;

    struct jc4_cbUnknownConsts {
        glm::vec2 TilingUV = glm::vec2(1, 1);
        glm::vec2 Unknown;
        glm::vec4 Scale = glm::vec4(1, 1, 1, 1);
    } m_jc4_cbUnknownConsts;

    jc::RenderBlocks::Character      m_Block;
    std::vector<jc::CSkinBatch>      m_SkinBatches;
    std::array<ConstantBuffer_t*, 3> m_VertexShaderConstants   = {nullptr};
    std::array<ConstantBuffer_t*, 2> m_FragmentShaderConstants = {nullptr};
    int32_t                          m_Stride                  = 0;

    // ConstantBuffer_t*                m_VertexShaderConstants     = nullptr;

  public:
    RenderBlockCharacter() = default;
    virtual ~RenderBlockCharacter()
    {
        // delete the skin batch lookup
        for (auto& batch : m_SkinBatches) {
            SAFE_DELETE(batch.m_BatchLookup);
        }

        // destroy shader constants
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[0]);
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[1]);
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[2]);
        Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants[0]);
        Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants[1]);
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

    void CreateBuffers(FileBuffer* vertex, FileBuffer* index)
    {
        m_VertexBuffer = Renderer::Get()->CreateBuffer(
            vertex->data(), (vertex->size() / sizeof(jc::Vertex::RenderBlockCharacter::Packed4Bones1UV)),
            sizeof(jc::Vertex::RenderBlockCharacter::Packed4Bones1UV), VERTEX_BUFFER);

        m_IndexBuffer = Renderer::Get()->CreateBuffer(index->data(), (index->size() / sizeof(uint16_t)),
                                                      sizeof(uint16_t), INDEX_BUFFER);

        m_Block                  = {};
        m_Block.attributes.scale = 1.0f;

        Create();

        /*uint32_t stride_1 = sizeof(jc::Vertex::PackedVertexPosition);
        uint32_t stride_2 = sizeof(jc::Vertex::GeneralShortPacked);

        m_VertexBuffer =
            Renderer::Get()->CreateBuffer(vertex->data(), vertex->size() / stride_1, stride_1, VERTEX_BUFFER);
        m_VertexBufferData =
            Renderer::Get()->CreateBuffer(data->data(), data->size() / stride_2, stride_2, VERTEX_BUFFER);
        m_IndexBuffer = Renderer::Get()->CreateBuffer(index->data(), index->size() / sizeof(uint16_t), sizeof(uint16_t),
                                                      INDEX_BUFFER);

        m_Block                             = {};
        m_Block.attributes.packed.format    = 1;
        m_Block.attributes.packed.scale     = 1.0f;
        m_Block.attributes.packed.uv0Extent = {1.0f, 1.0f};

        Create();*/
    }

    virtual void Create() override final
    {
        m_PixelShader = ShaderManager::Get()->GetPixelShader(
            (m_Block.attributes.flags & BODY_PART) != HAIR ? "character" : "characterhair_msk");

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
        if (g_IsJC4Mode) {
            m_VertexShaderConstants[0] =
                Renderer::Get()->CreateConstantBuffer(m_jc4_cbLocalConsts, "RenderBlockCharacter jc4_cbLocalConsts");
            m_VertexShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(
                m_jc4_cbSkinningConsts, "RenderBlockCharacter jc4_cbSkinningConsts");
            m_VertexShaderConstants[2] = Renderer::Get()->CreateConstantBuffer(
                m_jc4_cbUnknownConsts, "RenderBlockCharacter jc4_cbUnknownConsts");

            // TODO: fragment shader constants
        } else {
            m_VertexShaderConstants[0] =
                Renderer::Get()->CreateConstantBuffer(m_jc3_cbLocalConsts, "RenderBlockCharacter jc3_cbLocalConsts");
            m_FragmentShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(
                m_jc3_cbInstanceConsts, "RenderBlockCharacter jc3_cbInstanceConsts");
            m_FragmentShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(
                m_jc3_cbMaterialConsts, "RenderBlockCharacter jc3_cbMaterialConsts");

            // reset fragment shader material consts
            memset(&m_jc3_cbMaterialConsts, 0, sizeof(m_jc3_cbMaterialConsts));
        }

        // identity the palette data
        for (int i = 0; i < 70; ++i) {
            if (g_IsJC4Mode) {
                m_jc4_cbSkinningConsts.MatrixPalette[i] = glm::mat3x4(1);
            } else {
                m_jc3_cbLocalConsts.MatrixPalette[i] = glm::mat3x4(1);
            }
        }

        // create the sampler states
        {
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

            m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockCharacter");
        }
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace jc::Vertex;
        using namespace jc::Vertex::RenderBlockCharacter;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));

        if (m_Block.version != jc::RenderBlocks::CHARACTER_VERSION) {
            __debugbreak();
        }

        // read the materials
        ReadMaterials(stream);

        // get the vertices stride
        const auto flags = m_Block.attributes.flags;
        m_Stride         = (3 * ((flags >> 1) & 1) + ((flags >> 4) & 1) + ((flags >> 5) & 1));

        // read vertex data
        m_VertexBuffer = ReadBuffer(stream, VERTEX_BUFFER, VertexStrides[m_Stride]);

        // read skin batches
        ReadSkinBatch(stream, &m_SkinBatches);

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
        WriteSkinBatch(stream, &m_SkinBatches);

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
        using namespace jc::Vertex::RenderBlockCharacter;

        vertices_t vertices;
        uint16s_t  indices = m_IndexBuffer->CastData<uint16_t>();

        switch (m_Stride) {
            // 4bones1uv, 4bones2uvs, 4bones3uvs
            case 0:
            case 1:
            case 2: {
                // TODO: once multiple UVs are supported, change this!
                const auto& vb = m_VertexBuffer->CastData<Packed4Bones1UV>();
                vertices.reserve(vb.size());
                for (const auto& vertex : vb) {
                    vertex_t v{};
                    v.pos = glm::vec3{unpack(vertex.x), unpack(vertex.y), unpack(vertex.z)};
                    v.uv  = glm::vec2{unpack(vertex.u0), unpack(vertex.v0)};
                    vertices.emplace_back(std::move(v));
                }

                break;
            }

            // 8bones1uv, 8bones2uvs, 8bones3uvs
            case 3:
            case 4:
            case 5: {
                // TODO: once multiple UVs are supported, change this!
                const auto& vb = m_VertexBuffer->CastData<Packed8Bones1UV>();
                vertices.reserve(vb.size());
                for (const auto& vertex : vb) {
                    vertex_t v{};
                    v.pos = glm::vec3{unpack(vertex.x), unpack(vertex.y), unpack(vertex.z)};
                    v.uv  = glm::vec2{unpack(vertex.u0), unpack(vertex.v0)};
                    vertices.emplace_back(std::move(v));
                }

                break;
            }
        }

        return {vertices, indices};
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;

        IRenderBlock::Setup(context);

        const auto flags = m_Block.attributes.flags;

        // setup the constant buffer
        static auto world = glm::mat4(1);

        // set the textures
        IRenderBlock::BindTexture(0, m_SamplerState);

        if (g_IsJC4Mode) {
            // local consts
            m_jc4_cbLocalConsts.WorldViewProjection = world * context->m_viewProjectionMatrix;
            m_jc4_cbLocalConsts.World               = world;
            context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 2, m_jc4_cbLocalConsts);

            // skinning consts
            context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 3, m_jc4_cbSkinningConsts);

            // unknown consts
            m_jc4_cbUnknownConsts.Scale = glm::vec4(1 * m_ScaleModifier);
            context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[2], 4, m_jc4_cbUnknownConsts);
        } else {
            // set vertex shader constants
            m_jc3_cbLocalConsts.World               = world;
            m_jc3_cbLocalConsts.WorldViewProjection = world * context->m_viewProjectionMatrix;
            m_jc3_cbLocalConsts.Scale               = glm::vec4(m_Block.attributes.scale * m_ScaleModifier);
            context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 1, m_jc3_cbLocalConsts);

            // set fragment shader constants
            context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[0], 1, m_jc3_cbInstanceConsts);
            context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[1], 2, m_jc3_cbMaterialConsts);
        }

        // set the culling mode
        context->m_Renderer->SetCullMode((!(flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);

        // toggle alpha mask
        if (flags & ALPHA_MASK) {
            context->m_Renderer->SetAlphaTestEnabled(true);
            context->m_Renderer->SetBlendingEnabled(false);
        } else {
            context->m_Renderer->SetAlphaTestEnabled(false);
        }

        // setup textures and blending
        switch (flags & BODY_PART) {
            case GEAR: {
                IRenderBlock::BindTexture(1, m_SamplerState);
                IRenderBlock::BindTexture(2, m_SamplerState);
                IRenderBlock::BindTexture(3, m_SamplerState);
                IRenderBlock::BindTexture(4, m_SamplerState);
                IRenderBlock::BindTexture(9, m_SamplerState);

                if (flags & USE_FEATURE_MAP) {
                    IRenderBlock::BindTexture(5, m_SamplerState);
                }

                if (flags & USE_WRINKLE_MAP) {
                    IRenderBlock::BindTexture(7, 6, m_SamplerState);
                }

                if (flags & USE_CAMERA_LIGHTING) {
                    IRenderBlock::BindTexture(8, m_SamplerState);
                }

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
                if (m_Block.attributes.flags & USE_EYE_REFLECTION) {
                    IRenderBlock::BindTexture(10, 11, m_SamplerState);
                }

                context->m_Renderer->SetBlendingEnabled(true);
                context->m_Renderer->SetBlendingFunc(D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_SRC_ALPHA,
                                                     D3D11_BLEND_ONE);
                break;
            }

            case HAIR: {
                IRenderBlock::BindTexture(1, m_SamplerState);
                IRenderBlock::BindTexture(2, m_SamplerState);

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

        if (m_SkinBatches.size() > 0) {
            IRenderBlock::DrawSkinBatches(context, m_SkinBatches);
        } else {
            IRenderBlock::Draw(context);
        }
    }

    virtual void DrawContextMenu() override final
    {
        // clang-format off
        static std::array flag_labels = {
            "Disable Backface Culling",     "Eight Bones",                  "Use Alpha Mask",               "Transparency Alpha Blending",
            "Use Feature Map",              "Use Wrinkle Map",              "Use Camera Lighting",          "Use Eye Reflection",
        };
        // clang-format on

        ImGuiCustom::DropDownFlags(m_Block.attributes.flags, flag_labels);
    }

    virtual void DrawUI() override final
    {
        ImGui::Text(ICON_FA_COGS "  Attributes");
        ImGui::SliderFloat("Scale", &m_ScaleModifier, 0.0f, 20.0f);

        if (g_IsJC4Mode) {
            /*ImGui::DragFloat4("Unk", glm::value_ptr(m_cbJC4Unknown.unk));
            ImGui::DragFloat4("Unk2", glm::value_ptr(m_cbJC4Unknown.unk2));*/
        } else {
            /*if (ImGui::RadioButton("Gear", ((m_Block.attributes.flags & BODY_PART) == GEAR))) {}

            ImGui::SameLine();

            if (ImGui::RadioButton("Eyes", ((m_Block.attributes.flags & BODY_PART) == EYES))) {}

            ImGui::SameLine();

            if (ImGui::RadioButton("Hair", ((m_Block.attributes.flags & BODY_PART) == HAIR))) {}*/

            ImGui::SliderFloat4("Unknown #1", glm::value_ptr(m_jc3_cbInstanceConsts._unknown), 0, 1);
            ImGui::ColorEdit4("Diffuse Colour", glm::value_ptr(m_jc3_cbInstanceConsts.DiffuseColour));
            ImGui::SliderFloat4("Unknown #2", glm::value_ptr(m_jc3_cbInstanceConsts._unknown2), 0, 1);
        }

        // Textures
        ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
        ImGui::Columns(3, nullptr, false);
        {
            const auto flags = m_Block.attributes.flags;

            IRenderBlock::DrawTexture("DiffuseMap", 0);

            if ((flags & BODY_PART) == GEAR || (flags & BODY_PART) == HAIR) {
                IRenderBlock::DrawTexture("NormalMap", 1);
                IRenderBlock::DrawTexture("PropertiesMap", 2);
            }

            if ((flags & BODY_PART) == GEAR) {
                IRenderBlock::DrawTexture("DetailDiffuseMap", 3);
                IRenderBlock::DrawTexture("DetailNormalMap", 4);

                if (m_Block.attributes.flags & USE_FEATURE_MAP) {
                    IRenderBlock::DrawTexture("FeatureMap", 5);
                }

                if (m_Block.attributes.flags & USE_WRINKLE_MAP) {
                    IRenderBlock::DrawTexture("WrinkleMap", 7);
                }

                if (m_Block.attributes.flags & USE_CAMERA_LIGHTING) {
                    IRenderBlock::DrawTexture("CameraMap", 8);
                }

                IRenderBlock::DrawTexture("MetallicMap", 9);
            } else if ((flags & BODY_PART) == EYES && flags & USE_EYE_REFLECTION) {
                IRenderBlock::DrawTexture("ReflectionMap", 10);
            }
        }
        ImGui::EndColumns();
    }
};
