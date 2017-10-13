#include <graphics/UI.h>
#include <Window.h>
#include <jc3/FileLoader.h>

#include <imgui.h>
#include <json.hpp>

void UI::Render()
{
    if (ImGui::BeginMainMenuBar())
    {
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

            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }

    // Stats
    ImGui::Begin("Stats", nullptr, ImVec2(), 0.0f, (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove));
    {
        auto windowSize = Window::Get()->GetSize();

        ImGui::SetWindowPos(ImVec2(10, windowSize.y - 50));
        ImGui::Text("%.01f FPS (%.02f ms)", ImGui::GetIO().Framerate, (1000.0f / ImGui::GetIO().Framerate));
    }
    ImGui::End();

    //RenderFileTreeView();

#if 0
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("ListBox %d hovered", i);
    }
#endif

}

// TODO: this is terrible
void RenderDirectoryList(const json& current) {

    if (current.is_object()) {
        for (auto it = current.begin(); it != current.end(); ++it)
        {
            auto current_key = it.key();
            auto current_data = it.value();

            // if this is not the root directory, create a tree node
            if (current_key != "/")
            {
                if (ImGui::TreeNode(current_key.c_str()))
                {
                    RenderDirectoryList(current[current_key]);
                    ImGui::TreePop();
                }
            }
            else
            {
                RenderDirectoryList(current[current_key]);
            }
        }
    }
    else {
        if (current.is_string()) {
            ImGui::Text(current.get<std::string>().c_str());
        }
        // probably an array
        else {
            for (auto& o : current) {
                ImGui::Text(o.get<std::string>().c_str());
            }
        }
    }

};

void UI::RenderFileTreeView()
{
    ImGui::Begin("Just Cause 3", nullptr, ImVec2(), 1.0f, (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings));
    {
        auto size = Window::Get()->GetSize();
        ImGui::SetWindowPos(ImVec2(size.x - 400, 0));
        ImGui::SetWindowSize(ImVec2(400, size.y));

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