#pragma once

#include "iimportexporter.h"

namespace ImportExport
{
class RTPC2XML : public IImportExporter
{
  public:
    RTPC2XML()          = default;
    virtual ~RTPC2XML() = default;

    ImportExportType GetType() override final
    {
        return ImportExportType_Both;
    }

    const char* GetImportName() override final
    {
        return "Runtime Property Container";
    }

    std::vector<std::string> GetImportExtension() override final
    {
        return {".epe", ".blo"};
    }

    const char* GetExportName() override final
    {
        return "XML";
    }

    const char* GetExportExtension() override final
    {
        return ".xml";
    }

    bool IsDragDropImportable() override final
    {
        return true;
    }

    void Import(const std::filesystem::path& filename, ImportFinishedCallback callback) override final
    {
        return;
    }

    void Export(const std::filesystem::path& filename, const std::filesystem::path& to,
                ExportFinishedCallback callback) override final
    {
        return;
    }
};
}; // namespace ImportExport
