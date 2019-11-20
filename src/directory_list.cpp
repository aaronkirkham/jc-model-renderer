#include "directory_list.h"

#include <imgui.h>

#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/ui.h"

const char* DirectoryList::GetFileTypeIcon(const std::filesystem::path& filename)
{
    const auto& ext = filename.extension();

    if (ext == ".ee" || ext == ".bl" || ext == ".nl" || ext == ".fl")
        return ICON_FA_FILE_ARCHIVE;
    else if (ext == ".dds" || ext == ".ddsc" || ext == ".hmddsc" || ext == ".atx1" || ext == ".atx2")
        return ICON_FA_FILE_IMAGE;
    else if (ext == ".bank")
        return ICON_FA_FILE_AUDIO;
    else if (ext == ".bikc")
        return ICON_FA_FILE_VIDEO;

    return ICON_FA_FILE;
}

void DirectoryList::Draw(AvalancheArchive* current_archive, std::string acc_filepath, tree_node* tree)
{
    if (!tree) {
        return Draw(current_archive, acc_filepath, &m_Tree);
    }

    // draw folders
    for (auto&& folder : tree->folders) {
        const auto& key = folder.name;

        // check if the current directory is open
        const auto id      = ImGui::GetID(key.c_str());
        const auto is_open = ImGui::GetStateStorage()->GetBool(id);

        // draw folder
        if (ImGui::TreeNodeEx(key.c_str(), 0, "%s  %s", is_open ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER, key.c_str())) {
            Draw(current_archive, acc_filepath.length() ? acc_filepath + "/" + key : key, &folder);
            ImGui::TreePop();
        }
    }

    // draw files
    for (const auto& file : tree->files) {
        ImGui::TreeNodeEx(file.c_str(), (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen), "%s  %s",
                          GetFileTypeIcon(file), file.c_str());

        // render tooltip
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(file.c_str());
        }

        const auto& filename_with_path = acc_filepath.length() ? acc_filepath + "/" + file : file;

        // selection events
        if (ImGui::IsItemClicked()) {
            UI::Get()->Events().FileTreeItemSelected(filename_with_path, current_archive);
        }

        // render context menu
        UI::Get()->RenderContextMenu(filename_with_path, 0,
                                     current_archive ? ContextMenuFlags_File | ContextMenuFlags_Archive
                                                     : ContextMenuFlags_File);
    }
}
