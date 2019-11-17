#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "renderblockgeneralmkiii.h"

#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/imgui/imgui_buttondropdown.h"
#include "graphics/imgui/imgui_disabled.h"
#include "graphics/renderer.h"
#include "graphics/shader_manager.h"
#include "graphics/texture.h"
#include "graphics/texture_manager.h"

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
    m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(inputDesc, 3, m_VertexShader.get(),
                                                                   "RenderBlockGeneralMkIII (packed)");

    // create the constant buffers
    m_VertexShaderConstants[0] = Renderer::Get()->CreateConstantBuffer(m_cbRBIInfo, "RenderBlockGeneralMkIII RBIInfo");
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

void RenderBlockGeneralMkIII::Read(std::istream& stream)
{
    using namespace jc::Vertex;

    // read the block attributes
    stream.read((char*)&m_Block, sizeof(m_Block));

    if (m_Block.version != jc::RenderBlocks::GENERALMKIII_VERSION) {
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

void RenderBlockGeneralMkIII::Setup(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    IRenderBlock::Setup(context);

    // setup the constant buffer
    {
        static auto world = glm::mat4(1);

        // set vertex shader constants
        m_cbRBIInfo.ModelWorldMatrix     = world;
        m_cbInstanceAttributes.UVScale   = {m_Block.attributes.packed.uv0Extent, m_Block.attributes.packed.uv1Extent};
        m_cbInstanceAttributes.DepthBias = m_Block.attributes.depthBias;
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

void RenderBlockGeneralMkIII::Draw(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

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

    ImGuiCustom::DropDownFlags(m_Block.attributes.flags, flag_labels);
}

void RenderBlockGeneralMkIII::DrawUI()
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
        IRenderBlock::DrawUI_Texture("Albedo1Map", 0);
        IRenderBlock::DrawUI_Texture("Gloss1Map", 1);
        IRenderBlock::DrawUI_Texture("Metallic1Map", 2);
        IRenderBlock::DrawUI_Texture("Normal1Map", 3);
    }
    ImGui::EndColumns();
}

void RenderBlockGeneralMkIII::SetData(vertices_t* vertices, uint16s_t* indices, materials_t* materials)
{
    using namespace jc::Vertex;

    memset(&m_Block.attributes, 0, sizeof(m_Block.attributes));
    memset(&m_cbMaterialConsts2, 0, sizeof(m_cbMaterialConsts2));

    m_Block.version                            = jc::RenderBlocks::GENERALMKIII_VERSION;
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

    // textures
    // TODO: move the std::vector from IRenderBlock. Each block has a different max amount
    // of textures it can hold which NEED to written correctly when rebuilding the RBM
    m_Textures.resize(20);

    std::vector<PackedVertexPosition> packed_vertices;
    std::vector<GeneralShortPacked>   packed_data;

    for (const auto& vertex : *vertices) {
        // vertices data
        PackedVertexPosition pos{};
        pos.x = pack<int16_t>(vertex.pos.x);
        pos.y = pack<int16_t>(vertex.pos.y);
        pos.z = pack<int16_t>(vertex.pos.z);
        packed_vertices.emplace_back(std::move(pos));

        // uv data
        GeneralShortPacked gsp{};
        gsp.u0 = pack<int16_t>(vertex.uv.x);
        gsp.v0 = pack<int16_t>(vertex.uv.y);
        gsp.n  = pack_normal(vertex.normal);

        // TODO: generate tangent

        packed_data.emplace_back(std::move(gsp));
    }

    // load texture
    if (materials->size() > 0) {
        const auto& filename = materials->at(0);
        auto&       texture  = TextureManager::Get()->GetTexture(filename.filename());
        if (texture) {
            texture->LoadFromFile(filename);
            m_Textures[0] = std::move(texture);
        }
    }

    // TEMP TEXTURE HOLDERS
    // TODO: probably not this, but we want the slots to show up in the UI
    // so we can drag & drop stuff. that only works if the texture pointer is valid
    m_Textures[1] = std::make_shared<Texture>("");
    m_Textures[2] = std::make_shared<Texture>("");
    m_Textures[3] = std::make_shared<Texture>("");

    m_VertexBuffer     = Renderer::Get()->CreateBuffer(packed_vertices.data(), packed_vertices.size(),
                                                   sizeof(PackedVertexPosition), VERTEX_BUFFER);
    m_VertexBufferData = Renderer::Get()->CreateBuffer(packed_data.data(), packed_data.size(),
                                                       sizeof(GeneralShortPacked), VERTEX_BUFFER);
    m_IndexBuffer = Renderer::Get()->CreateBuffer(indices->data(), indices->size(), sizeof(uint16_t), INDEX_BUFFER);
}
}; // namespace jc3
