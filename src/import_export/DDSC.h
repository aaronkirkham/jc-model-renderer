#pragma once

#include "IImportExporter.h"

namespace import_export
{

class DDSC : public IImportExporter
{
public:
    DDSC() = default;
    virtual ~DDSC() = default;

    ImportExportType GetType() override final { return IE_TYPE_EXPORTER; }
    const char* GetName() override final { return "DirectDraw Surface"; }
    std::vector<const char*> GetInputExtensions() override final { return { ".dds", ".ddsc", ".hmddsc" }; }
    const char* GetOutputExtension() override final { return ".dds"; }

    void Export(const fs::path& filename, const fs::path& to, ImportExportFinishedCallback callback) override final
    {
        auto& path = to / filename.stem(); path += ".dds";
        DEBUG_LOG("DDSC::Export - Exporting texture to '" << path << "'...");

        FileLoader::Get()->ReadFile(filename, [&, path, callback](bool success, FileBuffer data) {
            if (success) {
                WriteBufferToFile(path, &data);
            }

            callback(success);
        });
    }
};

};