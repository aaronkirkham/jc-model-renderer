#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "renderblockcarpaintmm.h"

#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/imgui/imgui_buttondropdown.h"
#include "graphics/renderer.h"
#include "graphics/shader_manager.h"
#include "graphics/texture.h"
#include "graphics/texture_manager.h"

#include "game/render_block_factory.h"

namespace jc3
{
RenderBlockCarPaintMM::~RenderBlockCarPaintMM()
{
    // delete the skin batch lookup
    for (auto& batch : m_SkinBatches) {
        SAFE_DELETE(batch.m_BatchLookup);
    }

    // destroy shader constants
    for (auto& vb : m_VertexBufferData) {
        Renderer::Get()->DestroyBuffer(vb);
    }

    for (auto& vsc : m_VertexShaderConstants) {
        Renderer::Get()->DestroyBuffer(vsc);
    }

    for (auto& fsc : m_FragmentShaderConstants) {
        Renderer::Get()->DestroyBuffer(fsc);
    }
}

uint32_t RenderBlockCarPaintMM::GetTypeHash() const
{
    return RenderBlockFactory::RB_CARPAINTMM;
}

void RenderBlockCarPaintMM::Create()
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
            Renderer::Get()->CreateVertexDeclaration(inputDesc, 5, m_VertexShader.get(), "CarPaintMM VertexDecl");
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
                                                                       "CarPaintMM VertexDecl (deform)");
    } else if (m_ShaderName == "carpaintmm_skinned") {
#ifdef DEBUG
        __debugbreak();
#endif

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
    m_VertexShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbRBIInfo, "CarPaintMM RBIInfo");
    m_VertexShaderConstants[1] = Renderer::Get()->CreateConstantBuffer(m_cbInstanceConsts, "CarPaintMM InstanceConsts");
    m_VertexShaderConstants[2] = Renderer::Get()->CreateConstantBuffer(m_cbDeformConsts, "CarPaintMM DeformConsts");
    m_FragmentShaderConstants[0] =
        Renderer::Get()->CreateConstantBuffer(m_cbStaticMaterialParams, 20, "CarPaintMM StaticMaterialParams");
    m_FragmentShaderConstants[1] =
        Renderer::Get()->CreateConstantBuffer(m_cbDynamicMaterialParams, 5, "CarPaintMM DynamicMaterialParams");
    m_FragmentShaderConstants[2] =
        Renderer::Get()->CreateConstantBuffer(m_cbDynamicObjectParams, "CarPaintMM DynamicObjectParams");
    m_FragmentShaderConstants[3] = Renderer::Get()->CreateConstantBuffer(m_cbRBIInfo, "CarPaintMM RBIInfo (fragment)");

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

        m_SamplerState = Renderer::Get()->CreateSamplerState(params, "CarPaintMM SamplerState");
    }
}

void RenderBlockCarPaintMM::Setup(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    IRenderBlock::Setup(context);

    // setup the constant buffer
    {
        static auto world = glm::mat4(1);

        // set vertex shader constants
        m_cbRBIInfo.ModelWorldMatrix = world;
    }

    // set the textures
    for (int i = 0; i < 10; ++i) {
        BindTexture(i, m_SamplerState);
    }

    // set the layered albedo map texture
    if (m_Block.m_Attributes.m_Flags & SUPPORT_LAYERED) {
        BindTexture(10, 16);
    } else {
        context->m_Renderer->ClearTexture(16);
    }

    // set the overlay albedo map texture
    if (m_Block.m_Attributes.m_Flags & SUPPORT_OVERLAY) {
        BindTexture(11, 17);
    } else {
        context->m_Renderer->ClearTexture(17);
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
    context->m_Renderer->SetCullMode((!(m_Block.m_Attributes.m_Flags & DISABLE_BACKFACE_CULLING)) ? D3D11_CULL_BACK
                                                                                                  : D3D11_CULL_NONE);

    // set the 2nd and 3rd vertex buffers
    context->m_Renderer->SetVertexStream(m_VertexBufferData[0], 1);
    context->m_Renderer->SetVertexStream(m_VertexBufferData[1] ? m_VertexBufferData[1] : m_VertexBufferData[0], 2);
}

void RenderBlockCarPaintMM::DrawContextMenu()
{
    // clang-format off
    static std::array flag_labels = {
        "Support Decals",               "Support Damage Blend",         "Support Dirt",                 "",
        "Support Soft Tint",            "Support Layered",              "Support Overlay",              "Disable Backface Culling",
        "Transparency Alpha Blending",  "Transparency Alpha Testing",   "",                             "",
        "Is Deformable",                "Is Skinned",                   "",                             ""
    };
    // clang-format on

    if (ImGuiCustom::DropDownFlags(m_Block.m_Attributes.m_Flags, flag_labels)) {
        // update static material params
        m_cbStaticMaterialParams.SupportDecals   = m_Block.m_Attributes.m_Flags & SUPPORT_DECALS;
        m_cbStaticMaterialParams.SupportDmgBlend = m_Block.m_Attributes.m_Flags & SUPPORT_DAMAGE_BLEND;
        m_cbStaticMaterialParams.SupportLayered  = m_Block.m_Attributes.m_Flags & SUPPORT_LAYERED;
        m_cbStaticMaterialParams.SupportOverlay  = m_Block.m_Attributes.m_Flags & SUPPORT_OVERLAY;
        m_cbStaticMaterialParams.SupportDirt     = m_Block.m_Attributes.m_Flags & SUPPORT_DIRT;
        m_cbStaticMaterialParams.SupportSoftTint = m_Block.m_Attributes.m_Flags & SUPPORT_SOFT_TINT;
    }
}

void RenderBlockCarPaintMM::DrawUI()
{
    ImGui::Text(ICON_FA_COGS "  Attributes");

    ImGui::ColorEdit3("Diffuse Colour", glm::value_ptr(m_cbRBIInfo.ModelDiffuseColor));

    ImGui::Text("Static Material Params");
    ImGui::Separator();
    ImGui::SliderFloat4("Specular Gloss", glm::value_ptr(m_cbStaticMaterialParams.m_SpecularGloss), 0, 1);
    ImGui::SliderFloat4("Metallic", glm::value_ptr(m_cbStaticMaterialParams.m_Metallic), 0, 1);
    ImGui::SliderFloat4("Clear Coat", glm::value_ptr(m_cbStaticMaterialParams.m_ClearCoat), 0, 1);
    ImGui::SliderFloat4("Emissive", glm::value_ptr(m_cbStaticMaterialParams.m_Emissive), 0, 1);
    ImGui::SliderFloat4("Diffuse Wrap", glm::value_ptr(m_cbStaticMaterialParams.m_DiffuseWrap), 0, 1);

    // supports decals
    if (m_Block.m_Attributes.m_Flags & SUPPORT_DECALS) {
        ImGui::SliderFloat4("Decal Index", glm::value_ptr(m_cbDynamicObjectParams.m_DecalIndex), 0, 1);
        ImGui::SliderFloat4("Decal Width", glm::value_ptr(m_cbStaticMaterialParams.m_DecalWidth), 0, 1);
        ImGui::ColorEdit3("Decal 1 Colour", glm::value_ptr(m_cbStaticMaterialParams.m_Decal1Color));
        ImGui::ColorEdit3("Decal 2 Colour", glm::value_ptr(m_cbStaticMaterialParams.m_Decal2Color));
        ImGui::ColorEdit3("Decal 3 Colour", glm::value_ptr(m_cbStaticMaterialParams.m_Decal3Color));
        ImGui::ColorEdit3("Decal 4 Colour", glm::value_ptr(m_cbStaticMaterialParams.m_Decal4Color));
        ImGui::SliderFloat4("Decal Blend", glm::value_ptr(m_cbStaticMaterialParams.m_DecalBlend), 0, 1);
    }

    // suports damage
    if (m_Block.m_Attributes.m_Flags & SUPPORT_DAMAGE_BLEND) {
        ImGui::SliderFloat4("Damage", glm::value_ptr(m_cbStaticMaterialParams.m_Damage), 0, 1);
        ImGui::SliderFloat4("Damage Blend", glm::value_ptr(m_cbStaticMaterialParams.m_DamageBlend), 0, 1);
        ImGui::ColorEdit4("Damage Colour", glm::value_ptr(m_cbStaticMaterialParams.m_DamageColor));
    }

    // supports dirt
    if (m_Block.m_Attributes.m_Flags & SUPPORT_DIRT) {
        ImGui::SliderFloat("Dirt Amount", &m_cbDynamicObjectParams.m_DirtAmount, 0, 5);
        ImGui::SliderFloat4("Dirt Params", glm::value_ptr(m_cbStaticMaterialParams.m_DirtParams), 0, 1);
        ImGui::SliderFloat4("Dirt Blend", glm::value_ptr(m_cbStaticMaterialParams.m_DirtBlend), 0, 1);
        ImGui::ColorEdit3("Dirt Colour", glm::value_ptr(m_cbStaticMaterialParams.m_DirtColor));
    }

    ImGui::Text("Dynamic Material Params");
    ImGui::Separator();
    ImGui::ColorEdit3("Colour", glm::value_ptr(m_cbDynamicMaterialParams.m_TintColorR));
    ImGui::ColorEdit3("Colour 2", glm::value_ptr(m_cbDynamicMaterialParams.m_TintColorG));
    ImGui::ColorEdit3("Colour 3", glm::value_ptr(m_cbDynamicMaterialParams.m_TintColorB));
    ImGui::SliderFloat("Specular Gloss Override", &m_cbDynamicMaterialParams.m_SpecularGlossOverride, 0, 1);
    ImGui::SliderFloat("Metallic Override", &m_cbDynamicMaterialParams.m_MetallicOverride, 0, 1);
    ImGui::SliderFloat("Clear Coat Override", &m_cbDynamicMaterialParams.m_ClearCoatOverride, 0, 1);

    // Textures
    ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
    ImGui::Columns(3, nullptr, false);
    {
        DrawUI_Texture("DiffuseMap", 0);
        DrawUI_Texture("NormalMap", 1);
        DrawUI_Texture("PropertyMap", 2);
        DrawUI_Texture("TintMap", 3);
        DrawUI_Texture("DamageNormalMap", 4);
        DrawUI_Texture("DamageAlbedoMap", 5);
        DrawUI_Texture("DirtMap", 6);
        DrawUI_Texture("DecalAlbedoMap", 7);
        DrawUI_Texture("DecalNormalMap", 8);
        DrawUI_Texture("DecalPropertyMap", 9);

        if (m_Block.m_Attributes.m_Flags & SUPPORT_LAYERED) {
            DrawUI_Texture("LayeredAlbedoMap", 10);
        }

        if (m_Block.m_Attributes.m_Flags & SUPPORT_OVERLAY) {
            DrawUI_Texture("OverlayAlbedoMap", 11);
        }
    }
    ImGui::EndColumns();
}

void RenderBlockCarPaintMM::SetData(vertices_t* vertices, uint16s_t* indices, materials_t* materials)
{
    using namespace jc::Vertex;

    memset(&m_Block.m_Attributes, 0, sizeof(m_Block.m_Attributes));
    memset(&m_cbStaticMaterialParams, 0, sizeof(m_cbStaticMaterialParams));
    memset(&m_cbDynamicMaterialParams, 0, sizeof(m_cbDynamicMaterialParams));
    memset(&m_DeformTable, 0, sizeof(m_DeformTable));

    m_Block.m_Version            = jc::RenderBlocks::CARPAINTMM_VERSION;
    m_Block.m_Attributes.m_Flags = DISABLE_BACKFACE_CULLING;

    std::vector<UnpackedVertexPosition> vertices_;
    std::vector<UnpackedNormals>        data_;

    // convert vertices
    for (const auto& vertex : *vertices) {
        // vertices data
        UnpackedVertexPosition pos{vertex.pos.x, vertex.pos.y, vertex.pos.z};
        vertices_.emplace_back(std::move(pos));

        // uv data
        UnpackedNormals uv_data{};
        uv_data.u0 = vertex.uv.x;
        uv_data.v0 = vertex.uv.y;
        uv_data.n  = pack_normal(vertex.normal);
        // @TODO: tangent
        data_.emplace_back(std::move(uv_data));
    }

    // load textures
    MakeEmptyMaterials(jc::RenderBlocks::CARPAINTMM_TEXTURES_COUNT);
    for (const auto& mat : *materials) {
        const auto& [type, filename] = mat;

        // clang-format off
        uint8_t index = 0;
        if (type == "diffuse")       index = 0;
        else if (type == "normal")   index = 1;
        // clang-format on

        m_Textures[index]->LoadFromFile(filename);
        m_Textures[index]->SetFileName(filename.filename()); // @HACK: so we dont write full path
    }

    // create buffers
    m_VertexBuffer        = Renderer::Get()->CreateBuffer(vertices_, VERTEX_BUFFER, "CarPaintMM VertexBuffer");
    m_VertexBufferData[0] = Renderer::Get()->CreateBuffer(data_, VERTEX_BUFFER, "CarPaintMM VertexBufferData[0]");
    m_IndexBuffer         = Renderer::Get()->CreateBuffer(*indices, INDEX_BUFFER, "CarPaintMM IndexBuffer");
}
}; // namespace jc3
