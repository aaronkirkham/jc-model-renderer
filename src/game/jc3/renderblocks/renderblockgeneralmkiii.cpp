#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "renderblockgeneralmkiii.h"

#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/imgui/imgui_buttondropdown.h"
#include "graphics/renderer.h"
#include "graphics/shader_manager.h"
#include "graphics/texture.h"
#include "graphics/texture_manager.h"

#include "game/formats/render_block_model.h"
#include "game/render_block_factory.h"

namespace jc3
{
RenderBlockGeneralMkIII::~RenderBlockGeneralMkIII()
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

uint32_t RenderBlockGeneralMkIII::GetTypeHash() const
{
    return RenderBlockFactory::RB_GENERALMKIII;
}

void RenderBlockGeneralMkIII::Create()
{
    // reset constants
    memset(&m_cbInstanceAttributes, 0, sizeof(m_cbInstanceAttributes));
    memset(&m_cbMaterialConsts, 0, sizeof(m_cbMaterialConsts));

    // load shaders
    m_VertexShader = ShaderManager::Get()->GetVertexShader(m_ShaderName);
    m_PixelShader  = ShaderManager::Get()->GetPixelShader("generalmkiii");

    // clang-format off
    D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
        { "POSITION",   0,  DXGI_FORMAT_R16G16B16A16_SNORM,     0,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        { "TEXCOORD",   2,  DXGI_FORMAT_R16G16B16A16_SNORM,     1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
        { "TEXCOORD",   3,  DXGI_FORMAT_R32G32B32_FLOAT,        1,  D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,    0 },
    };
    // clang-format on

    // create the vertex declaration
    m_VertexDeclaration =
        Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(), "GeneralMkIII (packed)");

    // create the constant buffers
    m_VertexShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbRBIInfo, "GeneralMkIII RBIInfo");
    m_VertexShaderConstants[1] =
        Renderer::Get()->CreateConstantBuffer(m_cbInstanceAttributes, "GeneralMkIII InstanceAttributes");
    m_FragmentShaderConstants[0] =
        Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts, 2, "GeneralMkIII MaterialConsts");
    m_FragmentShaderConstants[1] =
        Renderer::Get()->CreateConstantBuffer(m_cbMaterialConsts2, 18, "GeneralMkIII MaterialConsts2");

    // create skinning palette buffer
    if (m_Block.m_Attributes.m_Flags & (IS_SKINNED | DESTRUCTION)) {
        m_VertexShaderConstants[2] =
            Renderer::Get()->CreateConstantBuffer(m_cbSkinningConsts, "GeneralMkIII SkinningConsts");

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

        m_SamplerState = Renderer::Get()->CreateSamplerState(params, "GeneralMkIII SamplerState");
    }
}

void RenderBlockGeneralMkIII::Read(std::istream& stream)
{
    using namespace jc::Vertex;

    // read the block attributes
    stream.read((char*)&m_Block, sizeof(m_Block));
    assert(m_Block.m_Version == jc::RenderBlocks::GENERALMKIII_VERSION);

    // read constant buffer data
    stream.read((char*)&m_cbMaterialConsts2, sizeof(m_cbMaterialConsts2));

    // read the materials
    ReadMaterials(stream);

    // models/jc_environments/structures/military/shared/textures/metal_grating_001_alpha_dif.ddsc
    if (m_Textures[0] && m_Textures[0]->GetHash() == 0x7d1af10d) {
        m_Block.m_Attributes.m_Flags |= ANISOTROPIC_FILTERING;
    }

    // read the vertex buffer
    if (m_Block.m_Attributes.m_Flags & IS_SKINNED) {
        m_VertexBuffer = ReadVertexBuffer<UnpackedVertexPositionXYZW>(stream);
    } else {
        m_VertexBuffer = ReadVertexBuffer<PackedVertexPosition>(stream);
    }

    // read the vertex buffer data
    m_VertexBufferData = ReadVertexBuffer<GeneralShortPacked>(stream);

    // read skin batches
    if (m_Block.m_Attributes.m_Flags & (IS_SKINNED | DESTRUCTION)) {
        ReadSkinBatch(stream, &m_SkinBatches);
    }

    // read index buffer
    m_IndexBuffer = ReadIndexBuffer(stream);
}

void RenderBlockGeneralMkIII::Setup(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    IRenderBlock::Setup(context);

    const auto  flags = m_Block.m_Attributes.m_Flags;
    static auto world = glm::mat4(1);

    // rbi info consts
    m_cbRBIInfo.ModelWorldMatrix = world;
    context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[0], 12, m_cbRBIInfo);

    // instance consts
    {
        m_cbInstanceAttributes.UVScale                 = {m_Block.m_Attributes.m_Packed.m_UV0Extent,
                                          m_Block.m_Attributes.m_Packed.m_UV1Extent};
        m_cbInstanceAttributes.DepthBias               = m_Block.m_Attributes.m_DepthBias;
        m_cbInstanceAttributes.QuantizationScale       = m_Block.m_Attributes.m_Packed.m_Scale;
        m_cbInstanceAttributes.EmissiveStartFadeDistSq = m_Block.m_Attributes.m_EmissiveStartFadeDistSq;
        m_cbInstanceAttributes.EmissiveTODScale        = 1.0f;

        if (m_Block.m_Attributes.m_Flags & DYNAMIC_EMISSIVE) {
            m_cbInstanceAttributes.EmissiveTODScale = m_Block.m_Attributes.m_EmissiveTODScale;
        }

        context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[1], 2, m_cbInstanceAttributes);
    }

    // material consts
    context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[0], 1, m_cbMaterialConsts);
    context->m_Renderer->SetPixelShaderConstants(m_FragmentShaderConstants[1], 2, m_cbMaterialConsts2);

    // set the textures
    {
        for (int i = 0; i < 4; ++i) {
            BindTexture(i, m_SamplerState);
        }

        // overlay
        if (flags & USE_OVERLAY || flags & USE_LAYERED) {
            BindTexture(6, 4, m_SamplerState);
            BindTexture(5, 5, m_SamplerState);

            // LayeredMaskMap                    texture  float4          2d             t4      1
            // LayeredHeightMap                  texture  float4          2d             t5      1
        } else {
            context->m_Renderer->ClearTextures(4, 5);
        }

        // layered
        if (flags & USE_LAYERED) {
            BindTexture(7, 6, m_SamplerState);
            BindTexture(8, 7, m_SamplerState);
            BindTexture(9, 8, m_SamplerState);
            BindTexture(10, 9, m_SamplerState);

            // LayeredAlbedoMap                  texture  float4          2d             t6      1
            // LayeredGlossMap                   texture  float4          2d             t7      1
            // LayeredMetallicMap                texture  float4          2d             t8      1
            // LayeredNormalMap                  texture  float4          2d             t9      1
        } else {
            context->m_Renderer->ClearTextures(6, 9);
        }

        // USE_DECAL
        if (flags & USE_DECAL) {
            BindTexture(11, 33, m_SamplerState);
            BindTexture(12, 34, m_SamplerState);
            BindTexture(13, 35, m_SamplerState);
            BindTexture(14, 36, m_SamplerState);

            // DecalAlbedoMap                    texture  float4          2d            t33      1
            // DecalGlossMap                     texture  float4          2d            t34      1
            // DecalMetallicMap                  texture  float4          2d            t35      1
            // DecalNormalMap                    texture  float4          2d            t36      1
        } else {
            context->m_Renderer->ClearTextures(33, 36);
        }

        // USE_DAMAGE
        if (flags & USE_DAMAGE) {
            BindTexture(15, 33, m_SamplerState);
            BindTexture(16, 34, m_SamplerState);
            BindTexture(17, 35, m_SamplerState);
            BindTexture(18, 36, m_SamplerState);
            BindTexture(19, 37, m_SamplerState);

            // DamageAlbedoMap                   texture  float4          2d            t33      1
            // DamageGlossMap                    texture  float4          2d            t34      1
            // DamageMaskMap                     texture  float4          2d            t35      1
            // DamageMetallicMap                 texture  float4          2d            t36      1
            // DamageNormalMap                   texture  float4          2d            t37      1
        } else {
            context->m_Renderer->ClearTextures(33, 37);
        }
    }

    // set the culling mode
    context->m_Renderer->SetCullMode((!(m_Block.m_Attributes.m_Flags & BACKFACE_CULLING)) ? D3D11_CULL_BACK
                                                                                          : D3D11_CULL_NONE);

    // set the 2nd vertex buffers
    context->m_Renderer->SetVertexStream(m_VertexBufferData, 1);
}

void RenderBlockGeneralMkIII::Draw(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    if (m_Block.m_Attributes.m_Flags & DESTRUCTION) {
        // skin batches
        for (auto& batch : m_SkinBatches) {
            // set the skinning palette data
            context->m_Renderer->SetVertexShaderConstants(m_VertexShaderConstants[2], 3, m_cbSkinningConsts);

            // draw the skin batch
            context->m_Renderer->DrawIndexed(batch.m_Offset, batch.m_Size, m_IndexBuffer);
        }
    } else if (m_Block.m_Attributes.m_Flags & IS_SKINNED) {
        // TODO: skinning palette data
        IRenderBlock::DrawSkinBatches(context, m_SkinBatches);
    } else {
        IRenderBlock::Draw(context);
    }
}

void RenderBlockGeneralMkIII::DrawContextMenu()
{
    // clang-format off
    static std::array flag_labels = {
        "Disable Backface Culling",     "Transparency Alpha Blending",  "Transparency Alpha Testing",   "Dynamic Emissive",
        "",                             "Is Skinned",                   "",                             "Use Layered",
        "Use Overlay",                  "Use Decal",                    "Use Damage",                   "",
        "",                             "",                             "Use Anisotropic Filtering",    "Destruction"
    };
    // clang-format on

    ImGuiCustom::DropDownFlags(m_Block.m_Attributes.m_Flags, flag_labels);
}

void RenderBlockGeneralMkIII::DrawUI()
{
    ImGui::Text(ICON_FA_COGS "  Attributes");
    ImGui::DragFloat("Scale", &m_Block.m_Attributes.m_Packed.m_Scale, 1.0f, 0.0f);
    ImGui::DragFloat2("UV0Extent", glm::value_ptr(m_Block.m_Attributes.m_Packed.m_UV0Extent), 1.0f, 0.0f);
    ImGui::DragFloat2("UV1Extent", glm::value_ptr(m_Block.m_Attributes.m_Packed.m_UV1Extent), 1.0f, 0.0f);
    ImGui::DragFloat("ColourExtent", &m_Block.m_Attributes.m_Packed.m_ColourExtent, 1.0f, 0.0f);
    // ImGui::ColorPicker4("Colour", &m_Block.m_Attributes.m_Packed.m_Colour, 1.0f, 0.0f);
    ImGui::DragFloat("Depth Bias", &m_Block.m_Attributes.m_DepthBias, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Emissive Start Fade Dist", &m_Block.m_Attributes.m_EmissiveStartFadeDistSq, 0.01f, 0.0f, 1.0f);

    if (m_Block.m_Attributes.m_Flags & DYNAMIC_EMISSIVE) {
        ImGui::DragFloat("Emissive Time of Day Scale", &m_Block.m_Attributes.m_EmissiveTODScale, 0.01f, 0.0f, 1.0f);
    }

    if (ImGui::Button("Material Consts")) {
        show_material_consts = !show_material_consts;
    }

#ifdef _DEBUG
    if (ImGui::Button("Recalc Bounding Box")) {
        m_Owner->RecalculateBoundingBox();
    }
#endif

    if (show_material_consts) {
        ImGui::SetNextWindowSize({560, 460}, ImGuiCond_Appearing);
        if (ImGui::Begin("Material Consts", &show_material_consts, ImGuiWindowFlags_NoCollapse)) {
            ImGui::DragFloat("NormalStrength", &m_cbMaterialConsts2.NormalStrength);
            ImGui::DragFloat("Reflectivity_1", &m_cbMaterialConsts2.Reflectivity_1);
            ImGui::DragFloat("Roughness_1", &m_cbMaterialConsts2.Roughness_1);
            ImGui::DragFloat("DiffuseWrap_1", &m_cbMaterialConsts2.DiffuseWrap_1);
            ImGui::DragFloat("Emissive_1", &m_cbMaterialConsts2.Emissive_1);
            ImGui::DragFloat("Transmission_1", &m_cbMaterialConsts2.Transmission_1);
            ImGui::DragFloat("ClearCoat_1", &m_cbMaterialConsts2.ClearCoat_1);
            ImGui::DragFloat("Roughness_2", &m_cbMaterialConsts2.Roughness_2);
            ImGui::DragFloat("DiffuseWrap_2", &m_cbMaterialConsts2.DiffuseWrap_2);
            ImGui::DragFloat("Emissive_2", &m_cbMaterialConsts2.Emissive_2);
            ImGui::DragFloat("Transmission_2", &m_cbMaterialConsts2.Transmission_2);
            ImGui::DragFloat("Reflectivity_2", &m_cbMaterialConsts2.Reflectivity_2);
            ImGui::DragFloat("ClearCoat_2", &m_cbMaterialConsts2.ClearCoat_2);
            ImGui::DragFloat("Roughness_3", &m_cbMaterialConsts2.Roughness_3);
            ImGui::DragFloat("DiffuseWrap_3", &m_cbMaterialConsts2.DiffuseWrap_3);
            ImGui::DragFloat("Emissive_3", &m_cbMaterialConsts2.Emissive_3);
            ImGui::DragFloat("Transmission_3", &m_cbMaterialConsts2.Transmission_3);
            ImGui::DragFloat("Reflectivity_3", &m_cbMaterialConsts2.Reflectivity_3);
            ImGui::DragFloat("ClearCoat_3", &m_cbMaterialConsts2.ClearCoat_3);
            ImGui::DragFloat("Roughness_4", &m_cbMaterialConsts2.Roughness_4);
            ImGui::DragFloat("DiffuseWrap_4", &m_cbMaterialConsts2.DiffuseWrap_4);
            ImGui::DragFloat("Emissive_4", &m_cbMaterialConsts2.Emissive_4);
            ImGui::DragFloat("Transmission_4", &m_cbMaterialConsts2.Transmission_4);
            ImGui::DragFloat("Reflectivity_4", &m_cbMaterialConsts2.Reflectivity_4);
            ImGui::DragFloat("ClearCoat_4", &m_cbMaterialConsts2.ClearCoat_4);
            ImGui::DragFloat("LayeredHeightMapUVScale", &m_cbMaterialConsts2.LayeredHeightMapUVScale);
            ImGui::DragFloat("LayeredUVScale", &m_cbMaterialConsts2.LayeredUVScale);
            ImGui::DragFloat("LayeredHeight1Influence", &m_cbMaterialConsts2.LayeredHeight1Influence);
            ImGui::DragFloat("LayeredHeight2Influence", &m_cbMaterialConsts2.LayeredHeight2Influence);
            ImGui::DragFloat("LayeredHeightMapInfluence", &m_cbMaterialConsts2.LayeredHeightMapInfluence);
            ImGui::DragFloat("LayeredMaskInfluence", &m_cbMaterialConsts2.LayeredMaskInfluence);
            ImGui::DragFloat("LayeredShift", &m_cbMaterialConsts2.LayeredShift);
            ImGui::DragFloat("LayeredRoughness", &m_cbMaterialConsts2.LayeredRoughness);
            ImGui::DragFloat("LayeredDiffuseWrap", &m_cbMaterialConsts2.LayeredDiffuseWrap);
            ImGui::DragFloat("LayeredEmissive", &m_cbMaterialConsts2.LayeredEmissive);
            ImGui::DragFloat("LayeredTransmission", &m_cbMaterialConsts2.LayeredTransmission);
            ImGui::DragFloat("LayeredReflectivity", &m_cbMaterialConsts2.LayeredReflectivity);
            ImGui::DragFloat("LayeredClearCoat", &m_cbMaterialConsts2.LayeredClearCoat);
            ImGui::DragFloat("DecalBlend", &m_cbMaterialConsts2.DecalBlend);
            ImGui::DragFloat("DecalBlendNormal", &m_cbMaterialConsts2.DecalBlendNormal);
            ImGui::DragFloat("DecalReflectivity", &m_cbMaterialConsts2.DecalReflectivity);
            ImGui::DragFloat("DecalRoughness", &m_cbMaterialConsts2.DecalRoughness);
            ImGui::DragFloat("DecalDiffuseWrap", &m_cbMaterialConsts2.DecalDiffuseWrap);
            ImGui::DragFloat("DecalEmissive", &m_cbMaterialConsts2.DecalEmissive);
            ImGui::DragFloat("DecalTransmission", &m_cbMaterialConsts2.DecalTransmission);
            ImGui::DragFloat("DecalClearCoat", &m_cbMaterialConsts2.DecalClearCoat);
            ImGui::DragFloat("OverlayHeightInfluence", &m_cbMaterialConsts2.OverlayHeightInfluence);
            ImGui::DragFloat("OverlayHeightMapInfluence", &m_cbMaterialConsts2.OverlayHeightMapInfluence);
            ImGui::DragFloat("OverlayMaskInfluence", &m_cbMaterialConsts2.OverlayMaskInfluence);
            ImGui::DragFloat("OverlayShift", &m_cbMaterialConsts2.OverlayShift);
            ImGui::DragFloat("OverlayColorR", &m_cbMaterialConsts2.OverlayColorR);
            ImGui::DragFloat("OverlayColorG", &m_cbMaterialConsts2.OverlayColorG);
            ImGui::DragFloat("OverlayColorB", &m_cbMaterialConsts2.OverlayColorB);
            ImGui::DragFloat("OverlayBrightness", &m_cbMaterialConsts2.OverlayBrightness);
            ImGui::DragFloat("OverlayGloss", &m_cbMaterialConsts2.OverlayGloss);
            ImGui::DragFloat("OverlayMetallic", &m_cbMaterialConsts2.OverlayMetallic);
            ImGui::DragFloat("OverlayReflectivity", &m_cbMaterialConsts2.OverlayReflectivity);
            ImGui::DragFloat("OverlayRoughness", &m_cbMaterialConsts2.OverlayRoughness);
            ImGui::DragFloat("OverlayDiffuseWrap", &m_cbMaterialConsts2.OverlayDiffuseWrap);
            ImGui::DragFloat("OverlayEmissive", &m_cbMaterialConsts2.OverlayEmissive);
            ImGui::DragFloat("OverlayTransmission", &m_cbMaterialConsts2.OverlayTransmission);
            ImGui::DragFloat("OverlayClearCoat", &m_cbMaterialConsts2.OverlayClearCoat);
            ImGui::DragFloat("DamageReflectivity", &m_cbMaterialConsts2.DamageReflectivity);
            ImGui::DragFloat("DamageRoughness", &m_cbMaterialConsts2.DamageRoughness);
            ImGui::DragFloat("DamageDiffuseWrap", &m_cbMaterialConsts2.DamageDiffuseWrap);
            ImGui::DragFloat("DamageEmissive", &m_cbMaterialConsts2.DamageEmissive);
            ImGui::DragFloat("DamageTransmission", &m_cbMaterialConsts2.DamageTransmission);
            ImGui::DragFloat("DamageHeightInfluence", &m_cbMaterialConsts2.DamageHeightInfluence);
            ImGui::DragFloat("DamageMaskInfluence", &m_cbMaterialConsts2.DamageMaskInfluence);
            ImGui::DragFloat("DamageClearCoat", &m_cbMaterialConsts2.DamageClearCoat);
        }
        ImGui::End();
    }

    // Textures
    ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");
    ImGui::Columns(3, nullptr, false);
    {
        DrawUI_Texture("Albedo1Map", 0);
        DrawUI_Texture("Gloss1Map", 1);
        DrawUI_Texture("Metallic1Map", 2);
        DrawUI_Texture("Normal1Map", 3);

        const auto flags = m_Block.m_Attributes.m_Flags;

        if (flags & USE_OVERLAY || flags & USE_LAYERED) {
            DrawUI_Texture("LayeredMaskMap", 4);
            DrawUI_Texture("LayeredHeightMap", 5);
        }

        if (flags & USE_LAYERED) {
            DrawUI_Texture("LayeredAlbedoMap", 6);
            DrawUI_Texture("LayeredGlossMap", 7);
            DrawUI_Texture("LayeredMetallicMap", 8);
            DrawUI_Texture("LayeredNormalMap", 9);
        }

        if (flags & USE_DECAL) {
            DrawUI_Texture("DecalAlbedoMap", 11);
            DrawUI_Texture("DecalGlossMap", 12);
            DrawUI_Texture("DecalMetallicMap", 13);
            DrawUI_Texture("DecalNormalMap", 14);
        }

        if (flags & USE_DAMAGE) {
            DrawUI_Texture("DamageAlbedoMap", 15);
            DrawUI_Texture("DamageGlossMap", 16);
            DrawUI_Texture("DamageMaskMap", 17);
            DrawUI_Texture("DamageMetallicMap", 18);
            DrawUI_Texture("DamageNormalMap", 19);
        }
    }
    ImGui::EndColumns();
}

void RenderBlockGeneralMkIII::SetData(vertices_t* vertices, uint16s_t* indices, materials_t* materials)
{
    using namespace jc::Vertex;

    memset(&m_Block.m_Attributes, 0, sizeof(m_Block.m_Attributes));
    memset(&m_cbMaterialConsts2, 0, sizeof(m_cbMaterialConsts2));

    m_Block.m_Version                              = jc::RenderBlocks::GENERALMKIII_VERSION;
    m_Block.m_Attributes.m_Packed.m_Scale          = 1.0f;
    m_Block.m_Attributes.m_Packed.m_UV0Extent      = {1.0f, 1.0f};
    m_Block.m_Attributes.m_EmissiveStartFadeDistSq = 2000.f;
    // m_Block.m_Attributes.m_Flags                   = BACKFACE_CULLING;

    std::vector<PackedVertexPosition> vertices_;
    std::vector<GeneralShortPacked>   data_;

    for (const auto& vertex : *vertices) {
        // vertices data
        PackedVertexPosition pos{};
        pos.x = pack<int16_t>(vertex.pos.x);
        pos.y = pack<int16_t>(vertex.pos.y);
        pos.z = pack<int16_t>(vertex.pos.z);
        vertices_.emplace_back(std::move(pos));

        // uv data
        GeneralShortPacked gsp{};
        gsp.u0 = pack<int16_t>(vertex.uv.x);
        gsp.v0 = pack<int16_t>(vertex.uv.y);
        gsp.n  = pack_normal(vertex.normal);
        // @TODO: tangent
        data_.emplace_back(std::move(gsp));
    }

    // load textures
    MakeEmptyMaterials(jc::RenderBlocks::GENERALMKIII_TEXTURES_COUNT);
    for (const auto& mat : *materials) {
        const auto& [type, filename] = mat;

        // clang-format off
        uint8_t index = 0;
        if (type == "diffuse")       index = 0;
        else if (type == "specular") index = 1;
        else if (type == "metallic") index = 2;
        else if (type == "normal")   index = 3;
        // clang-format on

        m_Textures[index]->LoadFromFile(filename);
        m_Textures[index]->SetFileName(filename.filename()); // @HACK: so we dont write full path
    }

    // create buffers
    m_VertexBuffer     = Renderer::Get()->CreateBuffer(vertices_, VERTEX_BUFFER, "GeneralMkIII VertexBuffer");
    m_VertexBufferData = Renderer::Get()->CreateBuffer(data_, VERTEX_BUFFER, "GeneralMkIII VertexBufferData");
    m_IndexBuffer      = Renderer::Get()->CreateBuffer(*indices, INDEX_BUFFER, "GeneralMkIII IndexBuffer");
}
}; // namespace jc3
