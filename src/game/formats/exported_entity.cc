#include "pch.h"

#include "exported_entity.h"

#include "app/app.h"
#include "app/directory_list.h"
#include "app/log.h"
#include "app/profile.h"

#include "game/game.h"
#include "game/resource_manager.h"

#include "render/fonts/icons.h"
#include "render/imguiex.h"
#include "render/renderer.h"

#include <imgui.h>

#include <chrono>
#include <mutex>
#include <queue>
#include <thread>

namespace jcmr::game::format
{
struct ExportedEntityArchive {
    using ArchiveEntries = std::vector<ava::StreamArchive::ArchiveEntry>;

    ExportedEntityArchive() = default;
    ExportedEntityArchive(const ByteArray& buffer, ArchiveEntries& entries)
        : buffer(buffer)
        , entries(entries)
    {
        build_tree();
    }

    void build_tree()
    {
        for (auto& entry : entries) {
            tree.add(entry.m_Filename);
        }

        tree.sort();
    }

    bool read_entry(ava::StreamArchive::ArchiveEntry& entry, ByteArray* out_buffer)
    {
        if (buffer.empty()) return false;
        return ava::StreamArchive::ReadEntry(buffer, entry, out_buffer) == ava::Result::E_OK;
    }

    bool read_entry(const std::string& filename, ByteArray* out_buffer)
    {
        if (buffer.empty()) return false;
        return ava::StreamArchive::ReadEntry(buffer, entries, filename, out_buffer) == ava::Result::E_OK;
    }

    ByteArray      buffer;
    DirectoryList  tree;
    ArchiveEntries entries;
};

struct ExportedEntityImpl final : ExportedEntity {
    ExportedEntityImpl(App& app)
        : m_app(app)
    {
        // start worker thread for archive decompression
        std::thread worker_thread(&ExportedEntityImpl::decompression_worker_thread, this);
        worker_thread.detach();

        // middleware resource manager read handler
        app.register_file_read_handler([this](const std::string& filename, ByteArray* out_buffer) {
            for (auto& archive : m_archives) {
                if (archive.second.read_entry(filename, out_buffer)) {
                    LOG_INFO("ExportedEntity : \"{}\" read from archive. ({} bytes read)", filename,
                             out_buffer->size());
                    return true;
                }
            }

            return false;
        });
    }

    void update() override
    {
        std::lock_guard<decltype(m_decompression_mutex)> _lock(m_decompression_mutex);
        if (m_decompression_finished_queue.empty()) {
            return;
        }

        auto& [filename, success, decompressed_buffer] = m_decompression_finished_queue.front();
        if (success) {
            parse_archive_entries(filename, decompressed_buffer);
        } else {
            unload(filename);
        }

        m_decompression_finished_queue.pop();
    }

    bool load(const std::string& filename) override
    {
        if (is_loaded(filename)) return true;

        LOG_INFO("ExportedEntity : loading \"{}\"...", filename);

        auto* resource_manager = m_app.get_game()->get_resource_manager();

        ByteArray buffer;
        if (!resource_manager->read(filename, &buffer)) {
            LOG_ERROR("ExportedEntity : failed to load file.");
            return false;
        }

        // if the archive is compressed, queue it for decompression
        if (ava::AvalancheArchiveFormat::IsCompressed(buffer)) {
            std::lock_guard<decltype(m_decompression_mutex)> _lock(m_decompression_mutex);
            m_decompression_queue.push(std::make_pair(filename, std::move(buffer)));
            m_archives.insert({filename, ExportedEntityArchive()});
            return true;
        }

        return parse_archive_entries(filename, buffer);
    }

    void unload(const std::string& filename) override
    {
        auto iter = m_archives.find(filename);
        if (iter == m_archives.end()) return;
        m_archives.erase(iter);
    }

    bool save(const std::string& filename, ByteArray* out_buffer) override
    {
        //
        return false;
    }

    bool is_loaded(const std::string& filename) const override { return m_archives.find(filename) != m_archives.end(); }

    void directory_tree_handler(const std::string& filename, const std::string& path) override
    {
        auto iter = m_archives.find(path);
        if (iter == m_archives.end()) return;

        (*iter).second.tree.render(m_app, filename);

        // archive has empty buffer, probably loading?
        if ((*iter).second.buffer.empty()) {
            ImGui::Indent();
            ImGui::PushStyleColor(ImGuiCol_Text, {0.53f, 0.53f, 0.53f, 1.0f});
            ImGuiEx::SpinningIconLabel(ICON_FA_SPINNER, "Loading...");
            ImGui::PopStyleColor();
            ImGui::Unindent();
        }
    }

    void context_menu_handler(const std::string& filename) override
    {
        if (!is_loaded(filename)) return;

        ImGui::Separator();

        if (ImGui::Selectable(ICON_FA_WINDOW_CLOSE " Close archive")) {
            unload(filename);
        }
    }

    bool export_to(const std::string& filename, const std::filesystem::path& path) override
    {
        LOG_INFO("ExportedEntity -> export_to({}, {})", filename, path.generic_string());
        if (!load_slow_no_decompression_queue(filename)) return false;

        // get the loaded archive
        auto iter = m_archives.find(filename);
        if (iter == m_archives.end()) return false;

        const auto& export_path = path / std::filesystem::path(filename).stem();
        std::filesystem::create_directories(export_path);

        // write files from entry
        for (auto& entry : (*iter).second.entries) {
            ByteArray buffer;
            if (!(*iter).second.read_entry(entry, &buffer)) {
                LOG_WARNING("ExportedEntity : failed to read \"{}\" from archive.", entry.m_Filename);
                continue;
            }

            const auto& export_filename = export_path / entry.m_Filename;
            std::filesystem::create_directories(export_filename.parent_path());

            // LOG_INFO("export file {}", export_filename.generic_string());

            if (!m_app.write_binary_file(export_filename, buffer)) {
                LOG_WARNING("ExportedEntity : failed to write \"{}\" from archive.", entry.m_Filename);
                continue;
            }
        }

        m_archives.erase(iter);
        return true;
    }

  private:
    void decompression_worker_thread()
    {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            std::string filename;
            ByteArray   compressed_buffer;

            // get first entry in queue
            {
                std::lock_guard<decltype(m_decompression_mutex)> _lock(m_decompression_mutex);
                if (m_decompression_queue.empty()) continue;
                std::tie(filename, compressed_buffer) = m_decompression_queue.front();
            }

            // decompress buffer
            ByteArray decompressed_buffer;
            auto      success =
                AVA_FL_SUCCEEDED(ava::AvalancheArchiveFormat::Decompress(compressed_buffer, &decompressed_buffer));

            // push into decompress complete queue
            {
                std::lock_guard<decltype(m_decompression_mutex)> _lock(m_decompression_mutex);
                m_decompression_finished_queue.push(std::make_tuple(filename, success, std::move(decompressed_buffer)));
                m_decompression_queue.pop();
            }
        }
    }

    bool parse_archive_entries(const std::string& filename, const ByteArray& buffer)
    {
        auto* resource_manager = m_app.get_game()->get_resource_manager();

        std::vector<ava::StreamArchive::ArchiveEntry> entries;
        AVA_FL_ENSURE(ava::StreamArchive::Parse(buffer, &entries), false);

        // attempt to patch from TOC
        const auto toc_filename = (filename + ".toc");
        ByteArray  toc_buffer;
        if (resource_manager->read(toc_filename, &toc_buffer)) {
            u32 num_added   = 0;
            u32 num_patched = 0;
            AVA_FL_ENSURE(ava::StreamArchive::ParseTOC(toc_buffer, &entries, &num_added, &num_patched), false);
            LOG_INFO("ExportedEntity : added {} and patched {} entries from \"{}\".", num_added, num_patched,
                     toc_filename);
        }

        // if the archive already exists, set the buffer and entries
        auto iter = m_archives.find(filename);
        if (iter != m_archives.end()) {
            auto& archive   = (*iter).second;
            archive.buffer  = std::move(buffer);
            archive.entries = std::move(entries);
            archive.build_tree();
            return true;
        }

        m_archives.insert({filename, ExportedEntityArchive(std::move(buffer), std::move(entries))});
        return true;
    }

    bool load_slow_no_decompression_queue(const std::string& filename)
    {
        if (is_loaded(filename)) return true;

        LOG_INFO("ExportedEntity : loading (slow mode) \"{}\"...", filename);

        auto* resource_manager = m_app.get_game()->get_resource_manager();

        ByteArray buffer;
        if (!resource_manager->read(filename, &buffer)) {
            LOG_ERROR("ExportedEntity : failed to load file.");
            return false;
        }

        if (ava::AvalancheArchiveFormat::IsCompressed(buffer)) {
            // decompress buffer
            ByteArray decompressed_buffer;
            auto      success = AVA_FL_SUCCEEDED(ava::AvalancheArchiveFormat::Decompress(buffer, &decompressed_buffer));
            ASSERT(success);
            return parse_archive_entries(filename, decompressed_buffer);
        }

        return parse_archive_entries(filename, buffer);
    }

  private:
    App&                                                   m_app;
    std::unordered_map<std::string, ExportedEntityArchive> m_archives;
    std::mutex                                             m_decompression_mutex;
    std::queue<std::pair<std::string, ByteArray>>          m_decompression_queue;
    std::queue<std::tuple<std::string, bool, ByteArray>>   m_decompression_finished_queue;
};

ExportedEntity* ExportedEntity::create(App& app)
{
    return new ExportedEntityImpl(app);
}

void ExportedEntity::destroy(ExportedEntity* instance)
{
    delete instance;
}
} // namespace jcmr::game::format