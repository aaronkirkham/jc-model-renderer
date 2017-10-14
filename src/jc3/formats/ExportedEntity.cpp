#include <jc3/formats/ExportedEntity.h>
#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>
#include <Window.h>
#include <graphics/UI.h>

#include <zlib.h>



RenderBlockModel* test_model = nullptr;

void ExportedEntity::Init()
{
    static std::once_flag once_;
    std::call_once(once_, [&] {

        Renderer::Get()->Events().RenderFrame.connect([&](RenderContext_t* context) {

            ImGui::Begin(m_File.filename().string().c_str(), nullptr, ImVec2(), 0.0f, ImGuiWindowFlags_AlwaysAutoResize);
            {
                for (auto& entry : m_StreamArchive->m_Files)
                {
                    if (entry.m_Filename.find(".rbm") != std::string::npos)
                    {
                        ImGui::TreeNodeEx(entry.m_Filename.c_str(), (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen));
                        if (ImGui::IsItemClicked()) {
                            UI::Get()->Events().FileTreeItemSelected(entry.m_Filename);
                        }
                    }
                }
            }
            ImGui::End();

        });
    });
}

ExportedEntity::ExportedEntity(const fs::path& file)
    : m_File(file)
{
    // make sure this is an ee file
    if (file.extension() != ".ee") {
        throw std::invalid_argument("ExportedEntity type only supports .ee files!");
    }

    // read the stream archive
    m_StreamArchive = FileLoader::Get()->ReadStreamArchive(file);

    Init();
}

ExportedEntity::ExportedEntity(const fs::path& filename, const std::vector<uint8_t>& buffer)
    : m_File(filename)
{
    // read the stream archive
    m_StreamArchive = FileLoader::Get()->ReadStreamArchive(buffer);

    Init();
}

ExportedEntity::~ExportedEntity()
{
    delete test_model;
    test_model = nullptr;
}