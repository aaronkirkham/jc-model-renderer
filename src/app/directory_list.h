#ifndef JCMR_APP_DIRECTORY_LIST_H_HEADER_GUARD
#define JCMR_APP_DIRECTORY_LIST_H_HEADER_GUARD

#include "platform.h"

namespace jcmr
{
struct App;

struct DirectoryList {
    using File = std::tuple<std::string, std::string, u32>;

    std::string                name;
    std::string                path;
    std::vector<DirectoryList> folders;
    std::vector<File>          files;

    DirectoryList() = default;
    explicit DirectoryList(const std::string& folder_name, const std::string& remaining_parts,
                           const std::string& current_path, const std::string& full_path);

    void add(const std::string& filepath);
    void sort();

    void render_search_input();
    void render(App& app, const std::string& tooltip_prefix = "");

  private:
    void insert_part(const std::string& part, const std::string& full_path);
};
} // namespace jcmr

#endif // JCMR_APP_DIRECTORY_LIST_H_HEADER_GUARD
