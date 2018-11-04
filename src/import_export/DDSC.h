#pragma once

#include "IImportExporter.h"

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

    void Import(const fs::path& filename, ImportFinishedCallback callback) override final
    {
        //
    }

    void Export(const fs::path& filename, const fs::path& to, ExportFinishedCallback callback) override final
    {
        auto& path = to / filename.stem();
        path += GetExportExtension();
        DEBUG_LOG("DDSC::Export - Exporting texture to '" << path << "'...");

        FileLoader::Get()->ReadFile(filename, [&, path, callback](bool success, FileBuffer data) {
            if (success) {
                WriteBufferToFile(path, &data);
            }

            callback(success);
        });
    }
};

}; // namespace import_export
