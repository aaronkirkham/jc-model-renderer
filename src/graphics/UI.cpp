#include <graphics/UI.h>
#include <Window.h>
#include <jc3/FileLoader.h>

#include <imgui.h>
#include <json.hpp>

void UI::Render()
{
    if (ImGui::BeginMainMenuBar())
    {
        m_MainMenuBarHeight = ImGui::GetWindowHeight();

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                TerminateProcess(GetCurrentProcess(), 0);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Renderer"))
        {
            static bool wireframe = false;
            if (ImGui::Checkbox("Wireframe", &wireframe))
            {
                Renderer::Get()->SetFillMode(wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID);
            }

            static bool bounding_boxes = false;
            if (ImGui::Checkbox("Bounding Boxes", &bounding_boxes))
            {
                
            }

            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }

    // render archive windows
    {
        auto archives = FileLoader::Get()->GetLoadedArchives();
        for (auto& archive : archives)
        {
            ImGui::Begin(archive->m_Filename.string().c_str(), nullptr, ImVec2(), 0.0f, (ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings));
            {
                for (auto& entry : archive->m_Files)
                {
                    if (entry.m_Filename.find(".rbm") != std::string::npos)
                    {
                        ImGui::TreeNodeEx(entry.m_Filename.c_str(), (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen));

                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip(entry.m_Filename.c_str());
                        }

                        if (ImGui::IsItemClicked()) {
                            UI::Get()->Events().FileTreeItemSelected(entry.m_Filename);
                        }
                    }
                }
            }
            ImGui::End();
        }
    }

    // Stats
    ImGui::Begin("Stats", nullptr, ImVec2(), 0.0f, (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs));
    {
        auto windowSize = Window::Get()->GetSize();

        ImGui::SetWindowPos(ImVec2(10, windowSize.y - 40));
        ImGui::Text("%.01f FPS (%.02f ms) (%.0f x %.0f)", ImGui::GetIO().Framerate, (1000.0f / ImGui::GetIO().Framerate), windowSize.x, windowSize.y);
    }
    ImGui::End();

    RenderFileTreeView();
}

void RenderDirectoryList(json* current)
{
    if (current->is_object()) {
        for (auto it = current->begin(); it != current->end(); ++it) {
            auto current_key = it.key();
            
            if (current_key != "/") {
                ImGuiTreeNodeFlags flags = 0;

                // open the root directories
                if (current_key == "editor" || current_key == "entities" || current_key == "models") {
                    flags |= ImGuiTreeNodeFlags_DefaultOpen;
                }

                if (ImGui::TreeNodeEx(current_key.c_str(), flags)) {
                    auto next = &current->operator[](current_key);
                    RenderDirectoryList(next);

                    ImGui::TreePop();
                }
            }
            else {
                auto next = &current->operator[](current_key);
                RenderDirectoryList(next);
            }
        }
    }
    else {
        if (current->is_string()) {
            ImGui::TextWrapped("I FORGOT WHAT THIS IS :(");
        }
        else {
            for (auto& leaf : *current) {
                auto filename = leaf.get<std::string>();

                ImGui::TreeNodeEx(filename.c_str(), (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen));

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(filename.c_str());
                }

                if (ImGui::IsItemClicked()) {
                    UI::Get()->Events().FileTreeItemSelected(filename);
                }
            }
        }
    }
};

void UI::RenderFileTreeView()
{
    ImGui::Begin("Entity Explorer", nullptr, ImVec2(), 1.0f, (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse));
    {
        auto size = Window::Get()->GetSize();
        ImGui::SetWindowPos(ImVec2(size.x - 400, m_MainMenuBarHeight));
        ImGui::SetWindowSize(ImVec2(400, (size.y - m_MainMenuBarHeight)));

        ImGui::PushItemWidth(-1);
        {
            if (!FileLoader::Get()->IsLoadingDictionary())
            {
                RenderDirectoryList(FileLoader::Get()->GetDirectoryList()->GetStructure());
            }
            else
            {
                ImGui::Text("Loading archive dictionary...");
            }
        }
        ImGui::PopItemWidth();
    }
    ImGui::End();
}