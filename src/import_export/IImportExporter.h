#pragma once

enum ImportExportType {
    IE_TYPE_IMPORTER = 0,
    IE_TYPE_EXPORTER,
    IE_TYPE_BOTH
};

class RenderBlockModel;
class IImportExporter
{
public:
    IImportExporter() = default;
    virtual ~IImportExporter() = default;

    virtual ImportExportType GetType() = 0;
    virtual const char* GetName() = 0;
    virtual const char* GetExtension() = 0;

    virtual void Export(RenderBlockModel*) = 0;
};