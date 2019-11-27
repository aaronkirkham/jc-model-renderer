#pragma once

#include "iimportexporter.h"

#include "../vendor/ava-format-lib/include/archives/archive_table.h"

namespace ImportExport
{
class ArchiveTable : public IImportExporter
{
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

    void Import(const std::filesystem::path& filename, ImportFinishedCallback callback) override final;

    void Export(const std::filesystem::path& filename, const std::filesystem::path& to,
                ExportFinishedCallback callback) override final
    {
        const auto& path = to / (filename.stem().string() + "_unpack");

        // create the directory if we need to
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directory(path);
        }
    }
};
}; // namespace ImportExport
