#pragma once

#include <filesystem>

struct tree_node {
    std::string              folder_name;
    std::vector<tree_node>   folders;
    std::vector<std::string> files;

    tree_node() = default;
    tree_node(const std::string& folder_name, const std::string& files)
        : folder_name(folder_name)
    {
        add(files);
    }

    void add(const std::string& filepath)
    {
        auto slash_pos = filepath.find('/');
        if (slash_pos != std::string::npos) {
            auto directory = filepath.substr(0, slash_pos);
            auto remain    = filepath.substr(slash_pos + 1, filepath.length());

            auto it = std::find_if(folders.begin(), folders.end(),
                                   [&](const tree_node& f) { return directory == f.folder_name; });

            if (it == folders.end()) {
                folders.emplace_back(tree_node{directory, remain});
            } else {
                (*it).add(remain);
            }
        } else {
            files.push_back(filepath);
        }
    }
};

class AvalancheArchive;
class DirectoryList
{
  private:
    tree_node   m_Tree;
    const char* GetFileTypeIcon(const std::filesystem::path& filename);

  public:
    DirectoryList()          = default;
    virtual ~DirectoryList() = default;

    void Add(const std::string& filepath)
    {
        m_Tree.add(filepath);
    }

    void Draw(AvalancheArchive* current_archive = nullptr, std::string acc_filepath = "", tree_node* tree = nullptr);
};
