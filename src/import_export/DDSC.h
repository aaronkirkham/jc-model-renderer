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

    void Export(const fs::path& filename, const std::any& input, const fs::path& to, ImportExportFinishedCallback callback) override final
    {
        auto path = to / filename.stem(); path += ".dds";
        auto& buffer = std::any_cast<FileBuffer>(input);

        DEBUG_LOG("DDSC::Export - Exporting texture to '" << path << "'...");

        uint32_t magic = 0;
        std::memcpy(&magic, (char *)&buffer.front(), sizeof(magic));

        // because we might already have the texture loaded in the TextureManager, let's see if the buffer contains
        // a compressed texture
        if (magic != DDS_MAGIC) {
            // are we exporting HMDDSC?
            // because the HMDDSC only contains pixel information, we now need to find the DDSC file which contains the texture data
            if (filename.extension() == ".hmddsc") {
                DEBUG_LOG("file is hmddsc. loading ddsc data...");

                auto tmp_filename = filename;
                auto ddsc_filename = tmp_filename.replace_extension(".ddsc");
                FileLoader::Get()->ReadFile(ddsc_filename, [this, path, buffer, callback](bool success, FileBuffer ddsc_buffer) {
                    FileBuffer data;
                    if (FileLoader::Get()->ReadCompressedTexture(&ddsc_buffer, &buffer, &data)) {
                        WriteBufferToFile(path, &data);
                    }

                    if (callback) {
                        callback();
                    }
                });
            }
            // just a standard DDSC file, export now
            else {
                FileBuffer data;
                if (FileLoader::Get()->ReadCompressedTexture(&buffer, nullptr, &data)) {
                    WriteBufferToFile(path, &data);
                }

                if (callback) {
                    callback();
                }
            }
        }
        else {
            WriteBufferToFile(path, &buffer);

            if (callback) {
                callback();
            }
        }
    }
};

};