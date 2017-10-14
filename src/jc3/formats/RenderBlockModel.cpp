#include <jc3/formats/RenderBlockModel.h>
#include <jc3/RenderBlockFactory.h>
#include <Window.h>
#include <jc3/RenderBlockFactory.h>
#include <graphics/Renderer.h>
#include <graphics/Camera.h>
#include <graphics/DebugRenderer.h>

static std::vector<RenderBlockModel*> g_Models;

void RenderBlockModel::ParseRenderBlockModel(std::istream& stream)
{
    g_Models.emplace_back(this);

    static std::once_flag once_;
    std::call_once(once_, [&] {
        Renderer::Get()->Events().RenderFrame.connect([&](RenderContext_t* context) {
            for (auto& model : g_Models) {
                model->Draw(context);
            }

            // debug model info
            ImGui::Begin("Model Stats", nullptr, ImVec2(0, 0), 0.0f, (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs));
            {
                std::size_t vertices = 0;
                std::size_t indices = 0;
                std::size_t triangles = 0;

                for (auto& model : g_Models) {
                    for (auto& block : model->GetRenderBlocks()) {
                        auto index_count = block->GetIndexBuffer()->m_ElementCount;

                        vertices += block->GetVertexBuffer()->m_ElementCount;
                        indices += index_count;
                        triangles += (index_count / 3);
                    }
                }

                ImGui::Text("Models: %d, Vertices: %d, Indices: %d, Triangles: %d, Textures: %d", g_Models.size(), vertices, indices, triangles, TextureManager::Get()->GetCacheSize());

                for (auto& model : g_Models) {
                    size_t block_index = 0;

                    ImGui::Text("%s", model->GetPath().filename().string().c_str());

                    for (auto& block : model->GetRenderBlocks()) {
                        ImGui::Text("\t%d: %s", block_index, block->GetTypeName());
                        block_index++;
                    }
                }
            }
            ImGui::End();
        });
    });

    // read the model header
    JustCause3::RBM header;
    stream.read((char *)&header, sizeof(header));

    // ensure the header magic is correct
    if (strncmp((char *)&header.m_Magic, "RBMDL", 5) != 0) {
        throw std::runtime_error("Invalid file header. Input file probably isn't a RenderBlockModel file.");
    }

    DEBUG_LOG("RenderBlockModel v" << header.m_VersionMajor << "." << header.m_VersionMinor << "." << header.m_VersionRevision);
    DEBUG_LOG(" - m_NumberOfBlocks=" << header.m_NumberOfBlocks);

    m_RenderBlocks.reserve(header.m_NumberOfBlocks);

    // store the bounding box
    m_BoundingBoxMin = header.m_BoundingBoxMin;
    m_BoundingBoxMax = header.m_BoundingBoxMax;

    // read all the render blocks
    for (uint32_t i = 0; i < header.m_NumberOfBlocks; i++)
    {
        uint32_t hash;
        stream.read((char *)&hash, sizeof(hash));

        auto renderBlock = RenderBlockFactory::CreateRenderBlock(hash);
        if (renderBlock) {
            renderBlock->Create();
            renderBlock->Read(m_File, stream);

            uint32_t checksum;
            stream.read((char *)&checksum, sizeof(checksum));

            // did we read the block correctly?
            if (checksum != 0x89ABCDEF) {
                DEBUG_LOG("[ERROR] Failed to read render block.");
                throw std::runtime_error("Failed to read render block.");
            }

            m_RenderBlocks.emplace_back(renderBlock);
        }
        else {
            // TODO: error message to the user!
            DEBUG_LOG("[ERROR] Unknown Render Block hash " << hash << "!");
            break;
        }
    }
}

RenderBlockModel::RenderBlockModel(const fs::path& file)
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

    ParseRenderBlockModel(stream);
}

RenderBlockModel::RenderBlockModel(const fs::path& filename, const std::vector<uint8_t>& buffer)
    : m_File(filename)
{
    // create the mesh constant buffer
    MeshConstants constants;
    m_MeshConstants = Renderer::Get()->CreateConstantBuffer(constants);

    std::istringstream stream(std::string{ (char*)buffer.data(), buffer.size() });
    ParseRenderBlockModel(stream);
}

RenderBlockModel::~RenderBlockModel()
{
    for (auto& renderBlock : m_RenderBlocks) {
        delete renderBlock;
    }

    Renderer::Get()->DestroyBuffer(m_MeshConstants);
}

void RenderBlockModel::Draw(RenderContext_t* context)
{
    //m_Rotation.z += 0.015f;

    // calculate the world matrix
    {
        m_WorldMatrix = glm::translate(glm::mat4(1.0f), m_Position);
        m_WorldMatrix = glm::scale(m_WorldMatrix, m_Scale);
        m_WorldMatrix = glm::rotate(m_WorldMatrix, m_Rotation.x, glm::vec3{ 0.0f, 0.0f, 1.0f });
        m_WorldMatrix = glm::rotate(m_WorldMatrix, m_Rotation.y, glm::vec3{ 1.0f, 0.0f, 0.0f });
        m_WorldMatrix = glm::rotate(m_WorldMatrix, m_Rotation.z, glm::vec3{ 0.0f, 1.0f, 0.0f });
    }

    // update mesh matrix
    {
        auto worldView = m_WorldMatrix * Camera::Get()->GetViewMatrix();

        MeshConstants constants;
        constants.worldMatrix = m_WorldMatrix;
        constants.worldViewInverseTranspose = glm::transpose(glm::inverse(worldView));
        constants.colour = glm::vec4{ 1, 1, 1, 1 };

        Renderer::Get()->SetVertexShaderConstants(m_MeshConstants, 1, constants);
    }

    for (auto& renderBlock : m_RenderBlocks) {
        Renderer::Get()->SetDefaultRenderStates();

        renderBlock->Setup(context);
        renderBlock->Draw(context);
    }

#if 1
    glm::vec3 screen;
    if (Camera::Get()->WorldToScreen(m_Position, &screen)) {
        auto _min = m_BoundingBoxMin + m_Position;
        auto _max = m_BoundingBoxMax + m_Position;

        DebugRenderer::Get()->DrawBBox(_min, _max, { 1, 0, 0, 1 });
    }
#endif
}