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
struct CarPaintMM {
    uint8_t              version;
    CarPaintMMAttributes attributes;
};
}; // namespace JustCause3::RenderBlocks
#pragma pack(pop)

class RenderBlockCarPaintMM : public IRenderBlock
{
  private:
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
    std::string                          m_ShaderName              = "carpaintmm";
    std::array<VertexBuffer_t*, 2>       m_VertexBufferData        = {nullptr};
    std::array<ConstantBuffer_t*, 3>     m_VertexShaderConstants   = {nullptr};
    std::array<ConstantBuffer_t*, 4>     m_FragmentShaderConstants = {nullptr};

    glm::mat4 world = glm::mat4(1);

  public:
    RenderBlockCarPaintMM() = default;
    virtual ~RenderBlockCarPaintMM()
    {
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
        return ~(LOWORD(m_Block.attributes.flags) >> 8) & 1;
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
            D3D11_SAMPLER_DESC params;
            ZeroMemory(&params, sizeof(params));
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

        // read static material params
        stream.read((char*)&m_cbStaticMaterialParams, sizeof(m_cbStaticMaterialParams));

        // read dynamic material params
        stream.read((char*)&m_cbDynamicMaterialParams, sizeof(m_cbDynamicMaterialParams));

        // read the deform table
        ReadDeformTable(stream, &m_DeformTable);

        // read the materials
        ReadMaterials(stream);

        // read the vertex buffers
        if (_bittest((const long*)&m_Block.attributes.flags, 13)) {
            std::vector<UnpackedVertexWithNormal1> vertices;
            ReadVertexBuffer<UnpackedVertexWithNormal1>(stream, &m_VertexBuffer, &vertices);

            std::vector<UnpackedNormals> vertices_data;
            ReadVertexBuffer<UnpackedNormals>(stream, &m_VertexBufferData[0], &vertices_data);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(vertex.x);
                m_Vertices.emplace_back(vertex.y);
                m_Vertices.emplace_back(vertex.z);
            }

            for (const auto& data : vertices_data) {
                m_UVs.emplace_back(data.u0);
                m_UVs.emplace_back(data.v0);
            }

            m_ShaderName = "carpaintmm_skinned";

            // read skin batch
            ReadSkinBatch(stream);
        } else if (_bittest((const long*)&m_Block.attributes.flags, 12)) {
            std::vector<VertexDeformPos> vertices;
            ReadVertexBuffer<VertexDeformPos>(stream, &m_VertexBuffer, &vertices);

            std::vector<VertexDeformNormal2> vertices_data;
            ReadVertexBuffer<VertexDeformNormal2>(stream, &m_VertexBufferData[0], &vertices_data);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(vertex.x);
                m_Vertices.emplace_back(vertex.y);
                m_Vertices.emplace_back(vertex.z);
            }

            for (const auto& data : vertices_data) {
                m_UVs.emplace_back(data.u0);
                m_UVs.emplace_back(data.v0);
            }

            m_ShaderName = "carpaintmm_deform";
        } else {
            std::vector<UnpackedVertexPosition> vertices;
            ReadVertexBuffer<UnpackedVertexPosition>(stream, &m_VertexBuffer, &vertices);

            std::vector<UnpackedNormals> vertices_data;
            ReadVertexBuffer<UnpackedNormals>(stream, &m_VertexBufferData[0], &vertices_data);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(vertex.x);
                m_Vertices.emplace_back(vertex.y);
                m_Vertices.emplace_back(vertex.z);
            }

            for (const auto& data : vertices_data) {
                m_UVs.emplace_back(data.u0);
                m_UVs.emplace_back(data.v0);
            }
        }

        // read the layered uv data if needed
        if (!((m_Block.attributes.flags & 0x60) == 0)) {
            std::vector<UnpackedUV> uvs;
            ReadVertexBuffer<UnpackedUV>(stream, &m_VertexBufferData[1], &uvs);

            for (const auto& uv : uvs) {
                m_UVs.emplace_back(uv.u);
                m_UVs.emplace_back(uv.v);
            }
        }

        // read index buffer
        ReadIndexBuffer(stream, &m_IndexBuffer);
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
        WriteVertexBuffer(stream, m_VertexBuffer);
        WriteVertexBuffer(stream, m_VertexBufferData[0]);

        // write layered UV data
        if (!((m_Block.attributes.flags & 0x60) == 0)) {
            WriteVertexBuffer(stream, m_VertexBufferData[1]);
        }

        // write the index buffer
        WriteIndexBuffer(stream, m_IndexBuffer);
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
        if (m_Block.attributes.flags & 0x20) {
            const auto& texture = m_Textures.at(10);
            if (texture && texture->IsLoaded()) {
                texture->Use(16);
            }
        }

        // set the overlay albedo map
        if (m_Block.attributes.flags & 0x40) {
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
        context->m_Renderer->SetCullMode(static_cast<D3D11_CULL_MODE>(~(m_Block.attributes.flags >> 6) & 2 | 1));

        // set the 2nd and 3rd vertex buffers
        context->m_Renderer->SetVertexStream(m_VertexBufferData[0], 1);
        context->m_Renderer->SetVertexStream(m_VertexBufferData[1] ? m_VertexBufferData[1] : m_VertexBufferData[0], 2);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;

        if (m_Block.attributes.flags & 0x2000) {
            IRenderBlock::DrawSkinBatches(context);
        } else {
            // set deform data
            if (m_Block.attributes.flags & 0x1000) {
                // TODO
            }

            IRenderBlock::Draw(context);
        }
    }

    virtual void DrawUI() override final
    {
        static std::array flag_labels = {"",
                                         "",
                                         "",
                                         "",
                                         "",
                                         "Layered Albedo Map",
                                         "Overlay Albedo Map",
                                         "Disable Culling",
                                         "Is Transparent",
                                         "Alpha Test Enabled",
                                         "",
                                         "",
                                         "Is Deformable",
                                         "Is Skinned",
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

        ImGui::Text("World");
        ImGui::Separator();
        ImGui::InputFloat4("m0", glm::value_ptr(world[0]));
        ImGui::InputFloat4("m1", glm::value_ptr(world[1]));
        ImGui::InputFloat4("m2", glm::value_ptr(world[2]));
        ImGui::InputFloat4("m3", glm::value_ptr(world[3]));

        ImGui::ColorEdit3("Diffuse Colour", glm::value_ptr(m_cbRBIInfo.ModelDiffuseColor));

        ImGui::Text("Static Material Params");
        ImGui::Separator();
        ImGui::SliderFloat4("Specular Gloss", glm::value_ptr(m_cbStaticMaterialParams.m_SpecularGloss), 0, 1);
        ImGui::SliderFloat4("Metallic", glm::value_ptr(m_cbStaticMaterialParams.m_Metallic), 0, 1);
        ImGui::SliderFloat4("Clear Coat", glm::value_ptr(m_cbStaticMaterialParams.m_ClearCoat), 0, 1);
        ImGui::SliderFloat4("Emissive", glm::value_ptr(m_cbStaticMaterialParams.m_Emissive), 0, 1);
        ImGui::SliderFloat4("Diffuse Wrap", glm::value_ptr(m_cbStaticMaterialParams.m_DiffuseWrap), 0, 1);
        ImGui::SliderFloat4("Dirt Params", glm::value_ptr(m_cbStaticMaterialParams.m_DirtParams), 0, 1);
        ImGui::SliderFloat4("Dirt Blend", glm::value_ptr(m_cbStaticMaterialParams.m_DirtBlend), 0, 1);
        ImGui::ColorEdit3("Dirt Colour", glm::value_ptr(m_cbStaticMaterialParams.m_DirtColor));
        ImGui::SliderFloat4("Decal Width", glm::value_ptr(m_cbStaticMaterialParams.m_DecalWidth), 0, 1);
        ImGui::ColorEdit3("Decal 1 Colour", glm::value_ptr(m_cbStaticMaterialParams.m_Decal1Color));
        ImGui::ColorEdit3("Decal 2 Colour", glm::value_ptr(m_cbStaticMaterialParams.m_Decal2Color));
        ImGui::ColorEdit3("Decal 3 Colour", glm::value_ptr(m_cbStaticMaterialParams.m_Decal3Color));
        ImGui::ColorEdit3("Decal 4 Colour", glm::value_ptr(m_cbStaticMaterialParams.m_Decal4Color));
        ImGui::SliderFloat4("Decal Blend", glm::value_ptr(m_cbStaticMaterialParams.m_DecalBlend), 0, 1);
        ImGui::SliderFloat4("Damage", glm::value_ptr(m_cbStaticMaterialParams.m_Damage), 0, 1);
        ImGui::SliderFloat4("Damage Blend", glm::value_ptr(m_cbStaticMaterialParams.m_DamageBlend), 0, 1);
        ImGui::SliderFloat4("Damage Colour", glm::value_ptr(m_cbStaticMaterialParams.m_DamageColor), 0, 1);
        ImGui::Checkbox("Support Decals", (bool*)&m_cbStaticMaterialParams.SupportDecals);
        ImGui::Checkbox("Support Damage Blend", (bool*)&m_cbStaticMaterialParams.SupportDmgBlend);
        ImGui::Checkbox("Support Layered", (bool*)&m_cbStaticMaterialParams.SupportLayered);
        ImGui::Checkbox("Support Overlay", (bool*)&m_cbStaticMaterialParams.SupportOverlay);
        ImGui::Checkbox("Support Dirt", (bool*)&m_cbStaticMaterialParams.SupportDirt);
        ImGui::Checkbox("Support Soft Tint", (bool*)&m_cbStaticMaterialParams.SupportSoftTint);

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
