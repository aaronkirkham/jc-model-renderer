#pragma once

#include "iimportexporter.h"

#include "../window.h"

#include "../game/file_loader.h"
#include "../game/name_hash_lookup.h"

namespace ImportExport
{

class ArchiveTable : public IImportExporter
{
  private:
    bool is_number(const std::string& string)
    {
        return !string.empty() && std::all_of(string.begin(), string.end(), std::isdigit);
    }

  public:
    ArchiveTable()          = default;
    virtual ~ArchiveTable() = default;

    ImportExportType GetType() override final
    {
        return ImportExportType_Importer;
    }

    const char* GetImportName() override final
    {
        return "Archive Table";
    }

    std::vector<std::string> GetImportExtension() override final
    {
        return {".tab", ".arc"};
    }

    const char* GetExportName() override final
    {
        return "Archive Table";
    }

    const char* GetExportExtension() override final
    {
        return "/";
    }

    void Import(const std::filesystem::path& filename, ImportFinishedCallback callback) override final
    {
        if (!std::filesystem::is_directory(filename)) {
            if (callback) {
                callback(false, filename, {});
            }

            return;
        }

        auto path         = filename.parent_path();
        auto arc_filename = path / filename.filename().replace_extension(".arc");
        auto tab_filename = path / filename.filename().replace_extension(".tab");

        std::ofstream arc(arc_filename, std::ios::binary);
        if (arc.fail()) {
            if (callback) {
                callback(false, filename, {});
            }

            return;
        }

        std::ofstream tab(tab_filename, std::ios::binary);
        if (tab.fail()) {
            if (callback) {
                callback(false, filename, {});
            }

            return;
        }

        std::vector<jc4::ArchiveTable::VfsTabEntry> entries;
        uint32_t                                    total_size = 0;

        // read all input files
        for (const auto& filepath : std::filesystem::recursive_directory_iterator(filename)) {
            // skip none regular files
            if (!std::filesystem::is_regular_file(filepath)) {
                continue;
            }

            const auto&    current_filename = filepath.path();
            const uint32_t file_size        = filepath.file_size();

            const auto&    name     = current_filename.lexically_relative(filename);
            const uint32_t namehash = hashlittle(name.generic_string().c_str());

            SPDLOG_DEBUG(name.generic_string());

            // read the input file buffer
            FileBuffer    in_buffer(file_size);
            std::ifstream in_stream(current_filename, std::ios::binary);
            in_stream.read((char*)in_buffer.data(), file_size);
            in_stream.close();

            // write the input file buffer to the arc buffer
            arc.write((char*)in_buffer.data(), file_size);

            // generate the tab entry
            jc4::ArchiveTable::VfsTabEntry entry;
            entry.m_NameHash             = namehash;
            entry.m_Offset               = total_size;
            entry.m_CompressedSize       = file_size;
            entry.m_UncompressedSize     = file_size;
            entry.m_CompressedBlockIndex = 0;
            entry.m_CompressionType      = jc4::ArchiveTable::CompressionType_None;
            entry.m_Flags                = 0;
            entries.push_back(std::move(entry));

            //
            total_size += file_size;
        }

        // write header
        jc4::ArchiveTable::TabFileHeader header;
        tab.write((char*)&header, sizeof(header));

        // write compressed block count
        uint32_t                                 compressed_block_count = 2;
        jc4::ArchiveTable::VfsTabCompressedBlock compressed_block{0xFFFFFFFF, 0xFFFFFFFF};
        tab.write((char*)&compressed_block_count, sizeof(compressed_block_count));
        tab.write((char*)&compressed_block, sizeof(compressed_block));
        tab.write((char*)&compressed_block, sizeof(compressed_block));

        // write the tab entries
        tab.write((char*)entries.data(), (sizeof(jc4::ArchiveTable::VfsTabEntry) * entries.size()));

        tab.close();
        arc.close();

        if (callback) {
            callback(true, filename, {});
        }
    }

    void Export(const std::filesystem::path& filename, const std::filesystem::path& to,
                ExportFinishedCallback callback) override final
    {
        const auto& path = to / (filename.stem().string() + "_unpack");
        SPDLOG_INFO("Exporting archive table to \"{}\"", path.string());

        // create the directory if we need to
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directory(path);
        }
    }
};

}; // namespace ImportExport
