#pragma once

#include <filesystem>

struct tree_node {
    std::string              name;
    std::vector<tree_node>   folders;
    std::vector<std::string> files;

    tree_node() = default;
    tree_node(const std::string& folder_name, const std::string& remaining)
        : name(folder_name)
    {
        add(remaining);
    }

    void add(const std::string& filepath)
    {
        auto slash_pos = filepath.find('/');
        if (slash_pos != std::string::npos) {
            auto directory = filepath.substr(0, slash_pos);
            auto remaining = filepath.substr(slash_pos + 1, filepath.length());

            auto it = std::find_if(folders.begin(), folders.end(),
                                   [&](const tree_node& folder) { return directory == folder.name; });

            if (it == folders.end()) {
                folders.emplace_back(tree_node{directory, remaining});
            } else {
                (*it).add(remaining);
            }
        } else {
            files.push_back(filepath);
        }
    }

    static bool sort_function(const tree_node& lhs, const tree_node& rhs)
    {
        return lhs.name < rhs.name;
    }

    void sort()
    {
        std::sort(folders.begin(), folders.end(), sort_function);
        for (auto& folder : folders) {
            folder.sort();
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

    void Sort()
    {
        m_Tree.sort();
    }

    void Clear()
    {
        m_Tree = {};
    }

    void Draw(AvalancheArchive* current_archive = nullptr, std::string acc_filepath = "", tree_node* tree = nullptr);
};
