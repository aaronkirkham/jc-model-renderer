#pragma once

#include "iimportexporter.h"
#include "util.h"

#include "../vendor/ava-format-lib/include/util/byte_array_buffer.h"

#include "game/name_hash_lookup.h"

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

        auto path      = filename.parent_path();
        auto file_name = filename.filename().string();
        util::replace(file_name, "_resourcebundle", "");
        file_name += ".resourcebundle";

        try {
            std::vector<uint8_t> out_buffer;

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
                FileBuffer    in_buffer(file_size);
                std::ifstream in_stream(current_filename, std::ios::binary);
                in_stream.read((char*)in_buffer.data(), file_size);
                in_stream.close();

                // @TODO: fix filename if a hex string is passed!
                // const uint32_t path_hash = (!is_number(filename_str) ? hashlittle(filename_str.c_str()) :
                // std::stoul(filename_str));

                ava::ResourceBundle::WriteEntry(&out_buffer, current_filename, in_buffer);
            }

            std::ofstream stream(path / file_name, std::ios::binary);
            stream.write((char*)out_buffer.data(), out_buffer.size());
            stream.close();

            if (callback) {
                callback(true, filename, {});
            }
        } catch (const std::exception&) {
            if (callback) {
                callback(false, filename, {});
            }
        }
    }

    void Export(const std::filesystem::path& filename, const std::filesystem::path& to,
                ExportFinishedCallback callback) override final
    {
        using namespace ava::ResourceBundle;

        const auto& path = to / (filename.stem().string() + "_resourcebundle");
        SPDLOG_INFO("Exporting resource bundle to \"{}\"", path.string());

        // create the directory if we need to
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directory(path);
        }

        FileLoader::Get()->ReadFile(filename, [&, filename, path, callback](bool success, std::vector<uint8_t> data) {
            if (success) {
                byte_array_buffer buf(data);
                std::istream      stream(&buf);

                // @TODO: probably should do a better way.
                while ((size_t)stream.tellg() < data.size()) {
                    ResourceEntry entry{};
                    stream.read((char*)&entry, sizeof(ResourceEntry));

                    std::vector<uint8_t> file_buffer(entry.m_Size);
                    stream.read((char*)file_buffer.data(), entry.m_Size);

                    const auto&           extension = NameHashLookup::GetName(entry.m_ExtensionHash);
                    std::filesystem::path filename  = path / (std::to_string(entry.m_NameHash) + extension);

                    std::ofstream stream(filename, std::ios::binary);
                    if (!stream.fail()) {
                        stream.write((char*)file_buffer.data(), file_buffer.size());
                        stream.close();
                    }
                }
            }

            callback(success);
        });
    }
};
}; // namespace ImportExport
