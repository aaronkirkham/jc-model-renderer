#include "pch.h"

#include "resource_manager.h"

#include "app/app.h"
#include "app/directory_list.h"
#include "app/os.h"
#include "app/profile.h"

#include <AvaFormatLib/legacy/archive_table.h>
#include <AvaFormatLib/util/byte_array_buffer.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include <fstream>

namespace jcmr
{
struct ResourceManagerImpl final : ResourceManager {
  public:
    ResourceManagerImpl(App& app)
        : m_app(app)
    {
    }

    void set_base_path(const std::filesystem::path& base_path) override { m_base_path = base_path; }
    void set_flags(u32 flags) override { m_flags = flags; }

    void load_dictionary(i32 resource_id) override
    {
        LOG_INFO("ResourceManager : loading dictionary...");

        auto buffer = App::load_internal_resource(resource_id);
        ASSERT(!buffer.empty());

        byte_array_buffer buf(buffer);
        std::istream      buf_stream(&buf);

        rapidjson::IStreamWrapper stream(buf_stream);
        rapidjson::Document       document;
        document.ParseStream(stream);

        ASSERT(document.IsObject());

        m_dictionary.clear();
        m_dictionary.reserve(document.GetObjectA().MemberCount());

        for (auto iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter) {
            std::string name     = iter->name.GetString();
            const u32   namehash = ava::hashlittle(name.c_str());

            m_dictionary_tree.add(name);

            std::vector<std::string> paths;
            for (auto iter2 = iter->value.Begin(); iter2 != iter->value.End(); ++iter2) {
                paths.push_back(iter2->GetString());
            }

            m_dictionary.insert({namehash, std::make_pair(std::move(name), std::move(paths))});
        }

        LOG_INFO("ResourceManager : dictionary loaded. ({} entries)", m_dictionary.size());
    }

    DirectoryList& get_dictionary_tree() override { return m_dictionary_tree; }

    bool read(u32 namehash, ByteArray* out_buffer) override
    {
        ProfileBlock _("ResourceManager read");

        const auto& [archives, _namehash] = locate_in_dictionary(namehash);
        for (const auto& archive : archives) {
            LOG_INFO("ResourceManager : reading {:x} from archive \"{}\"...", namehash, archive);
            if (read_from_archive(archive.c_str(), namehash, out_buffer)) {
                return true;
            }
        }

        return false;
    }

    bool read(const std::string& filename, ByteArray* out_buffer) override
    {
        // pass to file handlers first
        auto& handlers = m_app.get_file_read_handlers();
        for (auto handler : handlers) {
            if (handler(filename, out_buffer)) {
                return true;
            }
        }

        return read(ava::hashlittle(filename.c_str()), out_buffer);
    }

    bool read_from_disk(const std::string& filename, ByteArray* out_buffer) override
    {
        auto filepath = m_base_path / filename;

        const auto size = std::filesystem::file_size(filepath);
        *out_buffer     = std::move(os::map_view_of_file(filepath.generic_string().c_str(), 0, size));
        return !out_buffer->empty();
    }

  private:
    using DictionaryLookupResult = std::tuple<std::vector<std::string>, u32>;
    using TabEntries             = std::vector<ava::ArchiveTable::TabEntry>;
    using LegacyTabEntries       = std::vector<ava::legacy::ArchiveTable::TabEntry>;
    using CompressionBlocks      = std::vector<ava::ArchiveTable::TabCompressedBlock>;

    DictionaryLookupResult locate_in_dictionary(u32 namehash)
    {
        auto iter = m_dictionary.find(namehash);
        if (iter == m_dictionary.end()) {
            return {};
        }

        const auto& entry = (*iter);
        return {entry.second.second, entry.first};
    }

    bool read_from_archive(const char* archive, u32 namehash, ByteArray* out_buffer)
    {
        auto tab_file = m_base_path / archive;
        tab_file += ".tab";

        auto arc_file = m_base_path / archive;
        arc_file += ".arc";

        if (!std::filesystem::exists(tab_file) || !std::filesystem::exists(arc_file)) {
            LOG_ERROR("ResourceManager : can't find arc/tab file. \"{}\" \"{}\"", tab_file.generic_string(),
                      arc_file.generic_string());
            return false;
        }

        ava::ArchiveTable::TabEntry entry{};
        CompressionBlocks           compression_blocks;

        // read entry from archive table
        if (!read_archive_table_entry(tab_file, namehash, &entry, &compression_blocks)) {
            LOG_ERROR("ResourceManager : failed to read archive table entry.");
            return false;
        }

        if (entry.m_Size == 0) {
            LOG_WARNING("ResourceManager : entry {:x} is empty (zero size)", entry.m_NameHash);
            return false;
        }

        // TODO : just use ReadEntryBuffer for all of this???
        // ArchiveTable::ReadEntryBuffer();

        auto buffer = os::map_view_of_file(arc_file.string().c_str(), entry.m_Offset, entry.m_Size);

        // file is not compressed
        if (entry.m_Library == ava::ArchiveTable::E_COMPRESS_LIBRARY_NONE) {
            *out_buffer = std::move(buffer);
            return !out_buffer->empty();
        }

        ASSERT(entry.m_Size != entry.m_UncompressedSize);

        // TODO : this might be wrong? should we map_view_of_file with the required_size instead?
        // NOTE : looks like on everything I've tested, the required buffer size is stored in m_Size when compression is
        //        used so we should be fine. to be safe, let's keep the ASSERT here.
        auto required_size = ava::ArchiveTable::GetEntryRequiredBufferSize(entry, compression_blocks);
        ASSERT(entry.m_Size == required_size);

        LOG_INFO("ResourceManager : archive is compressed! (size={}, uncompressed_size={}, required_size={})",
                 entry.m_Size, entry.m_UncompressedSize, required_size);

        AVA_FL_ENSURE(ava::ArchiveTable::DecompressEntryBuffer(buffer, entry, out_buffer, compression_blocks), false);
        return !out_buffer->empty();
    }

    bool read_archive_table(const std::filesystem::path& filename, TabEntries* out_entries,
                            CompressionBlocks* out_compression_blocks)
    {
        std::ifstream stream(filename, std::ios::binary);
        ASSERT(!stream.fail());

        const auto size = std::filesystem::file_size(filename);

        ByteArray buffer(size);
        stream.read((char*)buffer.data(), size);
        stream.close();

        // doesn't use legacy archive table
        if (!(m_flags & Flags::E_FLAG_LEGACY_ARCHIVE_TABLE)) {
            AVA_FL_ENSURE(ava::ArchiveTable::Parse(buffer, out_entries, out_compression_blocks), false);
            return true;
        }

        LegacyTabEntries entries;
        AVA_FL_ENSURE(ava::legacy::ArchiveTable::Parse(buffer, &entries), false);

        // convert legacy tab entries
        out_entries->reserve(entries.size());
        for (const auto& entry : entries) {
            // ava::ArchiveTable::TabEntry _entry = {
            //     .m_NameHash = entry.m_NameHash,
            //     .m_Offset = entry.m_Offset,
            //     .m_Size = entry.m_Size,
            //     .m_UncompressedSize = entry.m_Size;
            // };

            ava::ArchiveTable::TabEntry _entry{};
            _entry.m_NameHash         = entry.m_NameHash;
            _entry.m_Offset           = entry.m_Offset;
            _entry.m_Size             = entry.m_Size;
            _entry.m_UncompressedSize = entry.m_Size;
            out_entries->push_back(std::move(_entry));
        }

        return true;
    }

    bool read_archive_table_entry(const std::filesystem::path& filename, u32 namehash,
                                  ava::ArchiveTable::TabEntry* out_entry, CompressionBlocks* out_compression_blocks)
    {
        TabEntries entries;
        if (!read_archive_table(filename, &entries, out_compression_blocks)) {
            return false;
        }

        auto it = std::find_if(entries.begin(), entries.end(), [namehash](const ava::ArchiveTable::TabEntry& entry) {
            return entry.m_NameHash == namehash;
        });

        if (it == entries.end()) {
            return false;
        }

        *out_entry = (*it);
        return true;
    }

  private:
    using FileListDictionary = std::unordered_map<u32, std::pair<std::string, std::vector<std::string>>>;

    App&                  m_app;
    std::filesystem::path m_base_path;
    u32                   m_flags = 0;
    FileListDictionary    m_dictionary;
    DirectoryList         m_dictionary_tree;
};

ResourceManager* ResourceManager::create(App& app)
{
    return new ResourceManagerImpl(app);
}

void ResourceManager::destroy(ResourceManager* instance)
{
    delete instance;
}
} // namespace jcmr