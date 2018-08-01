#pragma once

#include <StdInc.h>
#include <DirectoryList.h>
#include <jc3/formats/StreamArchive.h>
#include <Factory.h>

class RenderBlockModel;
class AvalancheArchive : public Factory<AvalancheArchive>
{
private:
    fs::path m_File = "";
    StreamArchive_t* m_StreamArchive = nullptr;
    std::unique_ptr<DirectoryList> m_FileList = nullptr;

public:
    AvalancheArchive(const fs::path& file);
    AvalancheArchive(const fs::path& filename, const FileBuffer& buffer);
    virtual ~AvalancheArchive();

    virtual std::string GetFactoryKey() const { return m_File.string(); }

    void AddFile(const fs::path& filename, const FileBuffer& data);

    bool HasFile(const fs::path& filename);

    static void FileReadCallback(const fs::path& filename, const FileBuffer& data);

    StreamArchive_t* GetStreamArchive() { return m_StreamArchive; }
    DirectoryList* GetDirectoryList() { return m_FileList.get(); }

    const fs::path& GetFilePath() { return m_File; }
};