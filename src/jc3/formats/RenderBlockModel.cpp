#include <jc3/formats/RenderBlockModel.h>
#include <jc3/formats/AvalancheArchive.h>
#include <jc3/formats/RuntimeContainer.h>
#include <jc3/RenderBlockFactory.h>
#include <jc3/FileLoader.h>

#include <Window.h>

#include <imgui.h>
#include <graphics/Renderer.h>
#include <graphics/Camera.h>
#include <graphics/DebugRenderer.h>
#include <graphics/UI.h>
#include <graphics/imgui/fonts/fontawesome_icons.h>

#include <Input.h>

#include <any>

extern bool g_DrawBoundingBoxes;
extern bool g_ShowModelLabels;

std::recursive_mutex Factory<RenderBlockModel>::InstancesMutex;
std::map<uint32_t, std::shared_ptr<RenderBlockModel>> Factory<RenderBlockModel>::Instances;

RenderBlockModel::RenderBlockModel(const fs::path& filename)
    : m_Filename(filename)
{
    // find the parent archive
    // TODO: better way.
    std::lock_guard<std::recursive_mutex> _lk{ AvalancheArchive::InstancesMutex };
    for (const auto& archive : AvalancheArchive::Instances) {
        if (archive.second->HasFile(filename)) {
            m_ParentArchive = archive.second;
        }
    }

    if (!m_ParentArchive) {
        DEBUG_LOG("didn't find a parent :(");
    }
}

RenderBlockModel::~RenderBlockModel()
{
    DEBUG_LOG("RenderBlockModel::~RenderBlockModel");

    for (auto& render_block : m_RenderBlocks) {
        SAFE_DELETE(render_block);
    }
}

bool RenderBlockModel::Parse(const FileBuffer& data)
{
    std::istringstream stream(std::string{ (char*)data.data(), data.size() });

    bool parse_success = true;
        
    // read the model header
    JustCause3::RBMHeader header;
    stream.read((char *)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp((char *)&header.m_Magic, "RBMDL", 5) != 0) {
        DEBUG_LOG("RenderBlockModel::Parse - Invalid file header. Input file probably isn't a RenderBlockModel file.");

        parse_success = false;
        goto end;
    }

    DEBUG_LOG("RenderBlockModel v" << header.m_VersionMajor << "." << header.m_VersionMinor << "." << header.m_VersionRevision);
    DEBUG_LOG(" - m_NumberOfBlocks=" << header.m_NumberOfBlocks);

    m_RenderBlocks.reserve(header.m_NumberOfBlocks);

    // store the bounding box
    m_BoundingBox.m_Min = header.m_BoundingBoxMin;
    m_BoundingBox.m_Max = header.m_BoundingBoxMax;

    // focus on the bounding box
    if (RenderBlockModel::Instances.size() == 1) {
        Camera::Get()->FocusOn(this);
    }

    // use file batches
    if (!LoadingFromRC) FileLoader::UseBatches = true;

    // read all the blocks
    for (uint32_t i = 0; i < header.m_NumberOfBlocks; ++i) {
        uint32_t hash;
        stream.read((char *)&hash, sizeof(uint32_t));

        const auto render_block = RenderBlockFactory::CreateRenderBlock(hash);
        if (render_block) {
            render_block->Read(stream);
            render_block->Create();

            uint32_t checksum;
            stream.read((char *)&checksum, sizeof(uint32_t));

            // did we read the block correctly?
            if (checksum != 0x89ABCDEF) {
                DEBUG_LOG("RenderBlockModel::Parse - Failed to read Render Block");

                parse_success = false;
                break;
            }

            m_RenderBlocks.emplace_back(render_block);
        }
        else {
            DEBUG_LOG("[WARNING] RenderBlockModel::Parse - Unknown render block. \"" << RenderBlockFactory::GetRenderBlockName(hash) << "\" - " << std::setw(4) << std::hex << hash);

            if (LoadingFromRC) {
                std::stringstream error;
                error << "\"RenderBlock" << RenderBlockFactory::GetRenderBlockName(hash) << "\" (0x" << std::uppercase << std::setw(4) << std::hex << hash << ") is not yet supported.\n";

                if (std::find(SuppressedWarnings.begin(), SuppressedWarnings.end(), error.str()) == SuppressedWarnings.end()) {
                    SuppressedWarnings.emplace_back(error.str());
                }
            }
            else {
                std::stringstream error;
                error << "\"RenderBlock" << RenderBlockFactory::GetRenderBlockName(hash) << "\" (0x" << std::uppercase << std::setw(4) << std::hex << hash << ") is not yet supported.\n\n";
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
    if (parse_success) {
    }

    // if we are not loading from a runtime container, run the batches now
    if (!LoadingFromRC) {
        // run file batches
        FileLoader::UseBatches = false;
        FileLoader::Get()->RunFileBatches();
    }

    return parse_success;
}

void RenderBlockModel::Draw(RenderContext_t* context)
{
    auto largest_scale = 0.0f;

    // draw all render blocks
    for (auto& render_block : m_RenderBlocks) {
        render_block->Setup(context);
        render_block->Draw(context);

        // reset
        context->m_Renderer->SetDefaultRenderStates();

        //
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

    // draw filename label
    if (g_ShowModelLabels) {
        static auto white = glm::vec4{ 1, 1, 1, 0.6 };
        DebugRenderer::Get()->DrawText(m_Filename.filename().string(), m_BoundingBox.GetCenter(), white, true);
    }

    // draw bounding boxes
    if (g_DrawBoundingBoxes) {
        DebugRenderer::Get()->DrawBBox(m_BoundingBox.m_Min * largest_scale, m_BoundingBox.m_Max * largest_scale, { 1, 0, 0, 1 });
    }
}

void RenderBlockModel::ReadFileCallback(const fs::path& filename, const FileBuffer& data)
{
    auto rbm = RenderBlockModel::make(filename);
    assert(rbm);
    rbm->Parse(data);
}

void RenderBlockModel::Load(const fs::path& filename)
{
    FileLoader::Get()->ReadFile(filename, [&, filename](bool success, FileBuffer data) {
        if (success) {
            auto rbm = RenderBlockModel::make(filename);
            assert(rbm);
            rbm->Parse(data);
        }
        else {
            DEBUG_LOG("RenderBlockModel::Load - Failed to read model \"" << filename << "\".");
        }
    });
}

void RenderBlockModel::LoadFromRuntimeContainer(const fs::path& filename, std::shared_ptr<RuntimeContainer> rc)
{
    assert(rc);

    auto path = filename.parent_path().string();
    path = path.replace(path.find("editor\\entities"), strlen("editor\\entities"), "models");

    std::thread([&, rc, path] {
        FileLoader::UseBatches = true;
        RenderBlockModel::LoadingFromRC = true;

        for (const auto& container : rc->GetAllContainers("CPartProp")) {
            auto name = container->GetProperty("name");
            auto filename = std::any_cast<std::string>(name->GetValue());
            filename += "_lod1.rbm";

            fs::path modelname = path;
            modelname /= filename;

            DEBUG_LOG(modelname);
            RenderBlockModel::Load(modelname);
        }

        RenderBlockModel::LoadingFromRC = false;
        FileLoader::UseBatches = false;
        FileLoader::Get()->RunFileBatches();

        std::stringstream error;
        for (const auto& warning : RenderBlockModel::SuppressedWarnings) {
            error << warning;
        }

        error << "\nA model might still be rendered, but some parts of it may be missing.";
        Window::Get()->ShowMessageBox(error.str(), MB_ICONINFORMATION | MB_OK);
    }).detach();
}