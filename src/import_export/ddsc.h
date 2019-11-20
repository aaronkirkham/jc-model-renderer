#pragma once

#include "iimportexporter.h"

#include "game/file_loader.h"

namespace ImportExport
{
class DDSC : public IImportExporter
{
  public:
    DDSC()          = default;
    virtual ~DDSC() = default;

    ImportExportType GetType() override final
    {
        return ImportExportType_Both;
    }

    const char* GetImportName() override final
    {
        return "Avalanche Texture";
    }

    std::vector<std::string> GetImportExtension() override final
    {
        return {".ddsc", ".hmddsc", ".atx1", ".atx2"};
    }

    const char* GetExportName() override final
    {
        return "DirectDraw Surface";
    }

    const char* GetExportExtension() override final
    {
        return ".dds";
    }

    void Import(const std::filesystem::path& filename, ImportFinishedCallback callback) override final
    {
        if (!std::filesystem::exists(filename)) {
            return callback(false, filename, 0);
        }

        std::ifstream stream(filename, std::ios::binary);
        if (stream.fail()) {
            return callback(false, filename, 0);
        }

        const auto size = std::filesystem::file_size(filename);

        std::vector<uint8_t> buffer(size);
        stream.read((char*)buffer.data(), size);
        stream.close();

        callback(true, filename, std::move(buffer));
    }

    void Export(const std::filesystem::path& filename, const std::filesystem::path& to,
                ExportFinishedCallback callback) override final
    {
        auto& path = to / filename.stem();
        path += GetExportExtension();

        FileLoader::Get()->ReadFile(filename, [&, path, callback](bool success, std::vector<uint8_t> data) {
            if (success) {
                WriteBufferToFile(path, data);
            }

            callback(success);
        });
    }
};
}; // namespace ImportExport
