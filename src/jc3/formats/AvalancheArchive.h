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
    std::unique_ptr<StreamArchive_t> m_StreamArchive = nullptr;
    std::unique_ptr<DirectoryList> m_FileList = nullptr;
    bool m_HasUnsavedChanged = false;

public:
    AvalancheArchive(const fs::path& file, bool load_from_archive = true);
    AvalancheArchive(const fs::path& filename, const FileBuffer& buffer);
    virtual ~AvalancheArchive() = default;

    virtual std::string GetFactoryKey() const { return m_File.string(); }

    void AddFile(const fs::path& filename, const FileBuffer& data);
    void AddDirectory(const fs::path& filename, const fs::path& root = "");
    bool HasFile(const fs::path& filename);

    static void ReadFileCallback(const fs::path& filename, const FileBuffer& data, bool external);
    static bool SaveFileCallback(const fs::path& filename, const fs::path& directory);

    StreamArchive_t* GetStreamArchive() { return m_StreamArchive.get(); }
    DirectoryList* GetDirectoryList() { return m_FileList.get(); }

    bool HasUnsavedChanges() const { return m_HasUnsavedChanged; }

    const fs::path& GetFilePath() { return m_File; }
};