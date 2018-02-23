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
        // TODO: if we already clicked export, there's a chance we don't need to re-do all this as the texture is already available in the
        // texture manager (see above) need to do some work to the ExportFileRequest handler so we can check if we already have a loaded file
        // before reading new stuff

        auto path = to / filename.stem(); path += ".dds";
        auto& buffer = std::any_cast<FileBuffer>(input);

        DEBUG_LOG("DDSC::Export - Exporting texture to '" << path << "'...");

        uint32_t magic = 0;
        std::memcpy(&magic, (char *)&buffer.front(), sizeof(magic));

        // because we might already have the texture loaded in the TextureManager, let's see if the buffer contains
        // a compressed texture
        if (magic != DDS_MAGIC) {
            FileBuffer uncompressed_data;
            if (FileLoader::Get()->ReadCompressedTexture(buffer, std::numeric_limits<uint64_t>::max(), &uncompressed_data)) {
                WriteBufferToFile(path, &uncompressed_data);
            }
        }
        else {
            WriteBufferToFile(path, &buffer);
        }

        if (callback) {
            callback();
        }
    }
};

};