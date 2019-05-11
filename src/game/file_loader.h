#pragma once

#include <functional>

#include "directory_list.h"

#include "formats/avalanche_data_format.h"
#include "formats/stream_archive.h"
#include "types.h"

using ReadFileResultCallback = std::function<void(bool success, FileBuffer data)>;
using FileReadHandler = std::function<void(const std::filesystem::path& filename, FileBuffer data, bool external)>;
using FileSaveHandler = std::function<bool(const std::filesystem::path& filename, const std::filesystem::path& path)>;
using ReadArchiveCallback    = std::function<void(std::unique_ptr<StreamArchive_t>)>;
using DictionaryLookupResult = std::tuple<std::string, std::string, uint32_t>;

enum ReadFileFlags : uint8_t {
    ReadFileFlags_None              = 0,
    ReadFileFlags_SkipTextureLoader = (1 << 0),
};

struct TabFileEntry {
    uint8_t  m_Version              = 0;
    uint32_t m_NameHash             = 0;
    uint32_t m_Offset               = 0;
    uint32_t m_CompressedSize       = 0;
    uint32_t m_UncompressedSize     = 0;
    uint8_t  m_CompressionType      = 0;
    uint16_t m_CompressedBlockIndex = 0;

    TabFileEntry() {}

    TabFileEntry(const jc4::ArchiveTable::VfsTabEntry& entry)
    {
        m_Version              = 2;
        m_NameHash             = entry.m_NameHash;
        m_Offset               = entry.m_Offset;
        m_CompressedSize       = entry.m_CompressedSize;
        m_UncompressedSize     = entry.m_UncompressedSize;
        m_CompressionType      = entry.m_CompressionType;
        m_CompressedBlockIndex = entry.m_CompressedBlockIndex;
    }

    TabFileEntry(const jc3::ArchiveTable::VfsTabEntry& entry)
    {
        m_Version          = 1;
        m_NameHash         = entry.m_NameHash;
        m_Offset           = entry.m_Offset;
        m_CompressedSize   = entry.m_Size;
        m_UncompressedSize = entry.m_Size;
    }
};

class RuntimeContainer;
class FileLoader : public Singleton<FileLoader>
{
  private:
    std::unique_ptr<DirectoryList> m_FileList      = nullptr;
    HMODULE                        oo2core_7_win64 = nullptr;

    // file list dictionary
    std::unordered_map<uint32_t, std::pair<std::filesystem::path, std::vector<std::string>>> m_Dictionary;

    // file types
    std::unordered_map<std::string, std::vector<FileReadHandler>> m_FileReadHandlers;
    std::unordered_map<std::string, std::vector<FileSaveHandler>> m_FileSaveHandlers;

    // batch reading
    std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<ReadFileResultCallback>>> m_Batches;
    std::unordered_map<std::string, std::vector<ReadFileResultCallback>>                               m_PathBatches;
    std::recursive_mutex                                                                               m_BatchesMutex;

    std::unique_ptr<StreamArchive_t> m_AdfTypeLibraries;

    std::unique_ptr<StreamArchive_t> ParseStreamArchive(const FileBuffer* sarc_buffer,
                                                        FileBuffer*       toc_buffer = nullptr);

  public:
    inline static bool UseBatches = false;

    FileLoader();
    virtual ~FileLoader();

    void Init();
    void Shutdown();

    void ReadFile(const std::filesystem::path& filename, ReadFileResultCallback callback, uint8_t flags = 0) noexcept;
    void ReadFileBatched(const std::filesystem::path& filename, ReadFileResultCallback callback) noexcept;
    void RunFileBatches() noexcept;

    void ReadFileFromDisk(const std::filesystem::path& filename) noexcept;

    // archives
    bool ReadArchiveTable(const std::filesystem::path& filename, std::vector<TabFileEntry>* output,
                          std::vector<jc4::ArchiveTable::VfsTabCompressedBlock>* output_blocks) noexcept;
    bool ReadArchiveTableEntry(const std::filesystem::path& table, const std::filesystem::path& filename,
                               TabFileEntry*                                          output,
                               std::vector<jc4::ArchiveTable::VfsTabCompressedBlock>* output_blocks) noexcept;
    bool ReadArchiveTableEntry(const std::filesystem::path& table, uint32_t name_hash, TabFileEntry* output,
                               std::vector<jc4::ArchiveTable::VfsTabCompressedBlock>* output_blocks) noexcept;
    bool ReadFileFromArchive(const std::string& directory, const std::string& archive, uint32_t namehash,
                             FileBuffer* output) noexcept;

    // stream archive
    void ReadStreamArchive(const std::filesystem::path& filename, const FileBuffer& buffer, bool external_source,
                           ReadArchiveCallback callback) noexcept;
    void CompressArchive(std::ostream& stream, jc::AvalancheArchive::Header* header,
                         std::vector<jc::AvalancheArchive::Chunk>* chunks) noexcept;
    void CompressArchive(std::ostream& stream, StreamArchive_t* archive) noexcept;
    bool DecompressArchiveFromStream(std::istream& stream, FileBuffer* output) noexcept;
    std::tuple<StreamArchive_t*, StreamArchiveEntry_t>
    GetStreamArchiveFromFile(const std::filesystem::path& file, StreamArchive_t* archive = nullptr) noexcept;

    // toc
    bool WriteTOC(const std::filesystem::path& filename, StreamArchive_t* archive) noexcept;

    // textures
    void ReadTexture(const std::filesystem::path& filename, ReadFileResultCallback callback) noexcept;
    bool ReadAVTX(FileBuffer* data, FileBuffer* outData) noexcept;
    void ReadHMDDSC(FileBuffer* data, FileBuffer* outData) noexcept;
    bool WriteAVTX(Texture* texture, FileBuffer* outData) noexcept;

    // runtime containers
    std::shared_ptr<RuntimeContainer> ParseRuntimeContainer(const std::filesystem::path& filename,
                                                            const FileBuffer&            buffer) noexcept;

    // adf
    std::shared_ptr<AvalancheDataFormat> ReadAdf(const std::filesystem::path& filename) noexcept;

    // dictionary lookup
    inline bool            HasFileInDictionary(uint32_t name_hash) noexcept;
    DictionaryLookupResult LocateFileInDictionary(const std::filesystem::path& filename) noexcept;
    DictionaryLookupResult LocateFileInDictionary(uint32_t name_hash) noexcept;
    DictionaryLookupResult LocateFileInDictionaryByPartOfName(const std::filesystem::path& filename) noexcept;

    DirectoryList* GetDirectoryList()
    {
        return m_FileList.get();
    }

    StreamArchive_t* GetAdfTypeLibraries()
    {
        return m_AdfTypeLibraries.get();
    }

    // file callbacks
    void RegisterReadCallback(const std::vector<std::string>& extensions, FileReadHandler fn);
    void RegisterSaveCallback(const std::vector<std::string>& extensions, FileSaveHandler fn);
};
