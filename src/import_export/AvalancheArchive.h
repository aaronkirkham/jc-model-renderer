#pragma once

#include "IImportExporter.h"

namespace import_export
{

class AvalancheArchive : public IImportExporter
{
public:
    AvalancheArchive() = default;
    virtual ~AvalancheArchive() = default;

    ImportExportType GetType() override final { return IE_TYPE_EXPORTER; }
    const char* GetName() override final { return "Avalanche Archive"; }
    std::vector<const char*> GetInputExtensions() override final { return { ".ee", ".bl", ".nl" }; }
    const char* GetOutputExtension() override final { return "/"; }

    void Export(const fs::path& filename, const std::any& input, const fs::path& to, ImportExportFinishedCallback callback) override final
    {
        const auto& buffer = std::any_cast<FileBuffer>(input);

        std::thread([&, filename, buffer, to, callback] {
            auto path = filename.stem();
            DEBUG_LOG("AvalancheArchive::Export - Exporting archive to '" << path << "'...");

            // create the directory if we need to
            if (!fs::exists(path)) {
                fs::create_directory(path);
            }

            // read the archive data from the buffer
            auto archive = FileLoader::Get()->ReadStreamArchive(buffer);
            if (archive) {
                // iterate over all the archive entries
                for (const auto& entry : archive->m_Files) {
                    if (entry.m_Offset != 0) {
                        const auto file = to / path / entry.m_Filename;

                        auto bytes = archive->GetEntryBuffer(entry);
                        WriteBufferToFile(file, &bytes);
                    }
                }
            }

            if (callback) {
                callback();
            }
        }).detach();
    }
};

};