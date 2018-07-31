#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct GeneralMkIIIAttributes
{
    float depthBias;
    char _pad[0x8];
    float emissiveTODScale;
    float emissiveStartFadeDistSq;
    char _pad2[0x10];
    JustCause3::Vertex::SPackedAttribute packed;
    char _pad3[0x4];
    uint32_t flags;
    char _pad4[0x1C];
};

namespace JustCause3::RenderBlocks
{
    struct GeneralMkIII
    {
        uint8_t version;
        GeneralMkIIIAttributes attributes;
    };
};
#pragma pack(pop)

class RenderBlockGeneralMkIII : public IRenderBlock
{
private:
    struct RBIInfo
    {
        glm::mat4 ModelWorldMatrix;
        glm::mat4 ModelWorldMatrixPrev;     // [unused]
        glm::vec4 ModelDiffuseColor;
        glm::vec4 ModelAmbientColor;        // [unused]
        glm::vec4 ModelSpecularColor;       // [unused]
        glm::vec4 ModelEmissiveColor;
        glm::vec4 ModelDebugColor;          // [unused]
    } m_cbRBIInfo;

    struct InstanceAttributes
    {
        glm::vec4 UVScale;
        glm::vec4 RippleAngle;              // [unused]
        glm::vec4 FlashDirection;           // [unused]
        float DepthBias;
        float QuantizationScale;
        float EmissiveStartFadeDistSq;
        float EmissiveTODScale;
        float RippleSpeed;                  // [unused]
        float RippleMagnitude;              // [unused]
        float PixelScale;                   // [unused]
        float OutlineThickness;             // [unused]
    } m_cbInstanceAttributes;

    struct cbSkinningConsts
    {
        glm::vec4 MatrixPalette[2];
    } m_cbSkinningConsts;

    struct MaterialConsts
    {
        glm::vec4 DebugColor;               // [unused]
        float EnableVariableDialecticSpecFresnel;
        float UseMetallicVer2;
    } m_cbMaterialConsts;

    struct MaterialConsts2
    {
        float NormalStrength;              // [unused]
        float Reflectivity_1;              // [unused]
        float Roughness_1;
        float DiffuseWrap_1;
        float Emissive_1;
        float Transmission_1;
        float ClearCoat_1;
        float Roughness_2;                 // [unused]
        float DiffuseWrap_2;               // [unused]
        float Emissive_2;                  // [unused]
        float Transmission_2;              // [unused]
        float Reflectivity_2;              // [unused]
        float ClearCoat_2;                 // [unused]
        float Roughness_3;                 // [unused]
        float DiffuseWrap_3;               // [unused]
        float Emissive_3;                  // [unused]
        float Transmission_3;              // [unused]
        float Reflectivity_3;              // [unused]
        float ClearCoat_3;                 // [unused]
        float Roughness_4;                 // [unused]
        float DiffuseWrap_4;               // [unused]
        float Emissive_4;                  // [unused]
        float Transmission_4;              // [unused]
        float Reflectivity_4;              // [unused]
        float ClearCoat_4;                 // [unused]
        float LayeredHeightMapUVScale;     // [unused]
        float LayeredUVScale;              // [unused]
        float LayeredHeight1Influence;     // [unused]
        float LayeredHeight2Influence;     // [unused]
        float LayeredHeightMapInfluence;   // [unused]
        float LayeredMaskInfluence;        // [unused]
        float LayeredShift;                // [unused]
        float LayeredRoughness;            // [unused]
        float LayeredDiffuseWrap;          // [unused]
        float LayeredEmissive;             // [unused]
        float LayeredTransmission;         // [unused]
        float LayeredReflectivity;         // [unused]
        float LayeredClearCoat;            // [unused]
        float DecalBlend;                  // [unused]
        float DecalBlendNormal;            // [unused]
        float DecalReflectivity;           // [unused]
        float DecalRoughness;              // [unused]
        float DecalDiffuseWrap;            // [unused]
        float DecalEmissive;               // [unused]
        float DecalTransmission;           // [unused]
        float DecalClearCoat;              // [unused]
        float OverlayHeightInfluence;      // [unused]
        float OverlayHeightMapInfluence;   // [unused]
        float OverlayMaskInfluence;        // [unused]
        float OverlayShift;                // [unused]
        float OverlayColorR;               // [unused]
        float OverlayColorG;               // [unused]
        float OverlayColorB;               // [unused]
        float OverlayBrightness;           // [unused]
        float OverlayGloss;                // [unused]
        float OverlayMetallic;             // [unused]
        float OverlayReflectivity;         // [unused]
        float OverlayRoughness;            // [unused]
        float OverlayDiffuseWrap;          // [unused]
        float OverlayEmissive;             // [unused]
        float OverlayTransmission;         // [unused]
        float OverlayClearCoat;            // [unused]
        float DamageReflectivity;          // [unused]
        float DamageRoughness;             // [unused]
        float DamageDiffuseWrap;           // [unused]
        float DamageEmissive;              // [unused]
        float DamageTransmission;          // [unused]
        float DamageHeightInfluence;       // [unused]
        float DamageMaskInfluence;         // [unused]
        float DamageClearCoat;             // [unused]
    } m_cbMaterialConsts2;

    JustCause3::RenderBlocks::GeneralMkIII m_Block;
    VertexBuffer_t* m_VertexBufferData = nullptr;
    std::string m_ShaderName = "generalmkiii";
    std::array<ConstantBuffer_t*, 3> m_VertexShaderConstants = { nullptr };
    std::array<ConstantBuffer_t*, 2> m_FragmentShaderConstants = { nullptr };

public:
    RenderBlockGeneralMkIII() = default;
    virtual ~RenderBlockGeneralMkIII()
    {
        Renderer::Get()->DestroyBuffer(m_VertexBufferData);

        for (auto& vsc : m_VertexShaderConstants)
            Renderer::Get()->DestroyBuffer(vsc);

        for (auto& fsc : m_FragmentShaderConstants)
            Renderer::Get()->DestroyBuffer(fsc);
    }

    virtual const char* GetTypeName() override final { return "RenderBlockGeneralMkIII"; }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader(m_ShaderName);
        m_PixelShader = ShaderManager::Get()->GetPixelShader("generalmkiii");

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
            if (m_Block.attributes.flags & 0x20) {
                __debugbreak();
            }
            else {
                // create the element input desc
                D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                    { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   2,  DXGI_FORMAT_R16G16B16A16_SNORM,     1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                    { "TEXCOORD",   3,  DXGI_FORMAT_R32G32B32_FLOAT,        1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                };

                // create the vertex declaration
                m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(), "RenderBlockGeneralMkIII (packed)");
            }
        //}

        // create the constant buffers
        m_VertexShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbRBIInfo, "RenderBlockGeneralMkIII RBIInfo");
        m_VertexShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(m_cbInstanceAttributes, "RenderBlockGeneralMkIII InstanceAttributes");
        m_FragmentShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts, 2, "RenderBlockGeneralMkIII MaterialConsts");
        m_FragmentShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts2, 18, "RenderBlockGeneralMkIII MaterialConsts2");

        // create skinning palette buffer
        if (m_Block.attributes.flags & 0x8020) {
            m_VertexShaderConstants[2] = Renderer::Get()->CreateConstantBuffer(m_cbSkinningConsts, "RenderBlockGeneralMkIII SkinningConsts");

            // identity the palette data
            for (int i = 0; i < 2; ++i) {
                m_cbSkinningConsts.MatrixPalette[i] = glm::vec4(1);
            }
        } 

        // reset material constants
        memset(&m_cbMaterialConsts, 0, sizeof(m_cbMaterialConsts));

        // create the sampler states
        {
            // SamplerStateCreationParams_t params;
            // m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockGeneralMkIII");
        }
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace JustCause3::Vertex;

        // read the block header
        stream.read((char *)&m_Block, sizeof(m_Block));

        // read constant buffer data
        stream.read((char *)&m_cbMaterialConsts2, sizeof(MaterialConsts2));

        // read the materials
        ReadMaterials(stream);

        // read the vertex buffer
        if (m_Block.attributes.flags & 0x20) {
            std::vector<VertexUnknown> vertices;
            ReadVertexBuffer<VertexUnknown>(stream, &m_VertexBuffer, &vertices);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(vertex.x);
                m_Vertices.emplace_back(vertex.y);
                m_Vertices.emplace_back(vertex.z);
            }

            OutputDebugStringA("read unpacked vertices!\n");
        }
        else {
            std::vector<PackedVertexPosition> vertices;
            ReadVertexBuffer<PackedVertexPosition>(stream, &m_VertexBuffer, &vertices);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(unpack(vertex.x));
                m_Vertices.emplace_back(unpack(vertex.y));
                m_Vertices.emplace_back(unpack(vertex.z));
            }

            OutputDebugStringA("read packed vertices!\n");
        }

        // read the vertex buffer data
        std::vector<GeneralShortPacked> vertices_data;
        ReadVertexBuffer<GeneralShortPacked>(stream, &m_VertexBufferData, &vertices_data);

        for (const auto& data : vertices_data) {
            m_UVs.emplace_back(unpack(data.u0));
            m_UVs.emplace_back(unpack(data.v0));
        }

        // read skin batches
        if (m_Block.attributes.flags & 0x8020) {
            ReadSkinBatch(stream);
        }

        // read index buffer
        ReadIndexBuffer(stream, &m_IndexBuffer);
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        if (!m_Visible) return;

        IRenderBlock::Setup(context);

        // setup the constant buffer
        {
            static auto world = glm::mat4(1);

            // set vertex shader constants
            m_cbRBIInfo.ModelWorldMatrix = world;
            m_cbInstanceAttributes.UVScale = { m_Block.attributes.packed.uv0Extent, m_Block.attributes.packed.uv1Extent };
            m_cbInstanceAttributes.DepthBias = m_Block.attributes.depthBias;
            m_cbInstanceAttributes.QuantizationScale = m_Block.attributes.packed.scale * m_ScaleModifier;
            m_cbInstanceAttributes.EmissiveTODScale = (m_Block.attributes.flags & 8 ? m_Block.attributes.emissiveTODScale : 1.0f);
            m_cbInstanceAttributes.EmissiveStartFadeDistSq = m_Block.attributes.emissiveStartFadeDistSq;
        }

        // set the constant buffers
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 12, m_cbRBIInfo);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 2, m_cbInstanceAttributes);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[0], 1, m_cbMaterialConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[1], 2, m_cbMaterialConsts2);

        context->m_Renderer->SetCullMode((!(m_Block.attributes.flags & 1)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);

        // set the 2nd vertex buffers
        context->m_Renderer->SetVertexStream(m_VertexBufferData, 1);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible) return;

        // does the block have skin batches?
        if (m_Block.attributes.flags & 0x8000) {
            // skin batches
            for (auto& batch : m_SkinBatches) {
                // set the skinning palette data
                context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[2], 3, m_cbSkinningConsts);

                // draw the skin batch
                context->m_Renderer->DrawIndexed(batch.m_Offset, batch.m_Size, m_IndexBuffer);
            }
        }
        // does the block have packed vertices?
        else if (m_Block.attributes.flags & 0x20) {
            __debugbreak();
        }
        else {
            IRenderBlock::Draw(context);
        }
    }

    virtual void DrawUI() override final
    {
        // flags & 2 = something with transparency?

        static std::array flag_labels = {
            "Disable Culling", "", "", "", "", "Has Packed Vertices", "", "",
            "", "", "", "", "", "", "Use Anisotropic Filtering", "Has Skin Batches",

            "", "", "", "", "", "", "", "",
            "", "", "", "", "", "", "", ""
        };

        ImGuiCustom::BitFieldTooltip("Flags", &m_Block.attributes.flags, flag_labels);

        ImGui::SliderFloat("Scale", &m_ScaleModifier, 0.0f, 20.0f);
    }
};