#include "pch.h"

#include "resource_bundle.h"

#include "app/app.h"
#include "app/directory_list.h"
#include "app/utils.h"

#include "game/game.h"
#include "game/name_hash_lookup.h"
#include "game/resource_manager.h"

#include "render/imguiex.h"
#include "render/ui.h"

#include <imgui.h>

namespace jcmr::game::format
{
struct ResourceBundleObject {
    ResourceBundleObject() = delete;
    ResourceBundleObject(const std::vector<ava::ResourceBundle::ResourceEntry>& entries,
                         const std::vector<ByteArray>&                          buffers)
        : entries(std::move(entries))
        , buffers(std::move(buffers))
    {
        for (const auto& entry : entries) {
            auto name = std::string(find_in_namehash_lookup_table(entry.m_NameHash));

            // generate name if empty
            if (name.empty()) {
                name = fmt::format("{:#04x}", entry.m_NameHash);

                // append extension if we know the hash
                auto extension = find_in_namehash_lookup_table(entry.m_ExtensionHash);
                if (strlen(extension) > 0) {
                    name += extension;
                }
            }

            filename_lookup.insert({name, entry.m_NameHash});
            tree.add(name);
        }

        tree.sort();
    }

    bool read_entry(const std::string& filename, ByteArray* out_buffer) const
    {
        LOG_INFO("reading entry {} from resource bundle...", filename);

        // because we re-map the filename in the tree, we need to lookup.
        auto it = filename_lookup.find(filename);
        if (it == filename_lookup.end()) return false;

        const auto entry_namehash = (*it).second;

        // TODO : probably a better way of doing this instead of a 2nd find - can we just store a pointer on the
        // unordered_map?
        auto entry_it = std::find_if(entries.begin(), entries.end(),
                                     [entry_namehash](const ava::ResourceBundle::ResourceEntry& entry) {
                                         return entry.m_NameHash == entry_namehash;
                                     });
        if (entry_it == entries.end()) return false;

        auto index = std::distance(entries.begin(), entry_it);
        ASSERT(index <= buffers.size());
        *out_buffer = buffers[index];
        return true;
    }

    std::unordered_map<std::string, uint32_t>       filename_lookup;
    std::vector<ava::ResourceBundle::ResourceEntry> entries;
    std::vector<ByteArray>                          buffers;
    DirectoryList                                   tree;
};

struct ResourceBundleImpl final : ResourceBundle {
    ResourceBundleImpl(App& app)
        : m_app(app)
    {
        // middleware resource manager read handler
        app.register_file_read_handler([this](const std::string& filename, ByteArray* out_buffer) {
            for (const auto& resource_bundle : m_resource_bundles) {
                if (resource_bundle.second.read_entry(filename, out_buffer)) {
                    LOG_INFO("ResourceBundle : \"{}\" read from bundle. ({} bytes read)", filename, out_buffer->size());
                    return true;
                }
            }

            return false;
        });
    }

    bool load(const std::string& filename) override
    {
        auto* resource_manager = m_app.get_game()->get_resource_manager();

        ByteArray buffer;
        if (!resource_manager->read(filename, &buffer)) {
            return false;
        }

        std::vector<ava::ResourceBundle::ResourceEntry> entries;
        std::vector<ByteArray>                          buffers;
        AVA_FL_ENSURE(ava::ResourceBundle::ReadAllEntries(buffer, &entries, &buffers), false);

        ResourceBundleObject bundle(entries, buffers);
        m_resource_bundles.insert({filename, std::move(bundle)});
        return true;
    }

    void unload(const std::string& filename) override
    {
        auto iter = m_resource_bundles.find(filename);
        if (iter == m_resource_bundles.end()) return;
        m_resource_bundles.erase(iter);
    }

    bool save(const std::string& filename, ByteArray* out_buffer) override
    {
        // TODO
        return false;
    }

    bool is_loaded(const std::string& filename) const override
    {
        return m_resource_bundles.find(filename) != m_resource_bundles.end();
    }

    void directory_tree_handler(const std::string& filename, const std::string& path) override
    {
        auto iter = m_resource_bundles.find(path);
        if (iter == m_resource_bundles.end()) return;

        (*iter).second.tree.render(m_app, filename);
    }

    void context_menu_handler(const std::string& filename) override
    {
        if (!is_loaded(filename)) return;

        ImGui::Separator();

        if (ImGui::Selectable(ICON_FA_WINDOW_CLOSE " Close")) {
            unload(filename);
        }
    }

  private:
    App&                                                  m_app;
    std::unordered_map<std::string, ResourceBundleObject> m_resource_bundles;
};

ResourceBundle* ResourceBundle::create(App& app)
{
    return new ResourceBundleImpl(app);
}

void ResourceBundle::destroy(ResourceBundle* instance)
{
    delete instance;
}
} // namespace jcmr::game::format