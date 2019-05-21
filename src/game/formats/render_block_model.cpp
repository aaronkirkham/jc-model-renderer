#include "render_block_model.h"
#include "avalanche_archive.h"
#include "runtime_container.h"

#include "../file_loader.h"
#include "../render_block_factory.h"

#include "../jc3/renderblocks/irenderblock.h"

#include "../../graphics/camera.h"
#include "../../graphics/renderer.h"
#include "../../graphics/ui.h"
#include "../../util.h"
#include "../../window.h"

extern bool g_DrawBoundingBoxes;
extern bool g_ShowModelLabels;

std::recursive_mutex                                  Factory<RenderBlockModel>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<RenderBlockModel>> Factory<RenderBlockModel>::Instances;

RenderBlockModel::RenderBlockModel(const std::filesystem::path& filename)
    : m_Filename(filename)
{
    // find the parent archive
    // TODO: better way.
    std::lock_guard<std::recursive_mutex> _lk{AvalancheArchive::InstancesMutex};
    for (const auto& archive : AvalancheArchive::Instances) {
        if (archive.second->HasFile(filename)) {
            m_ParentArchive = archive.second;
            break;
        }
    }
}

RenderBlockModel::~RenderBlockModel()
{
    // remove from renderlist
    Renderer::Get()->RemoveFromRenderList(m_RenderBlocks);

    // delete the render blocks
    for (auto& render_block : m_RenderBlocks) {
        SAFE_DELETE(render_block);
    }
}

bool RenderBlockModel::ParseLOD(const FileBuffer& buffer)
{
    util::byte_array_buffer buf(buffer.data(), buffer.size());
    std::istream            stream(&buf);
    std::string             line;

    while (std::getline(stream, line)) {
        SPDLOG_INFO(line);
    }

    return true;
}

bool RenderBlockModel::ParseRBM(const FileBuffer& buffer, bool add_to_render_list)
{
    util::byte_array_buffer buf(buffer.data(), buffer.size());
    std::istream            stream(&buf);

    bool parse_success = true;

    // read the model header
    jc::RBMHeader header;
    stream.read((char*)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp((char*)&header.m_Magic, "RBMDL", 5) != 0) {
        SPDLOG_ERROR("Invalid header magic.");
        parse_success = false;
        goto end;
    }

    SPDLOG_INFO("RBM v{}.{}.{}. NumberOfBlocks={}, Flags={}", header.m_VersionMajor, header.m_VersionMinor,
                header.m_VersionRevision, header.m_NumberOfBlocks, header.m_Flags);

    // ensure we can read the file version (TODO: when JC4 is released, check this)
    if (header.m_VersionMajor != 1 && header.m_VersionMinor != 16) {
#ifdef DEBUG
        __debugbreak();
#endif
        return false;
    }

    m_RenderBlocks.reserve(header.m_NumberOfBlocks);

    // store the bounding box
    m_BoundingBox.m_Min = header.m_BoundingBoxMin;
    m_BoundingBox.m_Max = header.m_BoundingBoxMax;

    // focus on the bounding box
    if (RenderBlockModel::Instances.size() == 1) {
        Camera::Get()->FocusOn(&m_BoundingBox);
    }

    // use file batches
    if (!LoadingFromRuntimeContainer) {
        FileLoader::UseBatches = true;
    }

    // read all the blocks
    for (uint32_t i = 0; i < header.m_NumberOfBlocks; ++i) {
        uint32_t hash;
        stream.read((char*)&hash, sizeof(uint32_t));

        const auto render_block = (jc3::IRenderBlock*)RenderBlockFactory::CreateRenderBlock(hash);
        if (render_block) {
            // render_block->SetParent(this);
            render_block->Read(stream);
            render_block->Create();

            uint32_t checksum;
            stream.read((char*)&checksum, sizeof(uint32_t));

            // did we read the block correctly?
            if (checksum != RBM_END_OF_BLOCK) {
                SPDLOG_ERROR("Failed to read render block \"{}\" (checksum mismatch)",
                             RenderBlockFactory::GetRenderBlockName(hash));
                parse_success = false;
                break;
            }

            m_RenderBlocks.emplace_back(render_block);
        } else {
            SPDLOG_ERROR("Unknown render block \"{}\" (0x{:x})", RenderBlockFactory::GetRenderBlockName(hash), hash);

            if (LoadingFromRuntimeContainer) {
                std::stringstream error;
                error << "\"RenderBlock" << RenderBlockFactory::GetRenderBlockName(hash) << "\" (0x" << std::uppercase
                      << std::setw(4) << std::hex << hash << ") is not yet supported.\n";

                if (std::find(SuppressedWarnings.begin(), SuppressedWarnings.end(), error.str())
                    == SuppressedWarnings.end()) {
                    SuppressedWarnings.emplace_back(error.str());
                }
            } else {
                std::stringstream error;
                error << "\"RenderBlock" << RenderBlockFactory::GetRenderBlockName(hash) << "\" (0x" << std::uppercase
                      << std::setw(4) << std::hex << hash << ") is not yet supported.\n\n";
                error << "A model might still be rendered, but some parts of it may be missing.";

                Window::Get()->ShowMessageBox(error.str(), MB_ICONINFORMATION | MB_OK);
            }

            // if we haven't parsed any other blocks yet
            // we'll never see anything rendered, let the user know something is wrong
            if (m_RenderBlocks.size() == 0) {
                parse_success = false;
            }

            break;
        }
    }

end:
    if (parse_success) {}

    // if we are not loading from a runtime container, run the batches now
    if (!LoadingFromRuntimeContainer) {
        // run file batches
        FileLoader::UseBatches = false;
        FileLoader::Get()->RunFileBatches();
    }

    // add to renderlist
    if (add_to_render_list) {
        Renderer::Get()->AddToRenderList(m_RenderBlocks);
    }

    return parse_success;
}

#if 0
void RenderBlockModel::DrawGizmos()
{
    auto largest_scale = 0.0f;
    for (auto& render_block : m_RenderBlocks) {
        if (render_block->IsVisible()) {
            const auto scale = render_block->GetScale();

            // keep the biggest scale
            if (scale > largest_scale) {
                largest_scale = scale;
            }
        }
    }

    // hack to fix GetCenter
    m_BoundingBox.SetScale(largest_scale);
}
#endif

void RenderBlockModel::ReadFileCallback(const std::filesystem::path& filename, const FileBuffer& data, bool external)
{
    auto rbm = RenderBlockModel::make(filename);
    assert(rbm);

    if (filename.extension() == ".lod") {
        rbm->ParseLOD(std::move(data));
    } else if (filename.extension() == ".rbm") {
        rbm->ParseRBM(std::move(data));
    }
}

bool RenderBlockModel::SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path)
{
    const auto rbm = RenderBlockModel::get(filename.string());
    if (rbm) {
        std::ofstream stream(path, std::ios::binary);
        if (stream.fail()) {
            return false;
        }

        const auto& render_blocks = rbm->GetRenderBlocks();

        // generate the rbm header
        jc::RBMHeader header;
        header.m_BoundingBoxMin = rbm->GetBoundingBox()->GetMin();
        header.m_BoundingBoxMax = rbm->GetBoundingBox()->GetMax();
        header.m_NumberOfBlocks = render_blocks.size();
        header.m_Flags          = 8;

        // write the header
        stream.write((char*)&header, sizeof(header));

        for (const auto& render_block : render_blocks) {
            // write the block type hash
            const auto& type_hash = render_block->GetTypeHash();
            stream.write((char*)&type_hash, sizeof(type_hash));

            // write the block data
            ((jc3::IRenderBlock*)render_block)->Write(stream);

            // write the end of block checksum
            stream.write((char*)&RBM_END_OF_BLOCK, sizeof(RBM_END_OF_BLOCK));
        }

        stream.close();
        return true;
    }

    return false;
}

void RenderBlockModel::ContextMenuUI(const std::filesystem::path& filename)
{
    const auto rbm = get(filename.string());

    if (ImGui::BeginMenu("Add Render Block")) {
        // show available render blocks
        for (const auto& block_name : RenderBlockFactory::GetValidRenderBlocks()) {
            if (ImGui::MenuItem(block_name)) {
                const auto render_block = RenderBlockFactory::CreateRenderBlock(block_name);
                assert(render_block);
                rbm->GetRenderBlocks().emplace_back(render_block);
            }
        }

        ImGui::EndMenu();
    }
}

void RenderBlockModel::Load(const std::filesystem::path& filename)
{
    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
        if (success) {
            RenderBlockModel::ReadFileCallback(filename, std::move(data), false);
        } else {
            SPDLOG_ERROR("Failed to read model \"{}\"", filename.string());
        }
    });
}

void RenderBlockModel::LoadFromRuntimeContainer(const std::filesystem::path&      filename,
                                                std::shared_ptr<RuntimeContainer> rc)
{
    assert(rc);

    auto path = filename.parent_path().string();
    path      = path.replace(path.find("editor/entities"), strlen("editor/entities"), "models");

    std::thread([&, rc, path] {
        FileLoader::UseBatches                        = true;
        RenderBlockModel::LoadingFromRuntimeContainer = true;

        for (const auto& container : rc->GetAllContainers("CPartProp")) {
            auto name     = container->GetProperty("name");
            auto filename = std::any_cast<std::string>(name->GetValue());
            filename += "_lod1.rbm";

            std::filesystem::path modelname = path;
            modelname /= filename;

            RenderBlockModel::Load(modelname.generic_string());
        }

        RenderBlockModel::LoadingFromRuntimeContainer = false;
        FileLoader::UseBatches                        = false;
        FileLoader::Get()->RunFileBatches();

        std::stringstream error;
        for (const auto& warning : RenderBlockModel::SuppressedWarnings) {
            error << warning;
        }

        error << "\nA model might still be rendered, but some parts of it may be missing.";
        Window::Get()->ShowMessageBox(error.str(), MB_ICONINFORMATION | MB_OK);
    }).detach();
}
