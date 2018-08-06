#pragma once

#include <any>
#include <functional>
#include <thread>
#include <fstream>

enum ImportExportType {
    IE_TYPE_IMPORTER = 0,
    IE_TYPE_EXPORTER,
    IE_TYPE_BOTH
};

class RenderBlockModel;
class IImportExporter
{
public:
    using ImportExportFinishedCallback = std::function<void(bool)>;

public:
    IImportExporter() = default;
    virtual ~IImportExporter() = default;

    virtual ImportExportType GetType() = 0;
    virtual const char* GetName() = 0;
    virtual std::vector<const char*> GetInputExtensions() = 0;
    virtual const char* GetOutputExtension() = 0;

    // NOTE: callback can be nullptr! remember to check this correctly in overrides of this function!
    virtual void Export(const fs::path& filename, const fs::path& to, ImportExportFinishedCallback callback = {}) = 0;

    bool WriteBufferToFile(const fs::path& file, const FileBuffer* buffer) noexcept
    {
        // create the directories for the file
        fs::create_directories(file.parent_path());

        // open the stream
        std::ofstream stream(file, std::ios::binary);
        if (stream.fail()) {
            DEBUG_LOG("IImportExporter::WriteBufferToFile - Failed to open stream");
            return false;
        }

        DEBUG_LOG("IImportExporter::WriteBufferToFile - writing " << file);

        // write the buffer and finish
        stream.write((char *)&buffer->front(), buffer->size());
        stream.close();
        return true;
    }
};