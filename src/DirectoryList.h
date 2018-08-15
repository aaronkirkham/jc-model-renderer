#pragma once

#include <json.hpp>
#include <string.h>
#include <memory.h>

#include <jc3/formats/StreamArchive.h>

#include <imgui.h>
#include <graphics/imgui/fonts/fontawesome5_icons.h>
#include <graphics/UI.h>

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
            }
            else {
                auto temp = current;
                current = nullptr;
                current["/"] = temp;
                split(str, current[directory]);
            }
        }
        else {
            if (!current.is_object()) {
                current.emplace_back(str);
            }
            else {
                current["/"].emplace_back(str);
            }
        }
    }

    inline const char* filetype_icon(const fs::path& extension)
    {
        if (extension == ".ee" || extension == ".bl" || extension == ".nl" || extension == ".fl") return ICON_FA_FILE_ARCHIVE;
        else if (extension == ".dds" || extension == ".ddsc" || extension == ".hmddsc") return ICON_FA_FILE_IMAGE;
        else if (extension == ".bank") return ICON_FA_FILE_AUDIO;
        else if (extension == ".bikc") return ICON_FA_FILE_VIDEO;

        return ICON_FA_FILE;
    }

public:
    DirectoryList() = default;
    virtual ~DirectoryList() = default;

    void Add(const std::string& filename)
    {
        split(filename, m_Structure);
    }

    void Parse(nlohmann::json* structure, const std::vector<std::string>& include_only = {})
    {
        if (structure) {
            m_Structure.clear();

            for (auto it = structure->begin(); it != structure->end(); ++it) {
                const auto& key = it.key();

                // should we only parse specific includes?
                if (!include_only.empty()) {
                    for (const auto& extension : include_only) {
                        if (key.find(extension) != std::string::npos) {
                            split(key, m_Structure);
                        }
                    }
                }
                else {
                    split(key, m_Structure);
                }
            }
        }
    }

    void Parse(StreamArchive_t* archive, const std::vector<std::string>& include_only = {})
    {
        if (archive) {
            m_Structure.clear();

            for (auto& file : archive->m_Files) {
                if (!include_only.empty()) {
                    for (auto& extension : include_only) {
                        if (file.m_Filename.find(extension) != std::string::npos) {
                            split(file.m_Filename, m_Structure);
                        }
                    }
                }
                else {
                    split(file.m_Filename, m_Structure);
                }
            }
        }
    }

    void Draw(AvalancheArchive* archive, nlohmann::json* current = nullptr, std::string prev = "", bool open_folders = false)
    {
        if (!current) current = GetStructure();

        if (current->is_object()) {
            for (auto it = current->begin(); it != current->end(); ++it) {
                auto current_key = it.key();

                if (current_key != "/") {
                    auto id = ImGui::GetID(current_key.c_str());
                    auto is_open = ImGui::GetStateStorage()->GetInt(id);

                    if (ImGui::TreeNodeEx(current_key.c_str(), ImGuiTreeNodeFlags_None, "%s  %s", is_open ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER, current_key.c_str())) {
                        auto next = &current->operator[](current_key);
                        Draw(archive, next, prev.length() ? prev + "/" + current_key : current_key, open_folders);

                        ImGui::TreePop();
                    }
                }
                else {
                    auto next = &current->operator[](current_key);
                    Draw(archive, next, prev, open_folders);
                }
            }
        }
        else {
            if (current->is_string()) {
#ifdef DEBUG
                __debugbreak();
#endif
            }
            else {
                for (auto& leaf : *current) {
                    auto filename = fs::path(leaf.get<std::string>());
                    ImGui::TreeNodeEx(filename.c_str(), (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen), "%s  %s", filetype_icon(filename.extension()), filename.string().c_str());

                    auto prev_path = fs::path(prev);
                    fs::path file_path;

                    if (prev.length() == 0) {
                        file_path = filename;
                    }
                    else if (prev.length() == 1) {
                        file_path = prev_path.string() + filename.string();
                    }
                    else {
                        file_path = prev_path / filename;
                    }

                    // tooltips
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip(filename.string().c_str());
                    }

                    // fire file selected events
                    if (ImGui::IsItemClicked()) {
                        UI::Get()->Events().FileTreeItemSelected(file_path, archive);
                    }

                    // context menu
                    UI::Get()->RenderContextMenu(file_path, 0, archive ? CTX_FILE | CTX_ARCHIVE : CTX_FILE);
                }
            }
        }
    }

    nlohmann::json* GetStructure() { return &m_Structure; }
};