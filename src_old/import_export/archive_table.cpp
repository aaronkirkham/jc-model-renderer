#include <AvaFormatLib.h>

#include "../vendor/ava-format-lib/include/archives/legacy/archive_table.h"

#include "archive_table.h"

extern bool g_IsJC4Mode;

namespace ImportExport
{
void ArchiveTable::Import(const std::filesystem::path& filename, ImportFinishedCallback callback)
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

    try {
        std::vector<uint8_t> tab_buffer;
        std::vector<uint8_t> arc_buffer;

        // read all input files
        for (const auto& filepath : std::filesystem::recursive_directory_iterator(filename)) {
            // skip none regular files
            if (!std::filesystem::is_regular_file(filepath)) {
                continue;
            }

            const auto&    current_filename = filepath.path();
            const auto&    name             = current_filename.lexically_relative(filename);
            const uint32_t file_size        = filepath.file_size();

            // read the input file buffer
            std::vector<uint8_t> in_buffer(file_size);
            std::ifstream        in_stream(current_filename, std::ios::binary);
            in_stream.read((char*)in_buffer.data(), file_size);
            in_stream.close();

            // write the archive table entry
            if (g_IsJC4Mode) {
                ava::ArchiveTable::WriteEntry(&tab_buffer, &arc_buffer, name.generic_string(), in_buffer);
            } else {
                ava::legacy::ArchiveTable::WriteEntry(&tab_buffer, &arc_buffer, name.generic_string(), in_buffer);
            }
        }

        // write tab file
        std::ofstream tab(tab_filename, std::ios::binary);
        tab.write((char*)tab_buffer.data(), tab_buffer.size());
        tab.close();

        // write arc file
        std::ofstream arc(arc_filename, std::ios::binary);
        arc.write((char*)arc_buffer.data(), arc_buffer.size());
        arc.close();

        if (callback) {
            callback(true, filename, {});
        }
    } catch (const std::exception&) {
        if (callback) {
            callback(false, filename, {});
        }
    }
}
} // namespace ImportExport
