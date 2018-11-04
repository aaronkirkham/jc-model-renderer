#pragma once

#include "IImportExporter.h"

namespace import_export
{

class AvalancheArchive : public IImportExporter
{
  public:
    AvalancheArchive()          = default;
    virtual ~AvalancheArchive() = default;

    ImportExportType GetType() override final
    {
        return IE_TYPE_EXPORTER;
    }

    const char* GetName() override final
    {
        return "Avalanche Archive";
    }

    std::vector<const char*> GetImportExtension() override final
    {
        return {".ee", ".bl", ".nl", ".fl"};
    }

    const char* GetExportExtension() override final
    {
        return "/";
    }

    void WriteArchiveFiles(const fs::path& path, ::AvalancheArchive* archive)
    {
        assert(archive);
        assert(archive->GetStreamArchive());

        // create the directory if we need to
        if (!fs::exists(path)) {
            fs::create_directory(path);
        }

        // write files
        const auto sarc = archive->GetStreamArchive();
        for (const auto& entry : sarc->m_Files) {
            if (entry.m_Offset != 0 && entry.m_Offset != -1) {
                const auto& file_path = path / entry.m_Filename;
                const auto& bytes     = sarc->GetEntryBuffer(entry);
                WriteBufferToFile(file_path, &bytes);
            } else if (entry.m_Offset == -1) {
                // TODO: load external files
                // we want to export the latest files and make sure to include files which are inside patch_ archives
            }
        }
    }

    void Import(const fs::path& filename, ImportFinishedCallback callback) override final
    {
        //
    }

    void Export(const fs::path& filename, const fs::path& to, ExportFinishedCallback callback) override final
    {
        const auto& path = to / filename.stem();
        DEBUG_LOG("AvalancheArchive::Export - Exporting archive to '" << path << "'...");

        auto archive = ::AvalancheArchive::get(filename.string());
        if (archive) {
            WriteArchiveFiles(path, archive.get());
            callback(true);
        } else {
            FileLoader::Get()->ReadFile(filename, [&, filename, path, callback](bool success, FileBuffer data) {
                if (success) {
                    auto arc = std::make_unique<::AvalancheArchive>(filename, data);
                    WriteArchiveFiles(path, arc.get());
                }

                callback(success);
            });
        }
    }
};

}; // namespace import_export
