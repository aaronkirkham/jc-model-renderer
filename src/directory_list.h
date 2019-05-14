#pragma once

#include <filesystem>
#include <json.hpp>

#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/ui.h"

#include <imgui.h>

static constexpr auto DIRECTORYLIST_ROOT = "zzzzz_root";

class DirectoryList
{
  private:
    nlohmann::json m_Structure;

    void split(std::string str, nlohmann::json& current)
    {
        auto current_pos = str.find('/');
        if (current_pos != std::string::npos) {
            auto directory = str.substr(0, current_pos);
            str.erase(0, current_pos + 1);

            if (current.is_object() || current.is_null()) {
                split(str, current[directory]);
            } else {
                auto temp                   = current;
                current                     = nullptr;
                current[DIRECTORYLIST_ROOT] = temp;
                split(str, current[directory]);
            }
        } else {
            if (!current.is_object()) {
                current.emplace_back(str);
            } else {
                current[DIRECTORYLIST_ROOT].emplace_back(str);
            }
        }
    }

    inline const char* GetFileTypeIcon(const std::filesystem::path& filename)
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

  public:
    DirectoryList()          = default;
    virtual ~DirectoryList() = default;

    void Add(const std::string& filename)
    {
        split(filename, m_Structure);
    }

    void Parse(nlohmann::json* tree)
    {
        if (tree) {
            m_Structure.clear();

            for (auto it = tree->begin(); it != tree->end(); ++it) {
                split(it.key(), m_Structure);
            }
        }
    }

    void Draw(nlohmann::json* tree, AvalancheArchive* current_archive = nullptr, std::string acc_filepath = "")
    {
        // current tree is a directory
        if (tree->is_object()) {
            for (auto& it = tree->cbegin(); it != tree->cend(); ++it) {
                const auto& key = it.key();

                // render the directory root filelist
                if (key == DIRECTORYLIST_ROOT) {
                    Draw(&(*tree)[key], current_archive, acc_filepath);
                    continue;
                }

                // check if the current directory is open
                const auto id      = ImGui::GetID(key.c_str());
                const auto is_open = ImGui::GetStateStorage()->GetBool(id);

                if (ImGui::TreeNodeEx(key.c_str(), 0, "%s  %s", is_open ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER,
                                      key.c_str())) {
                    Draw(&(*tree)[key], current_archive, acc_filepath.length() ? acc_filepath + "/" + key : key);

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
                                  "%s  %s", GetFileTypeIcon(filename), filename.c_str());

                // render tooltip
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(filename.c_str());
                }

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
    }

    nlohmann::json* GetTree()
    {
        return &m_Structure;
    }
};
