#pragma once

#include <functional>

#include "directory_list.h"

#include "formats/avalanche_data_format.h"
#include "formats/stream_archive.h"
#include "types.h"

using ReadFileCallback = std::function<void(bool success, FileBuffer data)>;
using FileTypeCallback = std::function<void(const std::filesystem::path& filename, FileBuffer data, bool external)>;
using FileSaveCallback =
    std::function<bool(const std::filesystem::path& filename, const std::filesystem::path& directory)>;
using ReadArchiveCallback    = std::function<void(std::unique_ptr<StreamArchive_t>)>;
using DictionaryLookupResult = std::tuple<std::string, std::string, uint32_t>;

enum ReadFileFlags : uint8_t {
    ReadFileFlags_None              = 0,
    ReadFileFlags_SkipTextureLoader = (1 << 0),
};

struct TabFileEntry {
    uint8_t  m_Version;
    uint32_t m_NameHash;
    uint32_t m_Offset;
    uint32_t m_CompressedSize;
    uint32_t m_UncompressedSize;
    uint8_t  m_CompressionType;

    TabFileEntry() {}

    TabFileEntry(const jc4::ArchiveTable::VfsTabEntry& entry)
    {
        m_Version          = 2;
        m_NameHash         = entry.m_NameHash;
        m_Offset           = entry.m_Offset;
        m_CompressedSize   = entry.m_CompressedSize;
        m_UncompressedSize = entry.m_UncompressedSize;
        m_CompressionType  = entry.m_CompressionType;
    }

    TabFileEntry(const jc3::ArchiveTable::VfsTabEntry& entry)
    {
        m_Version          = 1;
        m_NameHash         = entry.m_NameHash;
        m_Offset           = entry.m_Offset;
        m_CompressedSize   = entry.m_Size;
        m_UncompressedSize = entry.m_Size;
        m_CompressionType  = 0;
    }
};

class RuntimeContainer;
class FileLoader : public Singleton<FileLoader>
{
  private:
    std::unique_ptr<DirectoryList> m_FileList = nullptr;

    // file list dictionary
    std::unordered_map<uint32_t, std::pair<std::filesystem::path, std::vector<std::string>>> m_Dictionary;

    // file types
    std::unordered_map<std::string, std::vector<FileTypeCallback>> m_FileTypeCallbacks;
    std::unordered_map<std::string, std::vector<FileSaveCallback>> m_SaveFileCallbacks;

    // batch reading
    std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<ReadFileCallback>>> m_Batches;
    std::unordered_map<std::string, std::vector<ReadFileCallback>>                               m_PathBatches;
    std::recursive_mutex                                                                         m_BatchesMutex;

    std::unique_ptr<StreamArchive_t> ParseStreamArchive(const FileBuffer* sarc_buffer,
                                                        FileBuffer*       toc_buffer = nullptr);

  public:
    inline static bool UseBatches = false;

    FileLoader();
    virtual ~FileLoader() = default;

    void ReadFile(const std::filesystem::path& filename, ReadFileCallback callback, uint8_t flags = 0) noexcept;
    void ReadFileBatched(const std::filesystem::path& filename, ReadFileCallback callback) noexcept;
    void RunFileBatches() noexcept;

    void ReadFileFromDisk(const std::filesystem::path& filename) noexcept;

    // archives
    bool ReadArchiveTable(const std::filesystem::path& filename, std::vector<TabFileEntry>* output) noexcept;
    bool ReadArchiveTableEntry(const std::filesystem::path& table, const std::filesystem::path& filename,
                               TabFileEntry* output) noexcept;
    bool ReadArchiveTableEntry(const std::filesystem::path& table, uint32_t name_hash, TabFileEntry* output) noexcept;
    bool ReadFileFromArchive(const std::string& directory, const std::string& archive, uint32_t namehash,
                             FileBuffer* output) noexcept;

    // stream archive
    void ReadStreamArchive(const std::filesystem::path& filename, const FileBuffer& buffer, bool external_source,
                           ReadArchiveCallback callback) noexcept;
    void CompressArchive(std::ostream& stream, jc::AvalancheArchive::Header* header,
                         std::vector<jc::AvalancheArchive::Chunk>* chunks) noexcept;
    void CompressArchive(std::ostream& stream, StreamArchive_t* archive) noexcept;
    bool DecompressArchiveFromStream(std::istream& stream, FileBuffer* output) noexcept;

    // toc
    void WriteTOC(const std::filesystem::path& filename, StreamArchive_t* archive) noexcept;

    // textures
    void ReadTexture(const std::filesystem::path& filename, ReadFileCallback callback) noexcept;
    bool ParseCompressedTexture(FileBuffer* data, FileBuffer* outData) noexcept;
    void ParseHMDDSCTexture(FileBuffer* data, FileBuffer* outData) noexcept;

    // runtime containers
    void                              WriteRuntimeContainer(RuntimeContainer* runtime_container) noexcept;
    std::shared_ptr<RuntimeContainer> ParseRuntimeContainer(const std::filesystem::path& filename,
                                                            const FileBuffer&            buffer) noexcept;

    // shader bundles
    std::unique_ptr<AvalancheDataFormat> ReadAdf(const std::filesystem::path& filename) noexcept;

    // stream archive caching
    std::tuple<StreamArchive_t*, StreamArchiveEntry_t>
                           GetStreamArchiveFromFile(const std::filesystem::path& file, StreamArchive_t* archive = nullptr) noexcept;
    DictionaryLookupResult LocateFileInDictionary(const std::filesystem::path& filename) noexcept;
    DictionaryLookupResult LocateFileInDictionary(uint32_t name_hash) noexcept;
    DictionaryLookupResult LocateFileInDictionaryByPartOfName(const std::filesystem::path& filename) noexcept;

    DirectoryList* GetDirectoryList()
    {
        return m_FileList.get();
    }

    // file callbacks
    void RegisterReadCallback(const std::vector<std::string>& extensions, FileTypeCallback fn);
    void RegisterSaveCallback(const std::vector<std::string>& extensions, FileSaveCallback fn);
};
