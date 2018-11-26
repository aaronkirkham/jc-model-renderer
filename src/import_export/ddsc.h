#pragma once

#include "iimportexporter.h"

#include "../window.h"

#include "../jc3/file_loader.h"

namespace import_export
{

class DDSC : public IImportExporter
{
  public:
    DDSC()          = default;
    virtual ~DDSC() = default;

    ImportExportType GetType() override final
    {
        return IE_TYPE_EXPORTER;
    }

    const char* GetName() override final
    {
        return "DirectDraw Surface";
    }

    std::vector<const char*> GetImportExtension() override final
    {
        return {".ddsc", ".hmddsc"};
    }

    const char* GetExportExtension() override final
    {
        return ".dds";
    }

    void Import(const std::filesystem::path& filename, ImportFinishedCallback callback) override final
    {
        //
    }

    void Export(const std::filesystem::path& filename, const std::filesystem::path& to,
                ExportFinishedCallback callback) override final
    {
        auto& path = to / filename.stem();
        path += GetExportExtension();
        LOG_INFO("Exporting texture to \"{}\"", path.string());

        FileLoader::Get()->ReadFile(filename, [&, path, callback](bool success, FileBuffer data) {
            if (success) {
                WriteBufferToFile(path, &data);
            }

            callback(success);
        });
    }
};

}; // namespace import_export
