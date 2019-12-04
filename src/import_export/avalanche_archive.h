#pragma once

#include "iimportexporter.h"

#include "game/file_loader.h"
#include "game/formats/avalanche_archive.h"

#include "../vendor/ava-format-lib/include/archives/stream_archive.h"

namespace ImportExport
{
class AvalancheArchive : public IImportExporter
{
  public:
    AvalancheArchive()          = default;
    virtual ~AvalancheArchive() = default;

    ImportExportType GetType() override final
    {
        return ImportExportType_Exporter;
    }

    const char* GetImportName() override final
    {
        return "Avalanche Archive Format";
    }

    std::vector<std::string> GetImportExtension() override final
    {
        return {".ee", ".bl", ".nl", ".fl"};
    }

    const char* GetExportName() override final
    {
        return "Avalanche Archive";
    }

    const char* GetExportExtension() override final
    {
        return "/";
    }

    void WriteArchiveFiles(const std::filesystem::path& path, ::AvalancheArchive* archive)
    {
        if (!archive) {
            return;
        }

        try {
            // create the directory if we need to
            if (!std::filesystem::exists(path)) {
                std::filesystem::create_directory(path);
            }

            // write files
            for (const auto& entry : archive->GetEntries()) {
                const auto& file_path = path / entry.m_Filename;

                if (entry.m_Offset != 0 && entry.m_Offset != -1) {
                    std::vector<uint8_t> buffer;
                    ava::StreamArchive::ReadEntry(archive->GetBuffer(), entry, &buffer);
                    WriteBufferToFile(file_path, buffer);
                }
                // read patched files directly from the table archive
                else {
                    FileLoader::Get()->ReadFileBatched(entry.m_Filename,
                                                       [&, file_path](bool success, std::vector<uint8_t> buffer) {
                                                           if (success) {
                                                               WriteBufferToFile(file_path, buffer);
                                                           }
                                                       });
                }
            }

            FileLoader::Get()->RunFileBatches();
        } catch (const std::exception&) {
        }
    }

    void Import(const std::filesystem::path& filename, ImportFinishedCallback callback) override final {}

    void Export(const std::filesystem::path& filename, const std::filesystem::path& to,
                ExportFinishedCallback callback) override final
    {
        const auto& path = to / filename.stem();

        auto archive = ::AvalancheArchive::get(filename.string());
        if (archive) {
            WriteArchiveFiles(path, archive.get());
            callback(true);
        } else {
            FileLoader::Get()->ReadFile(filename,
                                        [&, filename, path, callback](bool success, std::vector<uint8_t> data) {
                                            if (success) {
                                                auto arc = new ::AvalancheArchive(filename);
                                                arc->Parse(data, [&, path, callback, arc](bool success) {
                                                    if (success) {
                                                        WriteArchiveFiles(path, arc);
                                                    }

                                                    delete arc;
                                                    callback(success);
                                                });
                                            } else {
                                                callback(false);
                                            }
                                        });
        }
    }
};
}; // namespace ImportExport
