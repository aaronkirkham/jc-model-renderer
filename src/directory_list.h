#pragma once

#include <filesystem>
#include <json.hpp>

static constexpr auto DIRECTORYLIST_ROOT = "zzzzz_root";

class AvalancheArchive;
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

    const char* GetFileTypeIcon(const std::filesystem::path& filename);

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

    void Draw(nlohmann::json* tree, AvalancheArchive* current_archive = nullptr, std::string acc_filepath = "");

    nlohmann::json* GetTree()
    {
        return &m_Structure;
    }
};
