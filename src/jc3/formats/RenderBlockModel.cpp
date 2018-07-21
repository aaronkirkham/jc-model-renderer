#include <jc3/formats/RenderBlockModel.h>
#include <jc3/RenderBlockFactory.h>
#include <Window.h>
#include <graphics/Renderer.h>
#include <graphics/Camera.h>
#include <graphics/DebugRenderer.h>
#include <graphics/UI.h>
#include <jc3/formats/AvalancheArchive.h>
#include <fonts/fontawesome_icons.h>
#include <Input.h>
#include <imgui.h>

#include <jc3/ModelManager.h>
#include <jc3/FileLoader.h>

extern bool g_DrawBoundingBoxes;
extern bool g_ShowModelLabels;

extern AvalancheArchive* g_CurrentLoadedArchive;
static auto g_RenderBlockColour = glm::vec4{ 1, 1, 1, 1 };

/*RenderBlockModel::RenderBlockModel(const fs::path& file)
    : m_File(file)
{
    // make sure this is an rbm file
    if (file.extension() != ".rbm") {
        throw std::invalid_argument("RenderBlockModel type only supports .rbm files!");
    }

    // try and open the file
    std::ifstream stream(m_File, std::ios::binary);
    if (stream.fail()) {
        throw std::runtime_error("Failed to read RenderBlockModel file!");
    }

    // create the mesh constant buffer
    MeshConstants constants;
    m_MeshConstants = Renderer::Get()->CreateConstantBuffer(constants);

    //ParseRenderBlockModel(stream);
}*/

RenderBlockModel::RenderBlockModel(const fs::path& filename)
    : m_Filename(filename)
{
}

RenderBlockModel::~RenderBlockModel()
{
    DEBUG_LOG("RenderBlockModel::~RenderBlockModel");

    ModelManager::Get()->RemoveModel(this, g_CurrentLoadedArchive);

    for (auto& render_block : m_RenderBlocks) {
        safe_delete(render_block);
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
        m_ReadBlocksError = "Invalid file header. Input file probably isn't a RenderBlockModel file.";
        DEBUG_LOG("[ERROR] " << m_ReadBlocksError);

        parse_success = false;
        goto end;
    }

    DEBUG_LOG("RenderBlockModel v" << header.m_VersionMajor << "." << header.m_VersionMinor << "." << header.m_VersionRevision);
    DEBUG_LOG(" - m_NumberOfBlocks=" << header.m_NumberOfBlocks);

    m_RenderBlocks.reserve(header.m_NumberOfBlocks);

    // store the bounding box
    m_BoundingBoxMin = header.m_BoundingBoxMin;
    m_BoundingBoxMax = header.m_BoundingBoxMax;

    // use file batches
    FileLoader::Get()->m_UseBatches = true;

    // read all the blocks
    for (uint32_t i = 0; i < header.m_NumberOfBlocks; ++i) {
        uint32_t hash;
        stream.read((char *)&hash, sizeof(uint32_t));

        const auto renderBlock = RenderBlockFactory::CreateRenderBlock(hash);
        if (renderBlock) {
            renderBlock->Read(stream);
            renderBlock->Create();

            uint32_t checksum;
            stream.read((char *)&checksum, sizeof(uint32_t));

            // did we read the block correctly?
            if (checksum != 0x89ABCDEF) {
                m_ReadBlocksError = "Failed to read Render Block.";
                DEBUG_LOG("[ERROR] " << m_ReadBlocksError);

                parse_success = false;
                break;
            }

            m_RenderBlocks.emplace_back(renderBlock);
        }
        else {
            std::stringstream error;
            error << "\"RenderBlock" << RenderBlockFactory::GetRenderBlockName(hash) << "\" (0x" << std::uppercase << std::setw(4) << std::hex << hash << ") is not yet supported.\n\n";
            error << "A model might still be rendered, but some parts of it may be missing.";

            DEBUG_LOG("[WARNING] RenderBlockModel::Parse - Unknown render block. \"" << RenderBlockFactory::GetRenderBlockName(hash) << "\" - " << std::setw(4) << std::hex << hash);
            Window::Get()->ShowMessageBox(error.str());

            // if we haven't parsed any other blocks yet
            // we'll never see anything rendered, let the user know something is wrong
            if (m_RenderBlocks.size() == 0) {
                m_ReadBlocksError = error.str();

                parse_success = false;
                break;
            }
        }
    }

end:
    if (parse_success) {
        ModelManager::Get()->AddModel(this, g_CurrentLoadedArchive);
    }

    // run file batches
    FileLoader::Get()->m_UseBatches = false;
    FileLoader::Get()->RunFileBatches();

    return true;
}

void RenderBlockModel::FileReadCallback(const fs::path& filename, const FileBuffer& data)
{
    auto rbm = new RenderBlockModel(filename);
    if (!rbm->Parse(data)) {
        Window::Get()->ShowMessageBox(rbm->GetError());
        safe_delete(rbm);
    }
}

void RenderBlockModel::Draw(RenderContext_t* context)
{
#if 0
    //m_Rotation.z += 0.015f;

    // calculate the world matrix
    {
        m_WorldMatrix = glm::translate(glm::mat4(1.0f), m_Position);
        m_WorldMatrix = glm::scale(m_WorldMatrix, m_Scale);
        m_WorldMatrix = glm::rotate(m_WorldMatrix, m_Rotation.x, glm::vec3{ 0.0f, 0.0f, 1.0f });
        m_WorldMatrix = glm::rotate(m_WorldMatrix, m_Rotation.y, glm::vec3{ 1.0f, 0.0f, 0.0f });
        m_WorldMatrix = glm::rotate(m_WorldMatrix, m_Rotation.z, glm::vec3{ 0.0f, 1.0f, 0.0f });
    }
#endif

    // draw all render blocks
    for (auto& renderBlock : m_RenderBlocks) {
        context->m_Renderer->SetDefaultRenderStates();

        renderBlock->Setup(context);
        renderBlock->Draw(context);
    }

    // draw filename label
    if (g_ShowModelLabels) {
        static auto white = glm::vec4{ 1, 1, 1, 0.6 };
        DebugRenderer::Get()->DrawText(GetFileName(), m_Position, white, true);
    }

    // draw bounding boxes
    if (g_DrawBoundingBoxes) {
        auto _min = m_BoundingBoxMin + m_Position;
        auto _max = m_BoundingBoxMax + m_Position;

        static auto red = glm::vec4{ 1, 0, 0, 1 };
        static auto green = glm::vec4{ 0, 1, 0, 1 };

        bool is_hover = false;
        DebugRenderer::Get()->DrawBBox(_min, _max, is_hover ? green : red);
    }
}