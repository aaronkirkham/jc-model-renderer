#pragma once

class RenderBlockModel;
class IImportExporter
{
public:
    IImportExporter() = default;
    virtual ~IImportExporter() = default;

    virtual void Export(RenderBlockModel*) = 0;
};