#include "pch.h"

#include "avalanche_model_format.h"

#include "app/app.h"
#include "app/profile.h"

#include "game/game.h"
#include "game/games/justcause4/render_block.h"
#include "game/resource_manager.h"

#include "render/imguiex.h"
#include "render/renderer.h"
#include "render/ui.h"

#include <AvaFormatLib/util/byte_vector_stream.h>
#include <imgui.h>

DXGI_FORMAT format_lookup_table[] = {
    // 8-BIT SNORM
    DXGI_FORMAT_R8_SNORM,
    DXGI_FORMAT_R8G8_SNORM,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_R8G8B8A8_SNORM,
    // 8-BIT SINT
    DXGI_FORMAT_R8_SINT,
    DXGI_FORMAT_R8G8_SINT,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_R8G8B8A8_SINT,
    // 8-BIT UNORM
    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_R8G8_UNORM,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    // 8-BIT UINT
    DXGI_FORMAT_R8_UINT,
    DXGI_FORMAT_R8G8_UINT,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_R8G8B8A8_UINT,
    // 16-BIT SNORM
    DXGI_FORMAT_R16_SNORM,
    DXGI_FORMAT_R16G16_SNORM,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_R16G16B16A16_SNORM,
    // 16-BIT SINT
    DXGI_FORMAT_R16_SINT,
    DXGI_FORMAT_R16G16_SINT,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_R16G16B16A16_SINT,
    // 16-BIT UNORM
    DXGI_FORMAT_R16_UNORM,
    DXGI_FORMAT_R16G16_UNORM,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_R16G16B16A16_UNORM,
    // 16-BIT UINT
    DXGI_FORMAT_R16_UINT,
    DXGI_FORMAT_R16G16_UINT,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_R16G16B16A16_UINT,
    // 32-BIT SINT
    DXGI_FORMAT_R32_SINT,
    DXGI_FORMAT_R32G32_SINT,
    DXGI_FORMAT_R32G32B32_SINT,
    DXGI_FORMAT_R32G32B32A32_SINT,
    // 32-BIT UINT
    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32G32_UINT,
    DXGI_FORMAT_R32G32B32_UINT,
    DXGI_FORMAT_R32G32B32A32_UINT,
    // 16-BIT FLOAT
    DXGI_FORMAT_R16_FLOAT,
    DXGI_FORMAT_R16G16_FLOAT,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    // 32-BIT FLOAT
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R32G32_FLOAT,
    DXGI_FORMAT_R32G32B32_FLOAT,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
};

namespace jcmr::game::format
{
struct AmfModelInstance {
#define SAFE_DELETE(ptr)                                                                                               \
    if (ptr) delete ptr;

#define SAFE_FREE(ptr)                                                                                                 \
    if (ptr) std::free(ptr);

    AmfModelInstance(App& app, ava::AvalancheDataFormat::ADF* model_adf, ava::AvalancheModelFormat::SAmfModel* model)
        : m_app(app)
        , m_model_adf(model_adf)
        , m_model(model)
    {
        load_meshc();
        load_hrmeshc();
        create_render_blocks();
    }

    ~AmfModelInstance()
    {
        LOG_INFO("~AmfModelInstance");

        SAFE_DELETE(m_model_adf);
        SAFE_FREE(m_model);

        SAFE_DELETE(m_mesh_adf);
        SAFE_FREE(m_mesh_header);
        SAFE_FREE(m_mesh_low_buffers);

        SAFE_DELETE(m_mesh_high_adf);
        SAFE_FREE(m_mesh_high_buffers);
    }

  private:
    bool load_meshc()
    {
        auto* resource_manager = m_app.get_game().get_resource_manager();

        auto* mesh_filename = m_model_adf->HashLookup(m_model->m_Mesh);
        LOG_INFO("AvalancheModelFormat : loading meshc \"{}\" {:x}...", mesh_filename, m_model->m_Mesh);

        // read .meshc
        ByteArray mesh_buffer;
        if (!resource_manager->read(mesh_filename, &mesh_buffer)) {
            LOG_ERROR("AvalancheModelFormat : failed to read model mesh \"{}\"", mesh_filename);
            return false;
        }

        // parse .meshc
        AVA_FL_ENSURE(
            ava::AvalancheModelFormat::ParseMeshc(mesh_buffer, &m_mesh_adf, &m_mesh_header, &m_mesh_low_buffers),
            false);

        //
        {
            static auto usage_to_string = [](ava::AvalancheModelFormat::EAmfUsage usage) {
                using namespace ava::AvalancheModelFormat;
                switch (usage) {
                    case AmfUsage_Unspecified: return "AmfUsage_Unspecified";
                    case AmfUsage_Position: return "AmfUsage_Position";
                    case AmfUsage_TextureCoordinate: return "AmfUsage_TextureCoordinate";
                    case AmfUsage_Normal: return "AmfUsage_Normal";
                    case AmfUsage_BiTangent: return "AmfUsage_BiTangent";
                    case AmfUsage_TangentSpace: return "AmfUsage_TangentSpace";
                    case AmfUsage_BoneIndex: return "AmfUsage_BoneIndex";
                    case AmfUsage_BoneWeight: return "AmfUsage_BoneWeight";
                    case AmfUsage_Color: return "AmfUsage_Color";
                    case AmfUsage_WireRadius: return "AmfUsage_WireRadius";
                    default: return "Unknown";
                }
            };

            static auto format_to_string = [](ava::AvalancheModelFormat::EAmfFormat format) {
                using namespace ava::AvalancheModelFormat;
                switch (format) {
                    case AmfFormat_R32G32B32A32_FLOAT: return "AmfFormat_R32G32B32A32_FLOAT";
                    case AmfFormat_R32G32B32A32_UINT: return "AmfFormat_R32G32B32A32_UINT";
                    case AmfFormat_R32G32B32A32_SINT: return "AmfFormat_R32G32B32A32_SINT";
                    case AmfFormat_R32G32B32_FLOAT: return "AmfFormat_R32G32B32_FLOAT";
                    case AmfFormat_R32G32B32_UINT: return "AmfFormat_R32G32B32_UINT";
                    case AmfFormat_R32G32B32_SINT: return "AmfFormat_R32G32B32_SINT";
                    case AmfFormat_R16G16B16A16_FLOAT: return "AmfFormat_R16G16B16A16_FLOAT";
                    case AmfFormat_R16G16B16A16_UNORM: return "AmfFormat_R16G16B16A16_UNORM";
                    case AmfFormat_R16G16B16A16_UINT: return "AmfFormat_R16G16B16A16_UINT";
                    case AmfFormat_R16G16B16A16_SNORM: return "AmfFormat_R16G16B16A16_SNORM";
                    case AmfFormat_R16G16B16A16_SINT: return "AmfFormat_R16G16B16A16_SINT";
                    case AmfFormat_R16G16B16_FLOAT: return "AmfFormat_R16G16B16_FLOAT";
                    case AmfFormat_R16G16B16_UNORM: return "AmfFormat_R16G16B16_UNORM";
                    case AmfFormat_R16G16B16_UINT: return "AmfFormat_R16G16B16_UINT";
                    case AmfFormat_R16G16B16_SNORM: return "AmfFormat_R16G16B16_SNORM";
                    case AmfFormat_R16G16B16_SINT: return "AmfFormat_R16G16B16_SINT";
                    case AmfFormat_R32G32_FLOAT: return "AmfFormat_R32G32_FLOAT";
                    case AmfFormat_R32G32_UINT: return "AmfFormat_R32G32_UINT";
                    case AmfFormat_R32G32_SINT: return "AmfFormat_R32G32_SINT";
                    case AmfFormat_R10G10B10A2_UNORM: return "AmfFormat_R10G10B10A2_UNORM";
                    case AmfFormat_R10G10B10A2_UINT: return "AmfFormat_R10G10B10A2_UINT";
                    case AmfFormat_R11G11B10_FLOAT: return "AmfFormat_R11G11B10_FLOAT";
                    case AmfFormat_R8G8B8A8_UNORM: return "AmfFormat_R8G8B8A8_UNORM";
                    case AmfFormat_R8G8B8A8_UNORM_SRGB: return "AmfFormat_R8G8B8A8_UNORM_SRGB";
                    case AmfFormat_R8G8B8A8_UINT: return "AmfFormat_R8G8B8A8_UINT";
                    case AmfFormat_R8G8B8A8_SNORM: return "AmfFormat_R8G8B8A8_SNORM";
                    case AmfFormat_R8G8B8A8_SINT: return "AmfFormat_R8G8B8A8_SINT";
                    case AmfFormat_R16G16_FLOAT: return "AmfFormat_R16G16_FLOAT";
                    case AmfFormat_R16G16_UNORM: return "AmfFormat_R16G16_UNORM";
                    case AmfFormat_R16G16_UINT: return "AmfFormat_R16G16_UINT";
                    case AmfFormat_R16G16_SNORM: return "AmfFormat_R16G16_SNORM";
                    case AmfFormat_R16G16_SINT: return "AmfFormat_R16G16_SINT";
                    case AmfFormat_R32_FLOAT: return "AmfFormat_R32_FLOAT";
                    case AmfFormat_R32_UINT: return "AmfFormat_R32_UINT";
                    case AmfFormat_R32_SINT: return "AmfFormat_R32_SINT";
                    case AmfFormat_R8G8_UNORM: return "AmfFormat_R8G8_UNORM";
                    case AmfFormat_R8G8_UINT: return "AmfFormat_R8G8_UINT";
                    case AmfFormat_R8G8_SNORM: return "AmfFormat_R8G8_SNORM";
                    case AmfFormat_R8G8_SINT: return "AmfFormat_R8G8_SINT";
                    case AmfFormat_R16_FLOAT: return "AmfFormat_R16_FLOAT";
                    case AmfFormat_R16_UNORM: return "AmfFormat_R16_UNORM";
                    case AmfFormat_R16_UINT: return "AmfFormat_R16_UINT";
                    case AmfFormat_R16_SNORM: return "AmfFormat_R16_SNORM";
                    case AmfFormat_R16_SINT: return "AmfFormat_R16_SINT";
                    case AmfFormat_R8_UNORM: return "AmfFormat_R8_UNORM";
                    case AmfFormat_R8_UINT: return "AmfFormat_R8_UINT";
                    case AmfFormat_R8_SNORM: return "AmfFormat_R8_SNORM";
                    case AmfFormat_R8_SINT: return "AmfFormat_R8_SINT";
                    case AmfFormat_R32_UNIT_VEC_AS_FLOAT: return "AmfFormat_R32_UNIT_VEC_AS_FLOAT";
                    case AmfFormat_R32_R8G8B8A8_UNORM_AS_FLOAT: return "AmfFormat_R32_R8G8B8A8_UNORM_AS_FLOAT";
                    case AmfFormat_R8G8B8A8_TANGENT_SPACE: return "AmfFormat_R8G8B8A8_TANGENT_SPACE";
                    default: return "Unknown";
                }
            };

            static auto AmfFormatToDXGI = [](ava::AvalancheModelFormat::EAmfFormat format) -> DXGI_FORMAT {
                using namespace ava::AvalancheModelFormat;
                switch (format) {
                    case AmfFormat_R32G32B32A32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
                    case AmfFormat_R32G32B32A32_UINT: return DXGI_FORMAT_R32G32B32A32_UINT;
                    case AmfFormat_R32G32B32A32_SINT: return DXGI_FORMAT_R32G32B32A32_SINT;
                    case AmfFormat_R32G32B32_FLOAT: return DXGI_FORMAT_R32G32B32_FLOAT;
                    case AmfFormat_R32G32B32_UINT: return DXGI_FORMAT_R32G32B32_UINT;
                    case AmfFormat_R32G32B32_SINT: return DXGI_FORMAT_R32G32B32_SINT;
                    case AmfFormat_R16G16B16A16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
                    case AmfFormat_R16G16B16A16_UNORM: return DXGI_FORMAT_R16G16B16A16_UNORM;
                    case AmfFormat_R16G16B16A16_UINT: return DXGI_FORMAT_R16G16B16A16_UINT;
                    case AmfFormat_R16G16B16A16_SNORM: return DXGI_FORMAT_R16G16B16A16_SNORM;
                    case AmfFormat_R16G16B16A16_SINT:
                        return DXGI_FORMAT_R16G16B16A16_SINT;
                        // case AmfFormat_R16G16B16_FLOAT: return "AmfFormat_R16G16B16_FLOAT";
                        // case AmfFormat_R16G16B16_UNORM: return "AmfFormat_R16G16B16_UNORM";
                        // case AmfFormat_R16G16B16_UINT: return "AmfFormat_R16G16B16_UINT";
                        // case AmfFormat_R16G16B16_SNORM: return "AmfFormat_R16G16B16_SNORM";
                        // case AmfFormat_R16G16B16_SINT: return "AmfFormat_R16G16B16_SINT";
                    case AmfFormat_R32G32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
                    case AmfFormat_R32G32_UINT: return DXGI_FORMAT_R32G32_UINT;
                    case AmfFormat_R32G32_SINT: return DXGI_FORMAT_R32G32_SINT;
                    case AmfFormat_R10G10B10A2_UNORM: return DXGI_FORMAT_R10G10B10A2_UNORM;
                    case AmfFormat_R10G10B10A2_UINT: return DXGI_FORMAT_R10G10B10A2_UINT;
                    case AmfFormat_R11G11B10_FLOAT: return DXGI_FORMAT_R11G11B10_FLOAT;
                    case AmfFormat_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
                    case AmfFormat_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                    case AmfFormat_R8G8B8A8_UINT: return DXGI_FORMAT_R8G8B8A8_UINT;
                    case AmfFormat_R8G8B8A8_SNORM: return DXGI_FORMAT_R8G8B8A8_SNORM;
                    case AmfFormat_R8G8B8A8_SINT: return DXGI_FORMAT_R8G8B8A8_SINT;
                    case AmfFormat_R16G16_FLOAT: return DXGI_FORMAT_R16G16_FLOAT;
                    case AmfFormat_R16G16_UNORM: return DXGI_FORMAT_R16G16_UNORM;
                    case AmfFormat_R16G16_UINT: return DXGI_FORMAT_R16G16_UINT;
                    case AmfFormat_R16G16_SNORM: return DXGI_FORMAT_R16G16_SNORM;
                    case AmfFormat_R16G16_SINT: return DXGI_FORMAT_R16G16_SINT;
                    case AmfFormat_R32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
                    case AmfFormat_R32_UINT: return DXGI_FORMAT_R32_UINT;
                    case AmfFormat_R32_SINT: return DXGI_FORMAT_R32_SINT;
                    case AmfFormat_R8G8_UNORM: return DXGI_FORMAT_R8G8_UNORM;
                    case AmfFormat_R8G8_UINT: return DXGI_FORMAT_R8G8_UINT;
                    case AmfFormat_R8G8_SNORM: return DXGI_FORMAT_R8G8_SNORM;
                    case AmfFormat_R8G8_SINT: return DXGI_FORMAT_R8G8_SINT;
                    case AmfFormat_R16_FLOAT: return DXGI_FORMAT_R16_FLOAT;
                    case AmfFormat_R16_UNORM: return DXGI_FORMAT_R16_UNORM;
                    case AmfFormat_R16_UINT: return DXGI_FORMAT_R16_UINT;
                    case AmfFormat_R16_SNORM: return DXGI_FORMAT_R16_SNORM;
                    case AmfFormat_R16_SINT: return DXGI_FORMAT_R16_SINT;
                    case AmfFormat_R8_UNORM: return DXGI_FORMAT_R8_UNORM;
                    case AmfFormat_R8_UINT: return DXGI_FORMAT_R8_UINT;
                    case AmfFormat_R8_SNORM: return DXGI_FORMAT_R8_SNORM;
                    case AmfFormat_R8_SINT: return DXGI_FORMAT_R8_SINT;
                    default:
                        return DXGI_FORMAT_UNKNOWN;

                        // case AmfFormat_R32_UNIT_VEC_AS_FLOAT: return "AmfFormat_R32_UNIT_VEC_AS_FLOAT";
                        // case AmfFormat_R32_R8G8B8A8_UNORM_AS_FLOAT: return "AmfFormat_R32_R8G8B8A8_UNORM_AS_FLOAT";
                        // case AmfFormat_R8G8B8A8_TANGENT_SPACE: return "AmfFormat_R8G8B8A8_TANGENT_SPACE";
                }
            };

            auto& hi_lod     = m_mesh_header->m_LodGroups[0];
            u8    mesh_index = 0;
            for (auto& mesh : hi_lod.m_Meshes) {
                u8 attr_index = 0;
                for (auto& attr : mesh.m_StreamAttributes) {
                    const auto packing_data   = *(float*)&attr.m_PackingData;
                    const auto packing_data_2 = *(float*)&attr.m_PackingData[4];

                    LOG_INFO(" - AmfMesh({}) {} - usage={}, format={}, stream_index={}, "
                             "stream_offset={}, stream_stride={}, packing_data={} {}",
                             mesh_index, attr_index, usage_to_string(attr.m_Usage), format_to_string(attr.m_Format),
                             attr.m_StreamIndex, attr.m_StreamOffset, attr.m_StreamStride, packing_data,
                             packing_data_2);

                    attr.m_Usage;
                    ++attr_index;
                }

                LOG_INFO("");
                ++mesh_index;
            }
        }
        return true;
    }

    bool load_hrmeshc()
    {
        auto* resource_manager = m_app.get_game().get_resource_manager();

        auto* hrmesh_filename = m_mesh_adf->HashLookup(m_mesh_header->m_HighLodPath);
        LOG_INFO("AvalancheModelFormat : loading hrmeshc \"{}\" {:x}...", hrmesh_filename,
                 m_mesh_header->m_HighLodPath);

        // read hrmeshc
        ByteArray hrmesh_buffer;
        if (!resource_manager->read(hrmesh_filename, &hrmesh_buffer)) {
            LOG_ERROR("AvalancheModelFormat : failed to read model mesh \"{}\"", hrmesh_filename);
            return false;
        }

        // parse .hrmeshc
        AVA_FL_ENSURE(ava::AvalancheModelFormat::ParseHrmeshc(hrmesh_buffer, &m_mesh_high_adf, &m_mesh_high_buffers),
                      false);
        return true;
    }

    void create_render_blocks()
    {
        auto& game = m_app.get_game();

        ProfileBlock _("AvalancheModelFormat : create render blocks");

        for (auto& material : m_model->m_Materials) {
            auto* render_block = (game::justcause4::RenderBlock*)game.create_render_block(material.m_RenderBlockId);
            if (render_block) {
                render_block->set_resource_manager(game.get_resource_manager());
                render_block->read(material.m_Attributes);
                render_blocks.emplace_back(std::move(render_block));
            } else {
                LOG_WARNING("AvalancheModelFormat : failed to create render block with type {:x}",
                            material.m_RenderBlockId);
            }
        }
    }

  public:
    std::vector<game::justcause4::RenderBlock*> render_blocks;

  private:
    App&                                  m_app;
    ava::AvalancheDataFormat::ADF*        m_model_adf = nullptr;
    ava::AvalancheModelFormat::SAmfModel* m_model     = nullptr;

    // mesh
    ava::AvalancheDataFormat::ADF*              m_mesh_adf         = nullptr;
    ava::AvalancheModelFormat::SAmfMeshHeader*  m_mesh_header      = nullptr;
    ava::AvalancheModelFormat::SAmfMeshBuffers* m_mesh_low_buffers = nullptr;

    // hrmesh
    ava::AvalancheDataFormat::ADF*              m_mesh_high_adf     = nullptr;
    ava::AvalancheModelFormat::SAmfMeshBuffers* m_mesh_high_buffers = nullptr;
};

struct AvalancheModelFormatImpl final : AvalancheModelFormat {
    AvalancheModelFormatImpl(App& app)
        : m_app(app)
    {
#if 0
        app.get_ui().on_render([this](RenderContext& context) {
            std::string _render_block_to_unload;
            for (auto& render_block : m_render_blocks) {
                bool open = true;
                ImGui::SetNextWindowSize({800, 600}, ImGuiCond_Appearing);
                if (ImGui::Begin(render_block.first.c_str(), &open,
                                 (ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings))) {
                    // menu bar
                    if (ImGui::BeginMenuBar()) {
                        if (ImGui::BeginMenu("File")) {
                            // save file
                            if (ImGui::Selectable(ICON_FA_SAVE " Save to...")) {
                                std::filesystem::path path;
                                if (os::get_open_folder(&path, os::FileDialogParams{})) {
                                    m_app.save_file(this, render_block.first, path);
                                }
                            }

                            // save to dropzone
                            if (ImGui::Selectable(ICON_FA_SAVE " Save to dropzone")) {
                                m_app.save_file(this, render_block.first, "dropzone");
                            }

                            // close
                            ImGui::Separator();
                            if (ImGui::Selectable(ICON_FA_WINDOW_CLOSE " Close")) {
                                open = false;
                            }

                            ImGui::EndMenu();
                        }

                        ImGui::EndMenuBar();
                    }

                    // draw render blocks
                    for (auto* block : render_block.second) {
                        ImGui::PushID(block);

                        if (ImGui::CollapsingHeader(block->get_type_name())) {
                            ImGui::Indent();

                            // TODO : use ImGuiEx table stuff
                            if (ImGui::BeginTable("##layout", 2, ImGuiTableFlags_NoSavedSettings)) {
                                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed,
                                                        ImGui::GetContentRegionAvail().x * kLabelColumnWidth);
                                ImGui::TableSetupColumn("Widget", ImGuiTableColumnFlags_WidthStretch);

                                block->draw_ui();
                            }

                            ImGui::EndTable();
                            ImGui::Unindent();
                        }

                        ImGui::PopID();
                    }
                }

                if (!open) {
                    _render_block_to_unload = render_block.first;
                }

                ImGui::End();
            }

            if (!_render_block_to_unload.empty()) {
                unload(_render_block_to_unload);
                _render_block_to_unload.clear();
            }
        });
#endif
    }

    bool load(const std::string& filename) override
    {
        if (is_loaded(filename)) {
            return true;
        }

        LOG_INFO("AvalancheModelFormat : loading \"{}\"...", filename);

        auto* resource_manager = m_app.get_game().get_resource_manager();

        ByteArray buffer;
        if (!resource_manager->read(filename, &buffer)) {
            LOG_ERROR("AvalancheModelFormat : failed to load file.");
            return false;
        }

        ava::AvalancheDataFormat::ADF*        adf;
        ava::AvalancheModelFormat::SAmfModel* model;
        AVA_FL_ENSURE(ava::AvalancheModelFormat::ParseModelc(buffer, &adf, &model), false);

        m_models.insert({filename, std::make_unique<AmfModelInstance>(m_app, adf, model)});
        return true;
    }

    void unload(const std::string& filename) override
    {
        auto iter = m_models.find(filename);
        if (iter == m_models.end()) return;

#if 0
        auto& render_blocks = (*iter).second;
        m_app.get_renderer().remove_from_renderlist(render_blocks);

        for (auto* block : render_blocks) {
            delete block;
        }
#endif

        m_models.erase(iter);
    }

    bool save(const std::string& filename, ByteArray* out_buffer) override
    {
        //
        return false;
    }

    bool is_loaded(const std::string& filename) const override
    {
        return m_models.find(filename) != m_models.end();
    }

  private:
    App& m_app;

    std::unordered_map<std::string, std::unique_ptr<AmfModelInstance>> m_models;
};

AvalancheModelFormat* AvalancheModelFormat::create(App& app)
{
    return new AvalancheModelFormatImpl(app);
}

void AvalancheModelFormat::destroy(AvalancheModelFormat* instance)
{
    delete instance;
}
} // namespace jcmr::game::format