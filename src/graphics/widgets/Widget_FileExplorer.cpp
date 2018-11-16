#include "Widget_FileExplorer.h"
#include <jc3/FileLoader.h>

Widget_FileExplorer::Widget_FileExplorer()
{
    m_Title = "File Explorer";
}

void Widget_FileExplorer::Render(RenderContext_t* context)
{
    const auto directory_list = FileLoader::Get()->GetDirectoryList();
    if (directory_list) {
        Widget_FileExplorer::RenderTreeView(directory_list->GetTree());
    }
}

void Widget_FileExplorer::RenderTreeView(nlohmann::json* tree, AvalancheArchive* current_archive,
                                         std::string acc_filepath)
{
    assert(tree);

    // current tree is a directory
    if (tree->is_object()) {
        for (auto& it = tree->cbegin(); it != tree->cend(); ++it) {
            const auto& key = it.key();

            // render the directory root filelist
            if (key == DIRECTORYLIST_ROOT) {
                Widget_FileExplorer::RenderTreeView(&(*tree)[key], current_archive, acc_filepath);
                continue;
            }

            // check if the current directory is open
            const auto id      = ImGui::GetID(key.c_str());
            const auto is_open = ImGui::GetStateStorage()->GetBool(id);

            if (ImGui::TreeNodeEx(key.c_str(), 0, "%s  %s", is_open ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER,
                                  key.c_str())) {
                Widget_FileExplorer::RenderTreeView(&(*tree)[key], current_archive,
                                                    acc_filepath.length() ? acc_filepath + "/" + key : key);
                ImGui::TreePop();
            }
        }
    }
    // current tree is a filelist
    else if (tree->is_array()) {
        for (const auto& value : *tree) {
            const auto& filename           = value.get<std::string>();
            const auto& filename_with_path = acc_filepath.length() ? acc_filepath + "/" + filename : filename;

            ImGui::TreeNodeEx(filename.c_str(), (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen),
                              "%s  %s", UI::Get()->GetFileTypeIcon(filename), filename.c_str());

            // render tooltip
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(filename.c_str());
            }

            // selection events
            if (ImGui::IsItemClicked()) {
                UI::Get()->Events().FileTreeItemSelected(filename_with_path, current_archive);
            }

            // render context menu
            UI::Get()->RenderContextMenu(filename_with_path, 0, current_archive ? CTX_FILE | CTX_ARCHIVE : CTX_FILE);
        }
    }
}
