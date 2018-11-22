#pragma once

#include "../../directory_list.h"
#include "../../factory.h"
#include "../../graphics/types.h"
#include "stream_archive.h"

class RenderBlockModel;

class AvalancheArchive : public Factory<AvalancheArchive>
{
  private:
    std::filesystem::path            m_File              = "";
    std::unique_ptr<StreamArchive_t> m_StreamArchive     = nullptr;
    std::unique_ptr<DirectoryList>   m_FileList          = nullptr;
    bool                             m_HasUnsavedChanged = false;

  public:
    AvalancheArchive(const std::filesystem::path& filename, const FileBuffer& buffer, bool external = false);
    virtual ~AvalancheArchive() = default;

    virtual std::string GetFactoryKey() const
    {
        return m_File.string();
    }

    void AddFile(const std::filesystem::path& filename, const FileBuffer& data);
    void AddDirectory(const std::filesystem::path& filename, const std::filesystem::path& root = "");
    bool HasFile(const std::filesystem::path& filename);

    static void ReadFileCallback(const std::filesystem::path& filename, const FileBuffer& data, bool external);
    static bool SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& directory);

    StreamArchive_t* GetStreamArchive()
    {
        return m_StreamArchive.get();
    }

    DirectoryList* GetDirectoryList()
    {
        return m_FileList.get();
    }

    bool HasUnsavedChanges() const
    {
        return m_HasUnsavedChanged;
    }

    const std::filesystem::path& GetFilePath()
    {
        return m_File;
    }
};
