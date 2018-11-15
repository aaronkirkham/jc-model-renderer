#pragma once

#include <json.hpp>
#include <string.h>
#include <memory.h>

#include <jc3/formats/StreamArchive.h>

#include <imgui.h>
#include <graphics/imgui/fonts/fontawesome5_icons.h>
#include <graphics/UI.h>

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
            }
            else {
                auto temp = current;
                current = nullptr;
                current[DIRECTORYLIST_ROOT] = temp;
                split(str, current[directory]);
            }
        }
        else {
            if (!current.is_object()) {
                current.emplace_back(str);
            }
            else {
                current[DIRECTORYLIST_ROOT].emplace_back(str);
            }
        }
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

    nlohmann::json* GetTree() { return &m_Structure; }
};