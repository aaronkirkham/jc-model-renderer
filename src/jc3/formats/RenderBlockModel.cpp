#include <jc3/formats/RenderBlockModel.h>
#include <jc3/RenderBlockFactory.h>
#include <Window.h>
#include <jc3/RenderBlockFactory.h>
#include <graphics/Renderer.h>
#include <graphics/Camera.h>
#include <graphics/DebugRenderer.h>
#include <jc3/formats/AvalancheArchive.h>
#include <fonts/fontawesome_icons.h>
#include <Input.h>
#include <imgui.h>

static std::vector<RenderBlockModel*> g_Models;
static std::recursive_mutex g_ModelsMutex;
extern bool g_DrawBoundingBoxes;
extern bool g_DrawDebugInfo;
extern AvalancheArchive* g_CurrentLoadedArchive;

static auto g_RenderBlockColour = glm::vec4{ 1, 1, 1, 1 };

bool RenderBlockModel::ParseRenderBlockModel(std::istream& stream)
{
    static std::once_flag once_;
    static IRenderBlock* renderBlockViewTextures_ = nullptr;
    std::call_once(once_, [&] {
        Renderer::Get()->Events().RenderFrame.connect([&](RenderContext_t* context) {
            std::lock_guard<std::recursive_mutex> _lk{ g_ModelsMutex };
            for (auto& model : g_Models) {
                model->Draw(context);
            }

            // debug model info
            if (g_DrawDebugInfo) {
                ImGui::Begin("Model Stats", nullptr, ImVec2(), 0.0f, (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings));
                {
                    ImGui::SetWindowPos(ImVec2(10, 20));

                    //ImGui::ColorPicker4("texture_col", (float*)&color, (ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_Float));

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

                    size_t model_index = 0;
                    for (auto& model : g_Models) {
                        size_t block_index = 0;

                        ImGui::Text("%s", model->GetPath().filename().string().c_str());

                        for (auto& block : model->GetRenderBlocks()) {
                            ImGui::Text("\t%d: %s", block_index, block->GetTypeName());

                            const auto is_hovered = ImGui::IsItemHovered();
                            model->SetRenderBlockColour(block, is_hovered ? glm::vec4{ 1, 0, 0, 0.3 } : g_RenderBlockColour);

                            if (!block->GetTextures().empty()) {
                                ImGui::SameLine();

                                std::stringstream ss;
                                ss << model_index << "-textures-" << block_index;

                                ImGui::PushID(ss.str().c_str());
                                if (ImGui::Button("View Textures")) {
                                    renderBlockViewTextures_ = block;
                                }
                                ImGui::PopID();
                            }

                            block_index++;
                        }

                        model_index++;
                    }
                }
                ImGui::End();

                if (renderBlockViewTextures_) {
                    static bool open = true;
                    ImGui::Begin(ICON_FA_PICTURE_O " Textures", &open, ImVec2(400, 200), 1.0f, (ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings));
                    {
                        ImGui::Columns(3, 0, false);

                        for (auto& texture : renderBlockViewTextures_->GetTextures()) {
                            auto window_size = Window::Get()->GetSize();
                            auto aspect_ratio = (window_size.x / window_size.y);

                            ImGui::Text(texture->GetPath().filename().string().c_str());

                            ImGui::BeginGroup();
                            {
                                auto width = ImGui::GetWindowWidth() / ImGui::GetColumnsCount();
                                ImGui::Image(texture->GetSRV(), ImVec2(width, width / aspect_ratio));
                            }
                            ImGui::EndGroup();

                            ImGui::NextColumn();
                        }

                        ImGui::Columns();
                    }
                    ImGui::End();

                    // close the window
                    if (!open) {
                        renderBlockViewTextures_ = nullptr;
                        open = true;
                    }
                }
            }
        });
    });

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

    // read all the render blocks
    for (uint32_t i = 0; i < header.m_NumberOfBlocks; ++i)
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
                m_ReadBlocksError = "Failed to read Render Block.";
                DEBUG_LOG("[ERROR] " << m_ReadBlocksError);

                parse_success = false;
                break;
            }

            m_RenderBlocks.emplace_back(renderBlock);
        }
        else {
            std::stringstream ss;
            ss << "Unknown Render Block type ";
            ss << "0x" << std::uppercase << std::setw(4) << std::hex << hash << ".";
            m_ReadBlocksError = ss.str();

            DEBUG_LOG("[ERROR] " << m_ReadBlocksError);

            parse_success = false;
            break;
        }
    }

end:
    if (parse_success) {
        std::lock_guard<std::recursive_mutex> _lk{ g_ModelsMutex };
        g_Models.emplace_back(this);

        // if we have a current loaded archive, link the current RBM to it so we can unload it after
        if (g_CurrentLoadedArchive) {
            g_CurrentLoadedArchive->LinkRenderBlockModel(this);
        }
    }

    return parse_success;
}

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
    : m_File(filename)
{
    // create the mesh constant buffer
    MeshConstants constants;
    m_MeshConstants = Renderer::Get()->CreateConstantBuffer(constants);
}

RenderBlockModel::~RenderBlockModel()
{
    DEBUG_LOG("RenderBlockModel::~RenderBlockModel");

    std::lock_guard<std::recursive_mutex> _lk{ g_ModelsMutex };
    g_Models.erase(std::remove(g_Models.begin(), g_Models.end(), this), g_Models.end());

    for (auto& renderBlock : m_RenderBlocks) {
        delete renderBlock;
    }

    Renderer::Get()->DestroyBuffer(m_MeshConstants);
}

void RenderBlockModel::FileReadCallback(const fs::path& filename, const std::vector<uint8_t>& data)
{
    std::istringstream stream(std::string{ (char*)data.data(), data.size() });

    auto rbm = new RenderBlockModel(filename);
    if (!rbm->ParseRenderBlockModel(stream)) {
        Window::Get()->ShowMessageBox(rbm->GetError());
        delete rbm;
    }
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

    // update mesh constants
    MeshConstants constants;
    constants.worldMatrix = m_WorldMatrix;
    constants.worldViewInverseTranspose = glm::transpose(glm::inverse(m_WorldMatrix * Camera::Get()->GetViewMatrix()));

    // draw all render blocks
    for (auto& renderBlock : m_RenderBlocks) {
        Renderer::Get()->SetDefaultRenderStates();

        constants.colour = GetRenderBlockColour(renderBlock);

        Renderer::Get()->SetVertexShaderConstants(m_MeshConstants, 1, constants);
        Renderer::Get()->SetPixelShaderConstants(m_MeshConstants, 1, constants);

        renderBlock->Setup(context);
        renderBlock->Draw(context);
    }

    // draw filename
    static auto white = glm::vec4{ 1, 1, 1, 0.6 };
    DebugRenderer::Get()->DrawText(GetFileName(), m_Position, white, true);

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

void RenderBlockModel::SetRenderBlockColour(IRenderBlock* block, const glm::vec4& colour)
{
    auto it = m_RenderBlockColours.find(block);
    if (it != std::end(m_RenderBlockColours)) {
        (*it).second = colour;
        return;
    }

    m_RenderBlockColours[block] = colour;
}

const glm::vec4& RenderBlockModel::GetRenderBlockColour(IRenderBlock* block) const
{
    auto it = m_RenderBlockColours.find(block);
    if (it == std::end(m_RenderBlockColours)) {
        return g_RenderBlockColour;
    }

    return (*it).second;
}