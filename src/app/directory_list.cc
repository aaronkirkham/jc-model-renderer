#include "pch.h"

#include "directory_list.h"
#include "platform.h"

#include "app/app.h"
#include "app/utils.h"
#include "game/format.h"
#include "render/ui.h"

#include "render/fonts/icons.h"

#include <imgui.h>

namespace jcmr
{
// static ImVec4 highlight_colour{0.29f, 0.61f, 0.97f, 1.0f}; // #4b9ef9
static ImVec4 highlight_colour{0.65f, 0.65f, 0.65f, 1.0f}; // #a6a6a6
// static ImVec4 loaded_colour{1.0f, 0.8f, 0.0f, 1.0f};       // #ffcc00
static ImVec4 loaded_colour{0.29f, 0.61f, 0.97f, 1.0f}; // #4b9ef9

static char s_search_terms[1024] = {0};
static bool has_search_terms()
{
    return s_search_terms[0] != '\0';
}

DirectoryList::DirectoryList(const std::string& folder_name, const std::string& remaining_parts,
                             const std::string& current_path, const std::string& full_path)
    : name(folder_name)
    , path(current_path)
{
    insert_part(remaining_parts, full_path);
}

void DirectoryList::add(const std::string& filepath)
{
    insert_part(filepath, filepath);
}

void DirectoryList::insert_part(const std::string& part, const std::string& full_path)
{
    auto slash_pos = part.find('/');
    if (slash_pos != std::string::npos) {
        auto directory = part.substr(0, slash_pos);
        auto remaining = part.substr(slash_pos + 1, part.length());

        auto iter = std::find_if(folders.begin(), folders.end(),
                                 [&](const DirectoryList& folder) { return directory == folder.name; });

        if (iter == folders.end()) {
            // calculate the path to the current directory
            auto current_path = full_path.substr(0, full_path.find(directory) + directory.length());

            // add new directory
            DirectoryList dl(directory, remaining, current_path, full_path);
            folders.emplace_back(std::move(dl));
        } else {
            // folder already exists, insert the remaining bits into the current folder
            (*iter).insert_part(remaining, full_path);
        }
    } else {
        // add the file to the current folder
        auto extension_hash = ava::hashlittle(utils::get_extension(part).c_str());
        files.push_back(std::make_tuple(part, full_path, extension_hash));
    }
}

void DirectoryList::sort()
{
    // sort folders
    std::sort(folders.begin(), folders.end(),
              [](const DirectoryList& lhs, const DirectoryList& rhs) -> bool { return lhs.name < rhs.name; });

    // sort files
    std::sort(files.begin(), files.end(),
              [](const File& lhs, const File& rhs) { return std::get<0>(lhs) < std::get<0>(rhs); });

    // sort child folders
    for (auto& folder : folders) {
        folder.sort();
    }
}

void DirectoryList::render_search_input()
{
    const auto& style       = ImGui::GetStyle();
    const auto  avail_width = ImGui::GetContentRegionAvail().x;
    const auto  button_size = (ImGui::CalcTextSize(ICON_FA_TIMES).x + (style.FramePadding.x * 2));

    // total_width - button_size - (item spacing (between button and input))
    ImGui::PushItemWidth(avail_width - button_size - style.ItemSpacing.x);

    if (ImGui::InputTextWithHint("##search", ICON_FA_SEARCH " Search", s_search_terms, sizeof(s_search_terms))) {
        // TODO : maybe just a function which handles the filtering.
        //        we should maybe have a bool on each File in the tree for visibility so we don't have to test each time
        //        in the render function.

        // if (!has_search_terms() && !s_clicked_items_during_search.empty()) {
        //     for (auto& path : s_clicked_items_during_search) {
        //         LOG_INFO("{}", path);
        //     }

        //     s_clicked_items_during_search.clear();
        // }
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_TIMES "##clear_search")) {
        s_search_terms[0] = '\0';
    }
}

static void set_tooltip(const std::string& value, const std::string& prefix)
{
    if (ImGui::IsItemHovered()) {
        // TODO : how do we get the base colour without any PushStyleColor() overriding?
        ImGui::PushStyleColor(ImGuiCol_Text, {1, 1, 1, 1});

        if (prefix.empty()) {
            ImGui::SetTooltip("%s", value.c_str());
        } else {
            ImGui::SetTooltip("%s://%s", prefix.c_str(), value.c_str());
        }

        ImGui::PopStyleColor();
    }
}

static bool matches_search_terms(DirectoryList& directory_list)
{
    // look in sub-directories
    for (auto& folder : directory_list.folders) {
        // match folder name
        // TODO : if we want to keep this, we'll need to return something which indicates if we matched on a folder or a
        //        file, this is because, if we matched a folder we probably want to show all the contents of that
        //        folder, not just files which also match the search terms.
        // if (folder.name.find(s_search_terms) != std::string::npos) {
        //     return true;
        // }

        // look recursively inside the folder
        if (matches_search_terms(folder)) {
            return true;
        }
    }

    // find the file
    for (auto& [filename, path, extension_hash] : directory_list.files) {
        if (filename.find(s_search_terms) != std::string::npos) {
            return true;
        }
    }

    return false;
}

void DirectoryList::render(App& app, const std::string& tooltip_prefix)
{
    // folders
    for (auto& folder : folders) {
        bool should_open_tree_node = false;

        // ignore this folder if it doesn't contain anything which matches our search terms
        if (has_search_terms() && !matches_search_terms(folder)) {
            continue;
        }
        // we have search terms, AND something in this directory matches the terms
        else if (has_search_terms() && strlen(s_search_terms) >= 4) {
            should_open_tree_node = true;
        }

        auto& key = folder.name;

        u32 flags = ImGuiTreeNodeFlags_SpanAvailWidth;
        if (should_open_tree_node) {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        const bool last_open_state = ImGui::GetStateStorage()->GetBool(ImGui::GetID(key.c_str()));
        const bool tree_open       = ImGui::TreeNodeEx(key.c_str(), flags, "%s  %s",
                                                       (last_open_state ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER), key.c_str());

        set_tooltip(folder.path, tooltip_prefix);
        app.get_ui().draw_context_menu(folder.path, nullptr, UI::E_CONTEXT_FOLDER);

        if (tree_open) {
            folder.render(app, tooltip_prefix);
            ImGui::TreePop();
        }
    }

    // files
    for (auto& [filename, path, extension_hash] : files) {
        // skip the current file if we're searching and the filename doesn't contain the search terms
        // TODO : don't filter the file if the handler wants to override the tree.
        //        this will make it possible to open .ee archives while filtering and still see all the sub-files.
        if (has_search_terms() && filename.find(s_search_terms) == std::string::npos) {
            continue;
        }

        auto*       handler       = app.get_format_handler(extension_hash);
        bool        highlight     = false;
        bool        loaded        = false;
        const char* filetype_icon = ICON_FA_FILE;
        u32 flags = (ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);

        // the current file type wants to override the tree, reset flags and highlight
        if (handler) {
            filetype_icon = handler->get_filetype_icon();

            if (handler->is_loaded(path)) {
                loaded = true;
            }

            // directory highlight
            if (handler->wants_to_override_directory_tree() && loaded) {
                highlight = true;
                flags     = ImGuiTreeNodeFlags_SpanAvailWidth;
            }
        }

        // push highlight colour
        if (highlight) ImGui::PushStyleColor(ImGuiCol_Text, highlight_colour);
        if (loaded) ImGui::PushStyleColor(ImGuiCol_Text, loaded_colour);

        const bool tree_open = ImGui::TreeNodeEx(filename.c_str(), flags, "%s  %s", filetype_icon, filename.c_str());

        if (loaded) ImGui::PopStyleColor();

        set_tooltip(path, tooltip_prefix);
        app.get_ui().draw_context_menu(path, handler, UI::E_CONTEXT_FILE);

        // click handler
        if ((flags & ImGuiTreeNodeFlags_Leaf) && ImGui::IsItemClicked()) {
            // when and item is clicked while searching, open the directory tree up to the file
            // if (has_search_terms()) {
            //     open_folders_in_chain(this, this->path);
            // }

            // try get the handler from the file header magic, if we couldn't find one for the extension
            if (!handler) {
                handler = app.get_format_handler_for_file(path);
            }

            // TODO : if item is already loaded, just bring whatever window is rendering it to the front!

            if (handler && handler->load(path)) {
                // if the handler is overriding the directory tree, reset the items default state
                // NOTE : this is mainly to prevent a visual annoyance where if something gets unloaded after
                //        expanding the node, next time you click the leaf it will auto-close instead of opening.
                if (handler->wants_to_override_directory_tree()) {
                    ImGui::GetStateStorage()->SetBool(ImGui::GetID(filename.c_str()), false);
                }
            }
        }

        if (highlight) ImGui::Indent();

        // render handler
        if (tree_open && !(flags & ImGuiTreeNodeFlags_Leaf)) {
            if (handler) handler->directory_tree_handler(filename, path);
            ImGui::TreePop();
        }

        if (highlight) ImGui::Unindent();

        // pop highlight colour
        if (highlight) ImGui::PopStyleColor();
    }
}
} // namespace jcmr