#pragma once

#include <any>
#include <filesystem>
#include <fstream>
#include <functional>

#include "../graphics/types.h"

enum ImportExportType {
    ImportExportType_Importer = 0,
    ImportExportType_Exporter = 1,
    ImportExportType_Both     = 2,
};

using ImportFinishedCallback = std::function<void(bool, std::filesystem::path, std::any)>;
using ExportFinishedCallback = std::function<void(bool)>;

class RenderBlockModel;
class IImportExporter
{
  public:
    IImportExporter()          = default;
    virtual ~IImportExporter() = default;

    virtual ImportExportType         GetType()            = 0;
    virtual const char*              GetImportName()      = 0;
    virtual std::vector<std::string> GetImportExtension() = 0;
    virtual const char*              GetExportName()      = 0;
    virtual const char*              GetExportExtension() = 0;

    virtual bool HasSettingsUI()
    {
        return false;
    }

    virtual bool DrawSettingsUI()
    {
        return true;
    }

    virtual bool IsDragDropImportable()
    {
        return false;
    }

    // NOTE: callback can be nullptr! remember to check this correctly in overrides of this function!
    virtual void Import(const std::filesystem::path& filename, ImportFinishedCallback callback = {}) = 0;
    virtual void Export(const std::filesystem::path& filename, const std::filesystem::path& to,
                        ExportFinishedCallback callback = {})                                        = 0;

    bool WriteBufferToFile(const std::filesystem::path& file, const FileBuffer* buffer)
    {
        // create the directories for the file
        std::filesystem::create_directories(file.parent_path());

        // open the stream
        std::ofstream stream(file, std::ios::binary);
        if (stream.fail()) {
            return false;
        }

        // write the buffer and finish
        stream.write((char*)&buffer->front(), buffer->size());
        stream.close();
        return true;
    }
};
