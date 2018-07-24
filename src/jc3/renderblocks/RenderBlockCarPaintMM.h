#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>

#pragma pack(push, 1)
struct CarPaintMMAttributes
{
    uint32_t flags;
    float unknown;
};

namespace JustCause3::RenderBlocks
{
    struct CarPaintMM
    {
        uint8_t version;
        CarPaintMMAttributes attributes;
    };
};
#pragma pack(pop)

class RenderBlockCarPaintMM : public IRenderBlock
{
private:
    struct RBIInfo
    {
        glm::mat4 ModelWorldMatrix;
        glm::mat4 ModelWorldMatrixPrev;     // [unused]
        glm::vec4 ModelDiffuseColor = glm::vec4(1);
        glm::vec4 ModelAmbientColor;        // [unused]
        glm::vec4 ModelSpecularColor;       // [unused]
        glm::vec4 ModelEmissiveColor;       // [unused]
        glm::vec4 ModelDebugColor;          // [unused]
    } m_cbRBIInfo;

    struct InstanceConsts
    {
        float IsDeformed;
        float _pad[3];                      // [unused]
    } m_cbInstanceConsts;

    struct DeformConsts
    {
        glm::vec4 ControlPoints[216];
    } m_cbDeformConsts;

    struct CarPaintStaticMaterialParams
    {
        glm::vec4 m_SpecularGloss;
        glm::vec4 m_Metallic;
        glm::vec4 m_ClearCoat;
        glm::vec4 m_Emissive;
        glm::vec4 m_DiffuseWrap;
        glm::vec4 m_DirtParams;
        glm::vec4 m_DirtBlend;
        glm::vec4 m_DirtColor;
        glm::vec4 m_DecalCount;             // [unused]
        glm::vec4 m_DecalWidth;
        glm::vec4 m_Decal1Color;
        glm::vec4 m_Decal2Color;
        glm::vec4 m_Decal3Color;
        glm::vec4 m_Decal4Color;
        glm::vec4 m_DecalBlend;
        glm::vec4 m_Damage;
        glm::vec4 m_DamageBlend;
        glm::vec4 m_DamageColor;
        float SupportDecals;
        float SupportDmgBlend;
        float SupportLayered;
        float SupportOverlay;
        float SupportRotating;             // [unused]
        float SupportDirt;
        float SupportSoftTint;
    } m_cbStaticMaterialParams;

    struct CarPaintDynamicMaterialParams
    {
        glm::vec4 m_TintColorR;
        glm::vec4 m_TintColorG;
        glm::vec4 m_TintColorB;
        glm::vec4 _unused;                  // [unused]
        float m_SpecularGlossOverride;
        float m_MetallicOverride;
        float m_ClearCoatOverride;
    } m_cbDynamicMaterialParams;

    JustCause3::RenderBlocks::CarPaintMM m_Block;
    JustCause3::Vertex::CDeformTable m_DeformTable;
    std::string m_ShaderName = "carpaintmm";
    std::array<VertexBuffer_t*, 2> m_VertexBufferData = { nullptr };
    std::array<ConstantBuffer_t*, 3> m_VertexShaderConstants = { nullptr };
    std::array<ConstantBuffer_t*, 3> m_FragmentShaderConstants = { nullptr };

public:
    RenderBlockCarPaintMM() = default;
    virtual ~RenderBlockCarPaintMM()
    {
        Renderer::Get()->DestroyBuffer(m_VertexBufferData[0]);
        Renderer::Get()->DestroyBuffer(m_VertexBufferData[1]);
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[0]);
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[1]);
        Renderer::Get()->DestroyBuffer(m_VertexShaderConstants[2]);
        Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants[0]);
        Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants[1]);
        Renderer::Get()->DestroyBuffer(m_FragmentShaderConstants[2]);
    }

    virtual const char* GetTypeName() override final { return "RenderBlockCarPaintMM"; }

    virtual void Create() override final
    {
        // load shaders
        m_VertexShader = ShaderManager::Get()->GetVertexShader(m_ShaderName);
        m_PixelShader = ShaderManager::Get()->GetPixelShader("carpaintmm");

        if (m_ShaderName == "carpaintmm") {
            // create the element input desc
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R32G32B32_FLOAT,        0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   2,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   3,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   4,  DXGI_FORMAT_R32G32_FLOAT,           2,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };

            // create the vertex declaration
            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 5, m_VertexShader.get(), "RenderBlockCarPaintMM");
        }
        else if (m_ShaderName == "carpaintmm_deform") {
            // create the element input desc
            D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                { "POSITION",   0,  DXGI_FORMAT_R32G32B32A32_FLOAT,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   1,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   2,  DXGI_FORMAT_R32G32_FLOAT,           1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   3,  DXGI_FORMAT_R32G32B32A32_FLOAT,     1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
                { "TEXCOORD",   4,  DXGI_FORMAT_R32G32_FLOAT,           2,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
            };

            // create the vertex declaration
            m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 6, m_VertexShader.get(), "RenderBlockCarPaintMM (deform)");
        }
        else if (m_ShaderName == "carpaintmm_skinned") {
            __debugbreak();
        }

        //
        memset(&m_cbDeformConsts, 0, sizeof(m_cbDeformConsts));

        // create the constant buffers
        m_VertexShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbRBIInfo, "RenderBlockCarPaintMM RBIInfo");
        m_VertexShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "RenderBlockCarPaintMM InstanceConsts");
        m_VertexShaderConstants[2] = Renderer::Get()->CreateConstantBuffer(m_cbDeformConsts, "RenderBlockCarPaintMM DeformConsts");
        m_FragmentShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbStaticMaterialParams, 20, "RenderBlockCarPaintMM CarPaintStaticMaterialParams");
        m_FragmentShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(m_cbDynamicMaterialParams, 5, "RenderBlockCarPaintMM CarPaintDynamicMaterialParams");
        m_FragmentShaderConstants[2] = Renderer::Get()->CreateConstantBuffer(m_cbRBIInfo, "RenderBlockCarPaintMM RBIInfo (fragment)");

#if 0
        // create the sampler states
        {
            SamplerStateCreationParams_t params;
            m_SamplerState = Renderer::Get()->CreateSamplerState(params, "RenderBlockCarPaintMM");
        }
#endif
    }

    virtual void Read(std::istream& stream) override final
    {
        using namespace JustCause3::Vertex;

        // read the block header
        stream.read((char *)&m_Block, sizeof(m_Block));

        // read static material params
        stream.read((char *)&m_cbStaticMaterialParams, sizeof(CarPaintStaticMaterialParams));

        // read dynamic material params
        stream.read((char *)&m_cbDynamicMaterialParams, sizeof(CarPaintDynamicMaterialParams));

        // read the deform table
        {
            uint32_t deformTable[256];
            stream.read((char *)&deformTable, sizeof(deformTable));

            for (uint32_t i = 0; i < 256; ++i) {
                const auto data = deformTable[i];

                m_DeformTable.table[i] = data;
                if (data != -1 && m_DeformTable.size < i) {
                    m_DeformTable.size = i;
                }
            }
        }

        // read the materials
        ReadMaterials(stream);

        // read the vertex buffers
        if (_bittest((const long *)&m_Block.attributes.flags, 13)) {
            std::vector<UnpackedVertexWithNormal1> vertices;
            ReadVertexBuffer<UnpackedVertexWithNormal1>(stream, &m_VertexBuffer, &vertices);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(vertex.x);
                m_Vertices.emplace_back(vertex.y);
                m_Vertices.emplace_back(vertex.z);
            }

            std::vector<UnpackedNormals> vertices_data;
            ReadVertexBuffer<UnpackedNormals>(stream, &m_VertexBufferData[0], &vertices_data);

            for (const auto& data : vertices_data) {
                m_UVs.emplace_back(data.u0);
                m_UVs.emplace_back(data.v0);
            }

            m_ShaderName = "carpaintmm_skinned";

            // read skin batch
            ReadSkinBatch(stream);

            OutputDebugStringA("CarPaintMM (skinned) - UnpackedVertexWithNormal1\n");
        }
        else if (_bittest((const long *)&m_Block.attributes.flags, 12)) {
            std::vector<VertexDeformPos> vertices;
            ReadVertexBuffer<VertexDeformPos>(stream, &m_VertexBuffer, &vertices);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(vertex.x);
                m_Vertices.emplace_back(vertex.y);
                m_Vertices.emplace_back(vertex.z);
            }

            std::vector<VertexDeformNormal2> vertices_data;
            ReadVertexBuffer<VertexDeformNormal2>(stream, &m_VertexBufferData[0], &vertices_data);

            for (const auto& data : vertices_data) {
                m_UVs.emplace_back(data.u0);
                m_UVs.emplace_back(data.v0);
            }

            m_ShaderName = "carpaintmm_deform";

            OutputDebugStringA("CarPaintMM (deform) - VertexDeformPos\n");
        }
        else {
            std::vector<UnpackedVertexPosition> vertices;
            ReadVertexBuffer<UnpackedVertexPosition>(stream, &m_VertexBuffer, &vertices);

            for (const auto& vertex : vertices) {
                m_Vertices.emplace_back(vertex.x);
                m_Vertices.emplace_back(vertex.y);
                m_Vertices.emplace_back(vertex.z);
            }

            std::vector<UnpackedNormals> vertices_data;
            ReadVertexBuffer<UnpackedNormals>(stream, &m_VertexBufferData[0], &vertices_data);

            for (const auto& data : vertices_data) {
                m_UVs.emplace_back(data.u0);
                m_UVs.emplace_back(data.v0);
            }

            OutputDebugStringA("CarPaintMM - UnpackedVertexPosition\n");
        }

        // read the layered uv data if needed
        if (!((m_Block.attributes.flags & 0x60) == 0)) {
            std::vector<UnpackedUV> uvs;
            ReadVertexBuffer<UnpackedUV>(stream, &m_VertexBufferData[1], &uvs);
            
            for (const auto& uv : uvs) {
                m_UVs.emplace_back(uv.u);
                m_UVs.emplace_back(uv.v);
            }

            OutputDebugStringA("CarPaintMM - has layered UV data\n");
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
            //const auto scale = m_Block.attributes.packed.scale;
            auto world = glm::mat4(1);

            // set vertex shader constants
            m_cbRBIInfo.ModelWorldMatrix = world;

            // set fragment shader constants
            //
        }

        // set the constant buffers
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 6, m_cbRBIInfo);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 1, m_cbInstanceConsts);
        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[2], 2, m_cbDeformConsts);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[0], 2, m_cbStaticMaterialParams);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[1], 6, m_cbDynamicMaterialParams);
        context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[2], 8, m_cbRBIInfo);

        context->m_Renderer->SetCullMode(static_cast<D3D11_CULL_MODE>(~(m_Block.attributes.flags >> 6) & 2 | 1));

        // set the 2nd and 3rd vertex buffers
        context->m_Renderer->SetVertexStream(m_VertexBufferData[0], 1);
        context->m_Renderer->SetVertexStream(m_VertexBufferData[1] ? m_VertexBufferData[1] : m_VertexBufferData[0], 2);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible) return;

        if (m_Block.attributes.flags & 0x2000) {
            IRenderBlock::DrawSkinBatches(context);
        }
        else {
            // set deform data
            if (m_Block.attributes.flags & 0x1000) {
                // TODO
            }

            IRenderBlock::Draw(context);
        }
    }

    virtual void DrawUI() override final
    {
        static std::array flag_labels = {
            "", "", "", "", "", "Layered Albedo Map", "Overlay Albedo Map", "Disable Culling",
            "Is Transparent", "Alpha Test Enabled", "", "", "Is Deformable", "Is Skinned", "", "",
            "", "", "", "", "", "", "", "",
            "", "", "", "", "", "", "", ""
        };

        ImGuiCustom::BitFieldTooltip("Flags", &m_Block.attributes.flags, flag_labels);


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
        ImGui::Checkbox("Support Decals", (bool *)&m_cbStaticMaterialParams.SupportDecals);
        ImGui::Checkbox("Support Damage Blend", (bool *)&m_cbStaticMaterialParams.SupportDmgBlend);
        ImGui::Checkbox("Support Layered", (bool *)&m_cbStaticMaterialParams.SupportLayered);
        ImGui::Checkbox("Support Overlay", (bool *)&m_cbStaticMaterialParams.SupportOverlay);
        ImGui::Checkbox("Support Dirt", (bool *)&m_cbStaticMaterialParams.SupportDirt);
        ImGui::Checkbox("Support Soft Tint", (bool *)&m_cbStaticMaterialParams.SupportSoftTint);

        ImGui::Text("Dynamic Material Params");
        ImGui::Separator();
        ImGui::SliderFloat4("R", glm::value_ptr(m_cbDynamicMaterialParams.m_TintColorR), 0, 1);
        ImGui::SliderFloat4("G", glm::value_ptr(m_cbDynamicMaterialParams.m_TintColorG), 0, 1);
        ImGui::SliderFloat4("B", glm::value_ptr(m_cbDynamicMaterialParams.m_TintColorB), 0, 1);
    }
};