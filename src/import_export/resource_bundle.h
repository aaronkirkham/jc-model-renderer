#pragma once

#include "iimportexporter.h"

#include "../window.h"

#include "../game/file_loader.h"
#include "../game/name_hash_lookup.h"

namespace ImportExport
{

class ResourceBundle : public IImportExporter
{
  private:
    bool is_number(const std::string& string)
    {
        return !string.empty() && std::all_of(string.begin(), string.end(), std::isdigit);
    }

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
        if (!std::filesystem::is_directory(filename)) {
            if (callback) {
                callback(false, filename, {});
            }

            return;
        }

        auto path = filename.parent_path();
        auto _fn  = filename.filename().string();
        util::replace(_fn, "_resourcebundle", "");
        _fn += ".resourcebundle";

        auto          out_filename = path / _fn;
        std::ofstream stream(out_filename, std::ios::binary);
        if (stream.fail()) {
            if (callback) {
                callback(false, filename, {});
            }

            return;
        }

        // add each file in the directory to the stream
        // TODO: maybe some sanitization? this is not recursive, so atleast we have that.
        for (const auto& filepath : std::filesystem::directory_iterator(filename)) {
            // skip none regular files
            if (!std::filesystem::is_regular_file(filepath)) {
                continue;
            }

            const auto&    current_filename = filepath.path();
            const uint32_t file_size        = filepath.file_size();

            // read the file buffer
            FileBuffer    buffer(file_size);
            std::ifstream in_stream(current_filename, std::ios::binary);
            if (in_stream.fail()) {
                continue;
            }

            in_stream.read((char*)buffer.data(), file_size);
            in_stream.close();

            // generate path and extension hashes
            const auto     filename_str = current_filename.stem().string();
            const uint32_t path_hash =
                (!is_number(filename_str) ? hashlittle(filename_str.c_str()) : std::stoul(filename_str));
            const uint32_t extension_hash = hashlittle(current_filename.extension().string().c_str());

            // write file info
            stream.write((char*)&path_hash, sizeof(path_hash));
            stream.write((char*)&extension_hash, sizeof(extension_hash));
            stream.write((char*)&file_size, sizeof(file_size));
            stream.write((char*)buffer.data(), file_size);
        }

        stream.close();
        if (callback) {
            callback(true, filename, {});
        }
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
