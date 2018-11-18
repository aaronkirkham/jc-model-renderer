#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct GeneralMkIIIAttributes {
    float                                depthBias;
    char                                 _pad[0x8];
    float                                emissiveTODScale;
    float                                emissiveStartFadeDistSq;
    char                                 _pad2[0x10];
    JustCause3::Vertex::SPackedAttribute packed;
    char                                 _pad3[0x4];
    uint32_t                             flags;
    char                                 _pad4[0x1C];
};

static_assert(sizeof(GeneralMkIIIAttributes) == 0x68, "GeneralMkIIIAttributes alignment is wrong!");

namespace JustCause3::RenderBlocks
{
static constexpr uint8_t GENERALMKIII_VERSION = 5;

struct GeneralMkIII {
    uint8_t                version;
    GeneralMkIIIAttributes attributes;
};
}; // namespace JustCause3::RenderBlocks
#pragma pack(pop)

class RenderBlockGeneralMkIII : public IRenderBlock
{
  private:
    enum {
        DISABLE_BACKFACE_CULLING   = 0x1,
        TRANSPARENCY_ALPHABLENDING = 0x2,
        TRANSPARENCY_ALPHATESTING  = 0x4,
        DYNAMIC_EMISSIVE           = 0x8,
        IS_SKINNED                 = 0x20,
        USE_LAYERED                = 0x80,
        USE_OVERLAY                = 0x100,
        USE_DECAL                  = 0x200,
        USE_DAMAGE                 = 0x400,
        ANISOTROPIC_FILTERING      = 0x4000,
        DESTRUCTION                = 0x8000,
    };

    struct RBIInfo {
        glm::mat4 ModelWorldMatrix;
        glm::mat4 ModelWorldMatrixPrev; // [unused]
        glm::vec4 ModelDiffuseColor;
        glm::vec4 ModelAmbientColor;  // [unused]
        glm::vec4 ModelSpecularColor; // [unused]
        glm::vec4 ModelEmissiveColor;
        glm::vec4 ModelDebugColor; // [unused]
    } m_cbRBIInfo;

    struct InstanceAttributes {
        glm::vec4 UVScale;
        glm::vec4 RippleAngle;    // [unused]
        glm::vec4 FlashDirection; // [unused]
        float     DepthBias;
        float     QuantizationScale;
        float     EmissiveStartFadeDistSq;
        float     EmissiveTODScale;
        float     RippleSpeed;      // [unused]
        float     RippleMagnitude;  // [unused]
        float     PixelScale;       // [unused]
        float     OutlineThickness; // [unused]
    } m_cbInstanceAttributes;

    struct cbSkinningConsts {
        glm::vec4 MatrixPalette[2];
    } m_cbSkinningConsts;

    struct MaterialConsts {
        glm::vec4 DebugColor; // [unused]
        float     EnableVariableDialecticSpecFresnel;
        float     UseMetallicVer2;
    } m_cbMaterialConsts;

    struct MaterialConsts2 {
        float NormalStrength; // [unused]
        float Reflectivity_1; // [unused]
        float Roughness_1;
        float DiffuseWrap_1;
        float Emissive_1;
        float Transmission_1;
        float ClearCoat_1;
        float Roughness_2;               // [unused]
        float DiffuseWrap_2;             // [unused]
        float Emissive_2;                // [unused]
        float Transmission_2;            // [unused]
        float Reflectivity_2;            // [unused]
        float ClearCoat_2;               // [unused]
        float Roughness_3;               // [unused]
        float DiffuseWrap_3;             // [unused]
        float Emissive_3;                // [unused]
        float Transmission_3;            // [unused]
        float Reflectivity_3;            // [unused]
        float ClearCoat_3;               // [unused]
        float Roughness_4;               // [unused]
        float DiffuseWrap_4;             // [unused]
        float Emissive_4;                // [unused]
        float Transmission_4;            // [unused]
        float Reflectivity_4;            // [unused]
        float ClearCoat_4;               // [unused]
        float LayeredHeightMapUVScale;   // [unused]
        float LayeredUVScale;            // [unused]
        float LayeredHeight1Influence;   // [unused]
        float LayeredHeight2Influence;   // [unused]
        float LayeredHeightMapInfluence; // [unused]
        float LayeredMaskInfluence;      // [unused]
        float LayeredShift;              // [unused]
        float LayeredRoughness;          // [unused]
        float LayeredDiffuseWrap;        // [unused]
        float LayeredEmissive;           // [unused]
        float LayeredTransmission;       // [unused]
        float LayeredReflectivity;       // [unused]
        float LayeredClearCoat;          // [unused]
        float DecalBlend;                // [unused]
        float DecalBlendNormal;          // [unused]
        float DecalReflectivity;         // [unused]
        float DecalRoughness;            // [unused]
        float DecalDiffuseWrap;          // [unused]
        float DecalEmissive;             // [unused]
        float DecalTransmission;         // [unused]
        float DecalClearCoat;            // [unused]
        float OverlayHeightInfluence;    // [unused]
        float OverlayHeightMapInfluence; // [unused]
        float OverlayMaskInfluence;      // [unused]
        float OverlayShift;              // [unused]
        float OverlayColorR;             // [unused]
        float OverlayColorG;             // [unused]
        float OverlayColorB;             // [unused]
        float OverlayBrightness;         // [unused]
        float OverlayGloss;              // [unused]
        float OverlayMetallic;           // [unused]
        float OverlayReflectivity;       // [unused]
        float OverlayRoughness;          // [unused]
        float OverlayDiffuseWrap;        // [unused]
        float OverlayEmissive;           // [unused]
        float OverlayTransmission;       // [unused]
        float OverlayClearCoat;          // [unused]
        float DamageReflectivity;        // [unused]
        float DamageRoughness;           // [unused]
        float DamageDiffuseWrap;         // [unused]
        float DamageEmissive;            // [unused]
        float DamageTransmission;        // [unused]
        float DamageHeightInfluence;     // [unused]
        float DamageMaskInfluence;       // [unused]
        float DamageClearCoat;           // [unused]
    } m_cbMaterialConsts2;

    JustCause3::RenderBlocks::GeneralMkIII m_Block;
    std::vector<JustCause3::CSkinBatch>    m_SkinBatches;
    VertexBuffer_t*                        m_VertexBufferData        = nullptr;
    std::string                            m_ShaderName              = "generalmkiii";
    std::array<ConstantBuffer_t*, 3>       m_VertexShaderConstants   = {nullptr};
    std::array<ConstantBuffer_t*, 2>       m_FragmentShaderConstants = {nullptr};

  public:
    RenderBlockGeneralMkIII() = default;
    virtual ~RenderBlockGeneralMkIII()
    {
        // delete the skin batch lookup
        for (auto& batch : m_SkinBatches) {
            SAFE_DELETE(batch.m_BatchLookup);
        }

        // destroy the vertex buffer data
        Renderer::Get()->DestroyBuffer(m_VertexBufferData);

        // destroy shader constants
        for (auto& vsc : m_VertexShaderConstants) {
            Renderer::Get()->DestroyBuffer(vsc);
        }

        for (auto& fsc : m_FragmentShaderConstants) {
            Renderer::Get()->DestroyBuffer(fsc);
        }
    }

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockGeneralMkIII";
    }

    virtual uint32_t GetTypeHash() const override final
    {
        return RenderBlockFactory::RB_GENERALMKIII;
    }

    virtual bool IsOpaque() override final
    {
        return ~m_Block.attributes.flags & TRANSPARENCY_ALPHABLENDING;
    }

    virtual void Create() override final
    {
        // reset constants
        memset(&m_cbInstanceAttributes, 0, sizeof(m_cbInstanceAttributes));
        memset(&m_cbMaterialConsts, 0, sizeof(m_cbMaterialConsts));

        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader(m_ShaderName);
        m_PixelShader  = ShaderManager::Get()->GetPixelShader("generalmkiii");

#if 0
        if (m_Block.attributes.flags & 0x8020) {
            if (m_Block.attributes.flags & 0x20) {
                __debugbreak();
            }
            else {
                // create the element input desc
                D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                    { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   1,  DXGI_FORMAT_R8G8B8A8_UINT,          0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   2,  DXGI_FORMAT_R16G16B16A16_SNORM,     1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   3,  DXGI_FORMAT_R32G32B32_FLOAT,        1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                };

                // create the vertex declaration
                m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 5, m_VertexShader.get(), "RenderBlockGeneralMkIII (packed, skinned)");
            }
        }
        else {
#endif
        // clang-format off
        D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
            { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   2,  DXGI_FORMAT_R16G16B16A16_SNORM,     1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            { "TEXCOORD",   3,  DXGI_FORMAT_R32G32B32_FLOAT,        1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        };
        // clang-format on

        // create the vertex declaration
        m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(),
                                                                       "RenderBlockGeneralMkIII (packed)");

        // create the constant buffers
        m_VertexShaderConstants[0] =
            Renderer::Get()->CreateConstantBuffer(m_cbRBIInfo, "RenderBlockGeneralMkIII RBIInfo");
        m_VertexShaderConstants[1] =
            Renderer::Get()->CreateConstantBuffer(m_cbInstanceAttributes, "RenderBlockGeneralMkIII InstanceAttributes");
        m_FragmentShaderConstants[0] =
            Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts, 2, "RenderBlockGeneralMkIII MaterialConsts");
        m_FragmentShaderConstants[1] =
            Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts2, 18, "RenderBlockGeneralMkIII MaterialConsts2");

        // create skinning palette buffer
        if (m_Block.attributes.flags & (IS_SKINNED | DESTRUCTION)) {
            m_VertexShaderConstants[2] =
                Renderer::Get()->CreateConstantBuffer(m_cbSkinningConsts, "RenderBlockGeneralMkIII SkinningConsts");

            // identity the palette data
            for (int i = 0; i < 2; ++i) {
                m_cbSkinningConsts.MatrixPalette[i] = glm::vec4(1);
            }
        }

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

            m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockGeneralMkIII");
        }
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace JustCause3::Vertex;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));

        if (m_Block.version != JustCause3::RenderBlocks::GENERALMKIII_VERSION) {
            __debugbreak();
        }

        // read constant buffer data
        stream.read((char*)&m_cbMaterialConsts2, sizeof(m_cbMaterialConsts2));

        // read the materials
        ReadMaterials(stream);

        // models/jc_environments/structures/military/shared/textures/metal_grating_001_alpha_dif.ddsc
        if (m_Textures[0] && m_Textures[0]->GetHash() == 0x7D1AF10D) {
            m_Block.attributes.flags |= ANISOTROPIC_FILTERING;
        }

        // read the vertex buffer
        if (m_Block.attributes.flags & IS_SKINNED) {
            m_VertexBuffer = ReadVertexBuffer<UnpackedVertexPositionXYZW>(stream);
        } else {
            m_VertexBuffer = ReadVertexBuffer<PackedVertexPosition>(stream);
        }

        // read the vertex buffer data
        m_VertexBufferData = ReadVertexBuffer<GeneralShortPacked>(stream);

        // read skin batches
        if (m_Block.attributes.flags & (IS_SKINNED | DESTRUCTION)) {
            ReadSkinBatch(stream, &m_SkinBatches);
        }

        // read index buffer
        m_IndexBuffer = ReadIndexBuffer(stream);
    }

    virtual void Write(std::ostream& stream) override final
    {
        // write the block attributes
        stream.write((char*)&m_Block, sizeof(m_Block));

        // write constant buffer data
        stream.write((char*)&m_cbMaterialConsts2, sizeof(m_cbMaterialConsts2));

        // write the materials
        WriteMaterials(stream);

        // write the vertex buffer
        WriteBuffer(stream, m_VertexBuffer);
        WriteBuffer(stream, m_VertexBufferData);

        // write the skin batches
        if (m_Block.attributes.flags & (IS_SKINNED | DESTRUCTION)) {
            WriteSkinBatch(stream, &m_SkinBatches);
        }

        // write the index buffer
        WriteBuffer(stream, m_IndexBuffer);
    }

#if 0
    virtual void SetData(floats_t* vertices, uint16s_t* indices, floats_t* uvs) override final
    {
        using namespace JustCause3::Vertex;

        // clang-format off

        memset(&m_Block.attributes, 0, sizeof(m_Block.attributes));
        memset(&m_cbMaterialConsts2, 0, sizeof(m_cbMaterialConsts2));

        m_Block.attributes.packed.scale = 1.f;
        m_Block.attributes.packed.uv0Extent = { 1.f, 1.f };
        m_Block.attributes.emissiveStartFadeDistSq = 2000.f;

        // test
        m_MaterialParams[0] = 1.0f;
        m_MaterialParams[1] = 1.0f;
        m_MaterialParams[2] = 1.0f;
        m_MaterialParams[3] = 1.0f;
  
        // vertices
        std::vector<glm::vec3> _vertices;
        {
            static const char* _vertices_strings[] = {
                "v 0.500000 -0.500000 0.500000",
                "v 0.500000 -0.500000 -0.500000",
                "v 0.500000 0.500000 0.500000",
                "v 0.500000 0.500000 -0.500000",
                "v -0.500000 0.500000 0.500000",
                "v -0.500000 0.500000 -0.500000",
                "v -0.500000 -0.500000 0.500000",
                "v -0.500000 -0.500000 -0.500000",
            };

            for (int i = 0; i < ARRAYSIZE(_vertices_strings); ++i) {
                float x, y, z;
                sscanf(_vertices_strings[i], "v %f %f %f", &x, &y, &z);

                _vertices.emplace_back(glm::vec3{ x, y, z });
            }
        }

        // indices
        std::vector<uint16_t> vertex_indices;
        std::vector<uint16_t> uv_indices;
        {
            static const char* faces[] = {
                "f 1/1/1 2/2/1 3/3/1",
                "f 3/3/1 2/2/1 4/4/1",

                "f 3/1/2 4/2/2 5/3/2",
                "f 5/3/2 4/2/2 6/4/2",

                "f 5/4/3 6/3/3 7/2/3",
                "f 7/2/3 6/3/3 8/1/3",

                "f 7/1/4 8/2/4 1/3/4",
                "f 1/3/4 8/2/4 2/4/4",

                "f 2/1/5 8/2/5 4/3/5",
                "f 4/3/5 8/2/5 6/4/5",

                "f 7/1/6 1/2/6 5/3/6",
                "f 5/3/6 1/2/6 3/4/6",
            };

            // 36

            for (int i = 0; i < ARRAYSIZE(faces); ++i) {
                int v_index[3], uv_index[3], n_index[3];

                sscanf(faces[i], "f %d/%d/%d %d/%d/%d %d/%d/%d",
                    &v_index[0], &uv_index[0], &n_index[0],
                    &v_index[1], &uv_index[1], &n_index[1],
                    &v_index[2], &uv_index[2], &n_index[2]);

                // vertex indices
                vertex_indices.emplace_back(v_index[0] - 1);
                vertex_indices.emplace_back(v_index[1] - 1);
                vertex_indices.emplace_back(v_index[2] - 1);

                // uv indices
                uv_indices.emplace_back(uv_index[0] - 1);
                uv_indices.emplace_back(uv_index[1] - 1);
                uv_indices.emplace_back(uv_index[2] - 1);
            }
        }

        // uvs
        std::vector<glm::vec2> _uvs;
        {
            static const char* _uv_strings[] = {
                "vt 0.000000 0.000000",
                "vt 1.000000 0.000000",
                "vt 0.000000 1.000000",
                "vt 1.000000 1.000000",
            };

            for (int i = 0; i < ARRAYSIZE(_uv_strings); ++i) {
                float x, y;
                sscanf(_uv_strings[i], "vt %f %f", &x, &y);

                _uvs.emplace_back(glm::vec2{ x, y });
            }
        }

        // init
        {
            std::vector<PackedVertexPosition> packed_vertices;
            std::vector<GeneralShortPacked> packed_data;
            std::vector<uint16_t> _indices;
            
            for (int i = 0; i < vertex_indices.size(); ++i) {
                auto vi = vertex_indices[i];
                auto ui = uv_indices[i];

                _indices.emplace_back(packed_vertices.size());

                PackedVertexPosition pvp{};
                pvp.x = pack<int16_t>(_vertices[vi].x);
                pvp.y = pack<int16_t>(_vertices[vi].y);
                pvp.z = pack<int16_t>(_vertices[vi].z);
                packed_vertices.emplace_back(std::move(pvp));

                GeneralShortPacked gsp{};
                gsp.u0 = pack<int16_t>(_uvs[ui].x);
                gsp.v0 = pack<int16_t>(_uvs[ui].y);
                packed_data.emplace_back(std::move(gsp));
            }

            m_VertexBuffer = Renderer::Get()->CreateBuffer(packed_vertices.data(), packed_vertices.size(), sizeof(PackedVertexPosition), VERTEX_BUFFER);
            m_VertexBufferData = Renderer::Get()->CreateBuffer(packed_data.data(), packed_data.size(), sizeof(GeneralShortPacked), VERTEX_BUFFER);
            m_IndexBuffer = Renderer::Get()->CreateBuffer(_indices.data(), _indices.size(), sizeof(uint16_t), INDEX_BUFFER);
        }

        // textures
        {
            m_Materials.emplace_back("models/jc_characters/main_characters/rico/textures/nanos.dds");

            // load the material
            auto& texture = TextureManager::Get()->GetTexture("models/jc_characters/main_characters/rico/textures/nanos.dds");
            if (texture) {
                texture->LoadFromFile("C:/users/aaron/Desktop/nanos cube/nanos.dds");
                m_Textures.emplace_back(texture);
            }
        }

        // clang-format on
    }
#endif

#if 1
    virtual void SetData(floats_t* vertices, uint16s_t* indices, floats_t* uvs) override final
    {
        using namespace JustCause3::Vertex;

        memset(&m_Block.attributes, 0, sizeof(m_Block.attributes));
        memset(&m_cbMaterialConsts2, 0, sizeof(m_cbMaterialConsts2));

        m_Block.version                            = JustCause3::RenderBlocks::GENERALMKIII_VERSION;
        m_Block.attributes.packed.scale            = 1.f;
        m_Block.attributes.packed.uv0Extent        = {1.f, 1.f};
        m_Block.attributes.emissiveStartFadeDistSq = 2000.f;

        // temp
        m_Block.attributes.flags |= DISABLE_BACKFACE_CULLING;

        // test
        m_MaterialParams[0] = 1.0f;
        m_MaterialParams[1] = 1.0f;
        m_MaterialParams[2] = 1.0f;
        m_MaterialParams[3] = 1.0f;

        std::vector<PackedVertexPosition> packed_vertices;
        std::vector<GeneralShortPacked>   packed_data;

        for (int i = 0; i < vertices->size(); i += 3) {
            PackedVertexPosition pos{};
            pos.x = pack<int16_t>(vertices->at(i));
            pos.y = pack<int16_t>(vertices->at(i + 1));
            pos.z = pack<int16_t>(vertices->at(i + 2));
            packed_vertices.emplace_back(std::move(pos));
        }

        /*for (int i = 0; i < uvs->size(); i+=2) {
            GeneralShortPacked gsp{};
            gsp.u0 = pack<int16_t>(uvs->at(i));
            gsp.v0 = pack<int16_t>(uvs->at(i + 1));
            packed_data.emplace_back(std::move(gsp));
        }*/

        // textures
        {
#if 0
            m_Materials.emplace_back("models/jc_characters/main_characters/rico/textures/nanos.dds");

            // load the material
            auto& texture =
                TextureManager::Get()->GetTexture("models/jc_characters/main_characters/rico/textures/nanos.dds");
            if (texture) {
                texture->LoadFromFile("C:/users/aaron/Desktop/nanos cube/nanos.dds");
                m_Textures.emplace_back(texture);
            }
#endif
        }

        m_VertexBuffer     = Renderer::Get()->CreateBuffer(packed_vertices.data(), packed_vertices.size(),
                                                       sizeof(PackedVertexPosition), VERTEX_BUFFER);
        m_VertexBufferData = Renderer::Get()->CreateBuffer(packed_data.data(), packed_data.size(),
                                                           sizeof(GeneralShortPacked), VERTEX_BUFFER);
        m_IndexBuffer = Renderer::Get()->CreateBuffer(indices->data(), indices->size(), sizeof(uint16_t), INDEX_BUFFER);
    }
#endif

    virtual std::tuple<floats_t, uint16s_t, floats_t> GetData() override final
    {
        using namespace JustCause3::Vertex;

        floats_t  vertices;
        uint16s_t indices = m_IndexBuffer->CastData<uint16_t>();
        floats_t  uvs;

        if (m_Block.attributes.flags & IS_SKINNED) {
            const auto& vb = m_VertexBuffer->CastData<UnpackedVertexPositionXYZW>();

            for (const auto& vertex : vb) {
                vertices.emplace_back(vertex.x * m_Block.attributes.packed.scale);
                vertices.emplace_back(vertex.y * m_Block.attributes.packed.scale);
                vertices.emplace_back(vertex.z * m_Block.attributes.packed.scale);
            }
        } else {
            const auto& vb = m_VertexBuffer->CastData<PackedVertexPosition>();

            for (const auto& vertex : vb) {
                vertices.emplace_back(unpack(vertex.x) * m_Block.attributes.packed.scale);
                vertices.emplace_back(unpack(vertex.y) * m_Block.attributes.packed.scale);
                vertices.emplace_back(unpack(vertex.z) * m_Block.attributes.packed.scale);
            }
        }

        const auto& vbdata = m_VertexBufferData->CastData<GeneralShortPacked>();

        for (const auto& data : vbdata) {
            uvs.emplace_back(unpack(data.u0) * m_Block.attributes.packed.uv0Extent.x);
            uvs.emplace_back(unpack(data.v0) * m_Block.attributes.packed.uv0Extent.y);

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
            static auto world = glm::mat4(1);

            // set vertex shader constants
            m_cbRBIInfo.ModelWorldMatrix   = world;
            m_cbInstanceAttributes.UVScale = {m_Block.attributes.packed.uv0Extent, m_Block.attributes.packed.uv1Extent};
            m_cbInstanceAttributes.DepthBias         = m_Block.attributes.depthBias;
            m_cbInstanceAttributes.QuantizationScale = m_Block.attributes.packed.scale * m_ScaleModifier;
            m_cbInstanceAttributes.EmissiveTODScale =
                (m_Block.attributes.flags & DYNAMIC_EMISSIVE ? m_Block.attributes.emissiveTODScale : 1.0f);
            m_cbInstanceAttributes.EmissiveStartFadeDistSq = m_Block.attributes.emissiveStartFadeDistSq;
        }

        // set the textures
        for (int i = 0; i < 4; ++i) {
            IRenderBlock::BindTexture(i, m_SamplerState);
        }

        // set the constant buffers
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 12, m_cbRBIInfo);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 2, m_cbInstanceAttributes);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[0], 1, m_cbMaterialConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[1], 2, m_cbMaterialConsts2);

        // set the culling mode
        context->m_Renderer->SetCullMode((!(m_Block.attributes.flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK
                                                                                                  : D3D11_CULL_NONE);

        // set the 2nd vertex buffers
        context->m_Renderer->SetVertexStream(m_VertexBufferData, 1);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;

        if (m_Block.attributes.flags & DESTRUCTION) {
            // skin batches
            for (auto& batch : m_SkinBatches) {
                // set the skinning palette data
                context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[2], 3, m_cbSkinningConsts);

                // draw the skin batch
                context->m_Renderer->DrawIndexed(batch.m_Offset, batch.m_Size, m_IndexBuffer);
            }
        } else if (m_Block.attributes.flags & IS_SKINNED) {
            // TODO: skinning palette data
            IRenderBlock::DrawSkinBatches(context, m_SkinBatches);
        } else {
            IRenderBlock::Draw(context);
        }
    }

    virtual void DrawContextMenu() override final
    {
        // clang-format off
        static std::array flag_labels = {
            "Disable Backface Culling",     "Transparency Alpha Blending",  "Transparency Alpha Testing",   "Dynamic Emissive",
            "",                             "Is Skinned",                   "",                             "Use Layered",
            "Use Overlay",                  "Use Decal",                    "Use Damage",                   "",
            "",                             "",                             "Use Anisotropic Filtering",    "Destruction"
        };
        // clang-format on

        ImGuiCustom::DropDownFlags(m_Block.attributes.flags, flag_labels);
    }

    virtual void DrawUI() override final
    {
        ImGui::Text(ICON_FA_COGS "  Attributes");

        ImGui::SliderFloat("Scale", &m_ScaleModifier, 0.0f, 20.0f);

        ImGui::SliderFloat("Depth Bias", &m_Block.attributes.depthBias, 0, 1);

        ImGuiCustom::PushDisabled(!(m_Block.attributes.flags & DYNAMIC_EMISSIVE));
        ImGui::SliderFloat("Emissive Scale", &m_Block.attributes.emissiveTODScale, 0, 1);
        ImGuiCustom::PopDisabled();

        // Textures
        ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
        ImGui::Columns(3, nullptr, false);
        {
            IRenderBlock::DrawTexture("Albedo1Map", 0);
            IRenderBlock::DrawTexture("Gloss1Map", 1);
            IRenderBlock::DrawTexture("Metallic1Map", 2);
            IRenderBlock::DrawTexture("Normal1Map", 3);
        }
        ImGui::EndColumns();
    }
};