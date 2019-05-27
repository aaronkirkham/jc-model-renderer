#pragma once

#include "iimportexporter.h"

#include "../window.h"

#include "../game/file_loader.h"
#include "../game/name_hash_lookup.h"

namespace ImportExport
{

class ResourceBundle : public IImportExporter
{
  public:
    ResourceBundle()          = default;
    virtual ~ResourceBundle() = default;

    ImportExportType GetType() override final
    {
        return ImportExportType_Both;
    }

    const char* GetImportName() override final
    {
        return "Resource Bundle";
    }

    std::vector<std::string> GetImportExtension() override final
    {
        return {".resourcebundle"};
    }

    const char* GetExportName() override final
    {
        return "Resource Bundle";
    }

    const char* GetExportExtension() override final
    {
        return "/";
    }

    void Import(const std::filesystem::path& filename, ImportFinishedCallback callback) override final
    {
        // not implemented
    }

    void Export(const std::filesystem::path& filename, const std::filesystem::path& to,
                ExportFinishedCallback callback) override final
    {
        const auto& path = to / (filename.stem().string() + "_resourcebundle");
        SPDLOG_INFO("Exporting resource bundle to \"{}\"", path.string());

        // create the directory if we need to
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directory(path);
        }

        FileLoader::Get()->ReadFile(filename, [&, filename, path, callback](bool success, FileBuffer data) {
            if (success) {
                uint64_t current_read_offset = 0;
                while (current_read_offset < data.size()) {
                    const uint32_t path_hash      = *(uint32_t*)&data[current_read_offset];
                    const uint32_t extension_hash = *(uint32_t*)&data[current_read_offset + 4];
                    const uint32_t file_size      = *(uint32_t*)&data[current_read_offset + 8];

                    FileBuffer file_buffer(file_size);
                    std::memcpy(file_buffer.data(), &data[current_read_offset + 12], file_size);

                    const auto&           extension = NameHashLookup::GetName(extension_hash);
                    std::filesystem::path filename  = path / (std::to_string(path_hash) + extension);

                    std::ofstream stream(filename, std::ios::binary);
                    if (!stream.fail()) {
                        stream.write((char*)file_buffer.data(), file_buffer.size());
                        stream.close();
                    }

                    // NOTE: file size + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t)
                    current_read_offset += (file_size + 12);
                }
            }

            callback(success);
        });
    }
};

}; // namespace ImportExport
