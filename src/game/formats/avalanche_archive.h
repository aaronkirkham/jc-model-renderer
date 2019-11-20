#pragma once

#include "factory.h"

#include <filesystem>

namespace ava::StreamArchive
{
struct ArchiveEntry;
};

class DirectoryList;
class AvalancheArchive : public Factory<AvalancheArchive>
{
    using ParseCallback_t = std::function<void(bool success)>;

  private:
    std::filesystem::path                         m_Filename = "";
    std::vector<uint8_t>                          m_Buffer;
    std::vector<ava::StreamArchive::ArchiveEntry> m_Entries;
    std::unique_ptr<DirectoryList>                m_FileList           = nullptr;
    bool                                          m_UsingTOC           = false;
    bool                                          m_FromExternalSource = false;

  public:
    AvalancheArchive(const std::filesystem::path& filename);
    AvalancheArchive(const std::filesystem::path& filename, const std::vector<uint8_t>& buffer, bool external = false);
    virtual ~AvalancheArchive() = default;

    virtual std::string GetFactoryKey() const
    {
        return m_Filename.string();
    }

    void Parse(const std::vector<uint8_t>& buffer, ParseCallback_t callback = nullptr);

    static void ReadFileCallback(const std::filesystem::path& filename, const std::vector<uint8_t>& data,
                                 bool external);
    static bool SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path);

    bool AddFile(const std::filesystem::path& filename, const std::filesystem::path& root);
    bool AddFile(const std::filesystem::path& filename, const std::vector<uint8_t>& buffer);
    bool HasFile(const std::filesystem::path& filename);

    const std::filesystem::path& GetFilePath()
    {
        return m_Filename;
    }

    const std::vector<uint8_t>& GetBuffer()
    {
        return m_Buffer;
    }

    const std::vector<ava::StreamArchive::ArchiveEntry>& GetEntries()
    {
        return m_Entries;
    }

    DirectoryList* GetDirectoryList()
    {
        return m_FileList.get();
    }

    bool IsUsingTOC() const
    {
        return m_UsingTOC;
    }
};
