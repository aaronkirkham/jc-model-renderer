#pragma once

#include <StdInc.h>
#include <DirectoryList.h>
#include <jc3/formats/StreamArchive.h>

class RenderBlockModel;
class ExportedEntity
{
private:
    fs::path m_File = "";
    StreamArchive_t* m_StreamArchive = nullptr;
    std::unique_ptr<DirectoryList> m_FileList = nullptr;
    std::vector<RenderBlockModel*> m_LinkedRenderBlockModels;
    std::recursive_mutex m_LinkedRenderBlockModelsMutex;

    void Initialise();

public:
    ExportedEntity(const fs::path& file);
    ExportedEntity(const fs::path& filename, const std::vector<uint8_t>& buffer);
    virtual ~ExportedEntity();

    void LinkRenderBlockModel(RenderBlockModel* rbm);

    StreamArchive_t* GetStreamArchive() { return m_StreamArchive; }
    DirectoryList* GetDirectoryList() { return m_FileList.get(); }
};