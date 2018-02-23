#pragma once

#include <StdInc.h>
#include <DirectoryList.h>
#include <jc3/formats/StreamArchive.h>

class RenderBlockModel;
class AvalancheArchive
{
private:
    fs::path m_File = "";
    StreamArchive_t* m_StreamArchive = nullptr;
    std::unique_ptr<DirectoryList> m_FileList = nullptr;
    std::vector<RenderBlockModel*> m_LinkedRenderBlockModels;
    std::recursive_mutex m_LinkedRenderBlockModelsMutex;

    void Initialise();

public:
    AvalancheArchive(const fs::path& file);
    AvalancheArchive(const fs::path& filename, const FileBuffer& buffer);
    virtual ~AvalancheArchive();

    static void FileReadCallback(const fs::path& filename, const FileBuffer& data);

    void LinkRenderBlockModel(RenderBlockModel* rbm);

    StreamArchive_t* GetStreamArchive() { return m_StreamArchive; }
    DirectoryList* GetDirectoryList() { return m_FileList.get(); }

    const fs::path& GetFileName() { return m_File; }
};