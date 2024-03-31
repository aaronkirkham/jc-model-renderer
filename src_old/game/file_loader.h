#pragma once

#include "singleton.h"

#include <atomic>
#include <functional>
#include <mutex>

using ReadFileResultCallback = std::function<void(bool success, std::vector<uint8_t> data)>;
using FileReadHandler =
    std::function<void(const std::filesystem::path& filename, std::vector<uint8_t> data, bool external)>;
using FileSaveHandler = std::function<bool(const std::filesystem::path& filename, const std::filesystem::path& path)>;
using DictionaryLookupResult = std::tuple<std::string, std::string, uint32_t>;

enum ReadFileFlags : uint8_t {
    ReadFileFlags_None              = 0,
    ReadFileFlags_SkipTextureLoader = (1 << 0),
};

namespace ava
{
namespace StreamArchive
{
    struct ArchiveEntry;
}
namespace ArchiveTable
{
    struct TabEntry;
    struct TabCompressedBlock;
} // namespace ArchiveTable
}; // namespace ava

class AvalancheArchive;
class DirectoryList;
class RuntimeContainer;
class Texture;
class FileLoader : public Singleton<FileLoader>
{
  private:
    std::unique_ptr<DirectoryList> m_FileList = nullptr;

    // file list dictionary
    std::unordered_map<uint32_t, std::pair<std::filesystem::path, std::vector<std::string>>> m_Dictionary;

    // file types
    std::unordered_map<std::string, std::vector<FileReadHandler>> m_FileReadHandlers;
    std::unordered_map<std::string, std::vector<FileSaveHandler>> m_FileSaveHandlers;

    // batch reading
    std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<ReadFileResultCallback>>> m_Batches;
    std::unordered_map<std::string, std::vector<ReadFileResultCallback>>                               m_PathBatches;
    std::recursive_mutex                                                                               m_BatchesMutex;

    std::unique_ptr<AvalancheArchive> m_AdfTypeLibraries;

  public:
    inline static std::atomic<bool> m_DictionaryLoading = false;
    inline static bool              UseBatches          = false;

    FileLoader();
    virtual ~FileLoader();

    void Init();
    void Shutdown();

    void ReadFile(const std::filesystem::path& filename, ReadFileResultCallback callback, uint8_t flags = 0);
    void ReadFileBatched(const std::filesystem::path& filename, ReadFileResultCallback callback);
    void RunFileBatches();

    void ReadFileFromDisk(const std::filesystem::path& filename, ReadFileResultCallback callback);
    void ReadFileFromDiskAndRunHandlers(const std::filesystem::path& filename);

    // archives
    bool ReadArchiveTable(const std::filesystem::path& filename, std::vector<ava::ArchiveTable::TabEntry>* output,
                          std::vector<ava::ArchiveTable::TabCompressedBlock>* output_blocks);
    bool ReadArchiveTableEntry(const std::filesystem::path& table, const std::filesystem::path& filename,
                               ava::ArchiveTable::TabEntry*                        output,
                               std::vector<ava::ArchiveTable::TabCompressedBlock>* output_blocks);
    bool ReadArchiveTableEntry(const std::filesystem::path& table, uint32_t name_hash,
                               ava::ArchiveTable::TabEntry*                        output,
                               std::vector<ava::ArchiveTable::TabCompressedBlock>* output_blocks);
    bool ReadFileFromArchive(const std::string& directory, const std::string& archive, uint32_t namehash,
                             std::vector<uint8_t>* out_buffer);

    // stream archive
    std::tuple<AvalancheArchive*, ava::StreamArchive::ArchiveEntry>
    GetStreamArchiveFromFile(const std::filesystem::path& file, AvalancheArchive* archive = nullptr);

    // textures
    void ReadTexture(const std::filesystem::path& filename, ReadFileResultCallback callback);
    bool ReadAVTX(const std::vector<uint8_t>& buffer, std::vector<uint8_t>* out_buffer);
    void ParseTextureSource(std::vector<uint8_t>* buffer, std::vector<uint8_t>* out_buffer);
    bool WriteAVTX(Texture* texture, std::vector<uint8_t>* out_buffer);

    // dictionary lookup
    inline bool            HasFileInDictionary(uint32_t name_hash);
    DictionaryLookupResult LocateFileInDictionary(const std::filesystem::path& filename);
    DictionaryLookupResult LocateFileInDictionary(uint32_t name_hash);
    DictionaryLookupResult LocateFileInDictionaryByPartOfName(const std::filesystem::path& filename);

    DirectoryList* GetDirectoryList()
    {
        return m_FileList.get();
    }

    AvalancheArchive* GetAdfTypeLibraries()
    {
        return m_AdfTypeLibraries.get();
    }

    // file callbacks
    void RegisterReadCallback(const std::vector<std::string>& extensions, FileReadHandler fn);
    void RegisterSaveCallback(const std::vector<std::string>& extensions, FileSaveHandler fn);
};
