#pragma once

#include <StdInc.h>
#include <gtc/type_ptr.hpp>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct CarPaintMMAttributes {
    uint32_t flags;
    float    unknown;
};

static_assert(sizeof(CarPaintMMAttributes) == 0x8, "CarPaintMMAttributes alignment is wrong!");

namespace JustCause3::RenderBlocks
{
static constexpr uint8_t CARPAINTMM_VERSION = 14;

struct CarPaintMM {
    uint8_t              version;
    CarPaintMMAttributes attributes;
};
}; // namespace JustCause3::RenderBlocks
#pragma pack(pop)

class RenderBlockCarPaintMM : public IRenderBlock
{
  private:
    enum {
        SUPPORT_DECALS             = 0x1,
        SUPPORT_DAMAGE_BLEND       = 0x2,
        SUPPORT_DIRT               = 0x4,
        SUPPORT_SOFT_TINT          = 0x10,
        SUPPORT_LAYERED            = 0x20,
        SUPPORT_OVERLAY            = 0x40,
        DISABLE_BACKFACE_CULLING   = 0x80,
        TRANSPARENCY_ALPHABLENDING = 0x100,
        TRANSPARENCY_ALPHATESTING  = 0x200,
        IS_DEFORM                  = 0x1000,
        IS_SKINNED                 = 0x2000,
    };

    struct RBIInfo {
        glm::mat4 ModelWorldMatrix;
        glm::mat4 ModelWorldMatrixPrev; // [unused]
        glm::vec4 ModelDiffuseColor = glm::vec4(1);
        glm::vec4 ModelAmbientColor;  // [unused]
        glm::vec4 ModelSpecularColor; // [unused]
        glm::vec4 ModelEmissiveColor; // [unused]
        glm::vec4 ModelDebugColor;    // [unused]
    } m_cbRBIInfo;

    struct InstanceConsts {
        float IsDeformed = 0.0f;
        float _pad[3]; // [unused]
    } m_cbInstanceConsts;

    struct DeformConsts {
        glm::vec4 ControlPoints[216];
    } m_cbDeformConsts;

    struct CarPaintStaticMaterialParams {
        glm::vec4 m_SpecularGloss;
        glm::vec4 m_Metallic;
        glm::vec4 m_ClearCoat;
        glm::vec4 m_Emissive;
        glm::vec4 m_DiffuseWrap;
        glm::vec4 m_DirtParams;
        glm::vec4 m_DirtBlend;
        glm::vec4 m_DirtColor;
        glm::vec4 m_DecalCount; // [unused]
        glm::vec4 m_DecalWidth;
        glm::vec4 m_Decal1Color;
        glm::vec4 m_Decal2Color;
        glm::vec4 m_Decal3Color;
        glm::vec4 m_Decal4Color;
        glm::vec4 m_DecalBlend;
        glm::vec4 m_Damage;
        glm::vec4 m_DamageBlend;
        glm::vec4 m_DamageColor;
        float     SupportDecals;
        float     SupportDmgBlend;
        float     SupportLayered;
        float     SupportOverlay;
        float     SupportRotating; // [unused]
        float     SupportDirt;
        float     SupportSoftTint;
    } m_cbStaticMaterialParams;

    struct CarPaintDynamicMaterialParams {
        glm::vec4 m_TintColorR;
        glm::vec4 m_TintColorG;
        glm::vec4 m_TintColorB;
        glm::vec4 _unused; // [unused]
        float     m_SpecularGlossOverride;
        float     m_MetallicOverride;
        float     m_ClearCoatOverride;
    } m_cbDynamicMaterialParams;

    struct CarPaintDynamicObjectParams {
        glm::vec4 m_DecalIndex = glm::vec4(0);
        float     m_DirtAmount = 0.0f;
        char      _pad[16];
    } m_cbDynamicObjectParams;

    JustCause3::RenderBlocks::CarPaintMM m_Block;
    JustCause3::CDeformTable             m_DeformTable;
    std::vector<JustCause3::CSkinBatch>  m_SkinBatches;
    std::string                          m_ShaderName              = "carpaintmm";
    std::array<VertexBuffer_t*, 2>       m_VertexBufferData        = {nullptr};
    std::array<ConstantBuffer_t*, 3>     m_VertexShaderConstants   = {nullptr};
    std::array<ConstantBuffer_t*, 4>     m_FragmentShaderConstants = {nullptr};

    glm::mat4 world = glm::mat4(1);

  public:
    RenderBlockCarPaintMM() = default;
    virtual ~RenderBlockCarPaintMM()
    {
        // delete the skin batch lookup
        for (auto& batch : m_SkinBatches) {
            SAFE_DELETE(batch.m_BatchLookup);
        }

        // destroy shader constants
        for (auto& vb : m_VertexBufferData)
            Renderer::Get()->DestroyBuffer(vb);

        for (auto& vsc : m_VertexShaderConstants)
            Renderer::Get()->DestroyBuffer(vsc);

        for (auto& fsc : m_FragmentShaderConstants)
            Renderer::Get()->DestroyBuffer(fsc);
    }

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockCarPaintMM";
    }

    virtual uint32_t GetTypeHash() const override final
    {
        return RenderBlockFactory::RB_CARPAINTMM;
    }

    virtual bool IsOpaque() override final
    {
        return ~m_Block.attributes.flags & TRANSPARENCY_ALPHABLENDING;
    }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader(m_ShaderName);
        m_PixelShader  = ShaderManager::Get()->GetPixelShader("carpaintmm");

        if (m_ShaderName == "carpaintmm") {
            // create the element input desc
            // clang-format off
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   2,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   3,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   4,  DXGI_FORMAT_R32G32_FLOAT,           2,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };
            // clang-format on

            // create the vertex declaration
            m_VertexDeclaration =
                Renderer::Get()->CreateVertexDeclaration(inputDesc, 5, m_VertexShader.get(), "RenderBlockCarPaintMM");
        } else if (m_ShaderName == "carpaintmm_deform") {
            // create the element input desc
            // clang-format off
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R32G32B32A32_FLOAT,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   2,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   3,  DXGI_FORMAT_R32G32B32A32_FLOAT,     1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   4,  DXGI_FORMAT_R32G32_FLOAT,           2,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };
            // clang-format on

            // create the vertex declaration
            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 6, m_VertexShader.get(),
                                                                           "RenderBlockCarPaintMM (deform)");
        } else if (m_ShaderName == "carpaintmm_skinned") {
            __debugbreak();

            /*
                POSITION, 0, DXGI_FORMAT_R32G32B32_FLOAT, 		0, 0, 0, 0
                TEXCOORD, 0, DXGI_FORMAT_R8G8B8A8_UINT, 		0, 12, 0, 0
                TEXCOORD, 1, DXGI_FORMAT_R32G32_FLOAT, 			1, 0, 0, 0
                TEXCOORD, 2, DXGI_FORMAT_R32G32_FLOAT, 			1, 8, 0, 0
                TEXCOORD, 3, DXGI_FORMAT_R32G32_FLOAT, 			1, 16, 0, 0
                TEXCOORD, 4, DXGI_FORMAT_R32G32_FLOAT, 			2, 0, 0, 0
            */
        }

        //
        memset(&m_cbDeformConsts, 0, sizeof(m_cbDeformConsts));

        // create the constant buffers
        m_VertexShaderConstants[0] =
            Renderer::Get()->CreateConstantBuffer(m_cbRBIInfo, "RenderBlockCarPaintMM RBIInfo");
        m_VertexShaderConstants[1] =
            Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "RenderBlockCarPaintMM InstanceConsts");
        m_VertexShaderConstants[2] =
            Renderer::Get()->CreateConstantBuffer(m_cbDeformConsts, "RenderBlockCarPaintMM DeformConsts");
        m_FragmentShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(
            m_cbStaticMaterialParams, 20, "RenderBlockCarPaintMM CarPaintStaticMaterialParams");
        m_FragmentShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(
            m_cbDynamicMaterialParams, 5, "RenderBlockCarPaintMM CarPaintDynamicMaterialParams");
        m_FragmentShaderConstants[2] = Renderer::Get()->CreateConstantBuffer(
            m_cbDynamicObjectParams, "RenderBlockCarPaintMM CarPaintDynamicObjectParams");
        m_FragmentShaderConstants[3] =
            Renderer::Get()->CreateConstantBuffer(m_cbRBIInfo, "RenderBlockCarPaintMM RBIInfo (fragment)");

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

            m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockCarPaintMM");
        }
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace JustCause3::Vertex;

        // read the block attributes
        stream.read((char*)&m_Block, sizeof(m_Block));

        if (m_Block.version != JustCause3::RenderBlocks::CARPAINTMM_VERSION) {
            __debugbreak();
        }

        // read static material params
        stream.read((char*)&m_cbStaticMaterialParams, sizeof(m_cbStaticMaterialParams));

        // read dynamic material params
        stream.read((char*)&m_cbDynamicMaterialParams, sizeof(m_cbDynamicMaterialParams));

        // read the deform table
        ReadDeformTable(stream, &m_DeformTable);

        // read the materials
        ReadMaterials(stream);

        // read the vertex buffers
        if (m_Block.attributes.flags & IS_SKINNED) {
            m_VertexBuffer        = ReadVertexBuffer<UnpackedVertexWithNormal1>(stream);
            m_VertexBufferData[0] = ReadVertexBuffer<UnpackedNormals>(stream);
            m_ShaderName          = "carpaintmm_skinned";
        } else if (m_Block.attributes.flags & IS_DEFORM) {
            m_VertexBuffer        = ReadVertexBuffer<VertexDeformPos>(stream);
            m_VertexBufferData[0] = ReadVertexBuffer<VertexDeformNormal2>(stream);
            m_ShaderName          = "carpaintmm_deform";
        } else {
            m_VertexBuffer        = ReadVertexBuffer<UnpackedVertexPosition>(stream);
            m_VertexBufferData[0] = ReadVertexBuffer<UnpackedNormals>(stream);
        }

        // read skin batch
        if (m_Block.attributes.flags & IS_SKINNED) {
            ReadSkinBatch(stream, &m_SkinBatches);
        }

        // read the layered uv data if needed
        if (m_Block.attributes.flags & (SUPPORT_LAYERED | SUPPORT_OVERLAY)) {
            m_VertexBufferData[1] = ReadVertexBuffer<UnpackedUV>(stream);
        }

        // read index buffer
        m_IndexBuffer = ReadIndexBuffer(stream);
    }

    virtual void Write(std::ostream& stream) override final
    {
        // write the block attributes
        stream.write((char*)&m_Block, sizeof(m_Block));

        // write the static material params
        stream.write((char*)&m_cbStaticMaterialParams, sizeof(m_cbStaticMaterialParams));

        // write the dynamic material params
        stream.write((char*)&m_cbDynamicMaterialParams, sizeof(m_cbDynamicMaterialParams));

        // write the deform table
        WriteDeformTable(stream, &m_DeformTable);

        // write the materials
        WriteMaterials(stream);

        // write the vertex buffer
        WriteBuffer(stream, m_VertexBuffer);
        WriteBuffer(stream, m_VertexBufferData[0]);

        // write skin batch
        if (m_Block.attributes.flags & IS_SKINNED) {
            WriteSkinBatch(stream, &m_SkinBatches);
        }

        // write layered UV data
        if (m_Block.attributes.flags & (SUPPORT_LAYERED | SUPPORT_OVERLAY)) {
            WriteBuffer(stream, m_VertexBufferData[1]);
        }

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

        if (m_Block.attributes.flags & IS_SKINNED) {
            const auto& vb     = m_VertexBuffer->CastData<UnpackedVertexWithNormal1>();
            const auto& vbdata = m_VertexBufferData[0]->CastData<UnpackedNormals>();

            for (const auto& vertex : vb) {
                vertices.emplace_back(vertex.x);
                vertices.emplace_back(vertex.y);
                vertices.emplace_back(vertex.z);
            }

            for (const auto& data : vbdata) {
                uvs.emplace_back(data.u0);
                uvs.emplace_back(data.v0);

                // TODO: u1,v1
            }
        } else if (m_Block.attributes.flags & IS_DEFORM) {
            const auto& vb     = m_VertexBuffer->CastData<VertexDeformPos>();
            const auto& vbdata = m_VertexBufferData[0]->CastData<VertexDeformNormal2>();

            for (const auto& vertex : vb) {
                vertices.emplace_back(vertex.x);
                vertices.emplace_back(vertex.y);
                vertices.emplace_back(vertex.z);
            }

            for (const auto& data : vbdata) {
                uvs.emplace_back(data.u0);
                uvs.emplace_back(data.v0);

                // TODO: u1,v1
            }
        } else {
            const auto& vb     = m_VertexBuffer->CastData<UnpackedVertexPosition>();
            const auto& vbdata = m_VertexBufferData[0]->CastData<UnpackedNormals>();

            for (const auto& vertex : vb) {
                vertices.emplace_back(vertex.x);
                vertices.emplace_back(vertex.y);
                vertices.emplace_back(vertex.z);
            }

            for (const auto& data : vbdata) {
                uvs.emplace_back(data.u0);
                uvs.emplace_back(data.v0);

                // TODO: u1,v1
            }
        }

        if (m_Block.attributes.flags & (SUPPORT_LAYERED | SUPPORT_OVERLAY)) {
            const auto& uvs = m_VertexBufferData[1]->CastData<UnpackedUV>();
            // TODO: u,v
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
            // const auto scale = m_Block.attributes.packed.scale;
            // static auto world = glm::mat4(1);

            // set vertex shader constants
            m_cbRBIInfo.ModelWorldMatrix = glm::scale(world, {1, 1, 1});
        }

        // set the layered albedo map
        if (m_Block.attributes.flags & SUPPORT_LAYERED) {
            const auto& texture = m_Textures.at(10);
            if (texture && texture->IsLoaded()) {
                texture->Use(16);
            }
        }

        // set the overlay albedo map
        if (m_Block.attributes.flags & SUPPORT_OVERLAY) {
            const auto& texture = m_Textures.at(11);
            if (texture && texture->IsLoaded()) {
                texture->Use(17);
            }
        }

        // set the sampler states
        for (int i = 0; i < 10; ++i) {
            context->m_Renderer->SetSamplerState(m_SamplerState, i);
        }

        // set the constant buffers
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 6, m_cbRBIInfo);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 1, m_cbInstanceConsts);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[2], 2, m_cbDeformConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[0], 2, m_cbStaticMaterialParams);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[1], 6, m_cbDynamicMaterialParams);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[2], 7, m_cbDynamicObjectParams);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[3], 8, m_cbRBIInfo);

        // set the culling mode
        context->m_Renderer->SetCullMode((!(m_Block.attributes.flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK
                                                                                                  : D3D11_CULL_NONE);

        // set the 2nd and 3rd vertex buffers
        context->m_Renderer->SetVertexStream(m_VertexBufferData[0], 1);
        context->m_Renderer->SetVertexStream(m_VertexBufferData[1] ? m_VertexBufferData[1] : m_VertexBufferData[0], 2);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;

        if (m_Block.attributes.flags & IS_SKINNED) {
            IRenderBlock::DrawSkinBatches(context, m_SkinBatches);
        } else {
            // set deform data
            if (m_Block.attributes.flags & IS_DEFORM) {
                // TODO
            }

            IRenderBlock::Draw(context);
        }
    }

    virtual void DrawUI() override final
    {
        // clang-format off
        static std::array flag_labels = {
            "Support Decals",               "Support Damage Blend",         "Support Dirt",                 "",
            "Support Soft Tint",            "Support Layered",              "Support Overlay",              "Disable Backface Culling",
            "Transparency Alpha Blending",  "Transparency Alpha Testing",   "",                             "",
            "Is Deformable",                "Is Skinned",                   "",                             ""
        };
        // clang-format on

        if (ImGuiCustom::BitFieldTooltip("Flags", &m_Block.attributes.flags, flag_labels)) {
            // update static material params
            m_cbStaticMaterialParams.SupportDecals   = m_Block.attributes.flags & SUPPORT_DECALS;
            m_cbStaticMaterialParams.SupportDmgBlend = m_Block.attributes.flags & SUPPORT_DAMAGE_BLEND;
            m_cbStaticMaterialParams.SupportLayered  = m_Block.attributes.flags & SUPPORT_LAYERED;
            m_cbStaticMaterialParams.SupportOverlay  = m_Block.attributes.flags & SUPPORT_OVERLAY;
            m_cbStaticMaterialParams.SupportDirt     = m_Block.attributes.flags & SUPPORT_DIRT;
            m_cbStaticMaterialParams.SupportSoftTint = m_Block.attributes.flags & SUPPORT_SOFT_TINT;
        }

#if 0
        ImGui::Text("World");
        ImGui::Separator();
        ImGui::InputFloat4("m0", glm::value_ptr(world[0]));
        ImGui::InputFloat4("m1", glm::value_ptr(world[1]));
        ImGui::InputFloat4("m2", glm::value_ptr(world[2]));
        ImGui::InputFloat4("m3", glm::value_ptr(world[3]));
#endif

        ImGui::ColorEdit3("Diffuse Colour", glm::value_ptr(m_cbRBIInfo.ModelDiffuseColor));

        ImGui::Text("Static Material Params");
        ImGui::Separator();
        ImGui::SliderFloat4("Specular Gloss", glm::value_ptr(m_cbStaticMaterialParams.m_SpecularGloss), 0, 1);
        ImGui::SliderFloat4("Metallic", glm::value_ptr(m_cbStaticMaterialParams.m_Metallic), 0, 1);
        ImGui::SliderFloat4("Clear Coat", glm::value_ptr(m_cbStaticMaterialParams.m_ClearCoat), 0, 1);
        ImGui::SliderFloat4("Emissive", glm::value_ptr(m_cbStaticMaterialParams.m_Emissive), 0, 1);
        ImGui::SliderFloat4("Diffuse Wrap", glm::value_ptr(m_cbStaticMaterialParams.m_DiffuseWrap), 0, 1);

        // supports decals
        ImGuiCustom::PushDisabled(!(m_Block.attributes.flags & SUPPORT_DECALS));
        {
            ImGui::SliderFloat4("Decal Index", glm::value_ptr(m_cbDynamicObjectParams.m_DecalIndex), 0, 1);
            ImGui::SliderFloat4("Decal Width", glm::value_ptr(m_cbStaticMaterialParams.m_DecalWidth), 0, 1);
            ImGui::ColorEdit3("Decal 1 Colour", glm::value_ptr(m_cbStaticMaterialParams.m_Decal1Color));
            ImGui::ColorEdit3("Decal 2 Colour", glm::value_ptr(m_cbStaticMaterialParams.m_Decal2Color));
            ImGui::ColorEdit3("Decal 3 Colour", glm::value_ptr(m_cbStaticMaterialParams.m_Decal3Color));
            ImGui::ColorEdit3("Decal 4 Colour", glm::value_ptr(m_cbStaticMaterialParams.m_Decal4Color));
            ImGui::SliderFloat4("Decal Blend", glm::value_ptr(m_cbStaticMaterialParams.m_DecalBlend), 0, 1);
        }
        ImGuiCustom::PopDisabled();

        // suports damage
        ImGuiCustom::PushDisabled(!(m_Block.attributes.flags & SUPPORT_DAMAGE_BLEND));
        {
            ImGui::SliderFloat4("Damage", glm::value_ptr(m_cbStaticMaterialParams.m_Damage), 0, 1);
            ImGui::SliderFloat4("Damage Blend", glm::value_ptr(m_cbStaticMaterialParams.m_DamageBlend), 0, 1);
            ImGui::ColorEdit4("Damage Colour", glm::value_ptr(m_cbStaticMaterialParams.m_DamageColor));
        }
        ImGuiCustom::PopDisabled();

        // supports dirt
        ImGuiCustom::PushDisabled(!(m_Block.attributes.flags & SUPPORT_DIRT));
        {
            ImGui::SliderFloat("Dirt Amount", &m_cbDynamicObjectParams.m_DirtAmount, 0, 5);
            ImGui::SliderFloat4("Dirt Params", glm::value_ptr(m_cbStaticMaterialParams.m_DirtParams), 0, 1);
            ImGui::SliderFloat4("Dirt Blend", glm::value_ptr(m_cbStaticMaterialParams.m_DirtBlend), 0, 1);
            ImGui::ColorEdit3("Dirt Colour", glm::value_ptr(m_cbStaticMaterialParams.m_DirtColor));
        }
        ImGuiCustom::PopDisabled();

        ImGui::Text("Dynamic Material Params");
        ImGui::Separator();
        ImGui::ColorEdit3("Colour", glm::value_ptr(m_cbDynamicMaterialParams.m_TintColorR));
        ImGui::ColorEdit3("Colour 2", glm::value_ptr(m_cbDynamicMaterialParams.m_TintColorG));
        ImGui::ColorEdit3("Colour 3", glm::value_ptr(m_cbDynamicMaterialParams.m_TintColorB));
        ImGui::SliderFloat("Specular Gloss Override", &m_cbDynamicMaterialParams.m_SpecularGlossOverride, 0, 1);
        ImGui::SliderFloat("Metallic Override", &m_cbDynamicMaterialParams.m_MetallicOverride, 0, 1);
        ImGui::SliderFloat("Clear Coat Override", &m_cbDynamicMaterialParams.m_ClearCoatOverride, 0, 1);
    }
};
