#pragma once

#include <StdInc.h>

#include <DirectoryList.h>

#include <jc3/NameHashLookup.h>
#include <jc3/Types.h>
#include <jc3/formats/AvalancheDataFormat.h>
#include <jc3/formats/StreamArchive.h>
#include <jc3/renderblocks/IRenderBlock.h>

#include <functional>

using ReadFileCallback       = std::function<void(bool success, FileBuffer data)>;
using FileTypeCallback       = std::function<void(const fs::path& filename, FileBuffer data, bool external)>;
using FileSaveCallback       = std::function<bool(const fs::path& filename, const fs::path& directory)>;
using ReadArchiveCallback    = std::function<void(std::unique_ptr<StreamArchive_t>)>;
using DictionaryLookupResult = std::tuple<std::string, std::string, uint32_t>;

enum ReadFileFlags : uint8_t {
    SKIP_TEXTURE_LOADER = 1,
};

class RuntimeContainer;
class FileLoader : public Singleton<FileLoader>
{
  private:
    std::unique_ptr<DirectoryList> m_FileList = nullptr;

    // file list dictionary
    std::unordered_map<uint32_t, std::pair<fs::path, std::vector<std::string>>> m_Dictionary;

    // file types
    std::unordered_map<std::string, std::vector<FileTypeCallback>> m_FileTypeCallbacks;
    std::unordered_map<std::string, std::vector<FileSaveCallback>> m_SaveFileCallbacks;

    // batch reading
    std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<ReadFileCallback>>> m_Batches;
    std::unordered_map<std::string, std::vector<ReadFileCallback>>                               m_PathBatches;
    std::recursive_mutex                                                                         m_BatchesMutex;

    std::unique_ptr<StreamArchive_t> ParseStreamArchive(FileBuffer* sarc_buffer, FileBuffer* toc_buffer = nullptr);

  public:
    inline static bool UseBatches = false;

    FileLoader();
    virtual ~FileLoader() = default;

    void ReadFile(const fs::path& filename, ReadFileCallback callback, uint8_t flags = 0) noexcept;
    void ReadFileBatched(const fs::path& filename, ReadFileCallback callback) noexcept;
    void RunFileBatches() noexcept;

    void ReadFileFromDisk(const fs::path& filename) noexcept;

    // archives
    bool ReadArchiveTable(const fs::path& filename, JustCause3::ArchiveTable::VfsArchive* output) noexcept;
    bool ReadFileFromArchive(const std::string& directory, const std::string& archive, uint32_t namehash,
                             FileBuffer* output) noexcept;

    // stream archive
    void ReadStreamArchive(const fs::path& filename, const FileBuffer& buffer, bool external_source,
                           ReadArchiveCallback callback) noexcept;
    void CompressArchive(std::ostream& stream, JustCause3::AvalancheArchive::Header* header,
                         std::vector<JustCause3::AvalancheArchive::Chunk>* chunks) noexcept;
    void CompressArchive(std::ostream& stream, StreamArchive_t* archive) noexcept;
    bool DecompressArchiveFromStream(std::istream& stream, FileBuffer* output) noexcept;

    // toc
    void WriteTOC(const fs::path& filename, StreamArchive_t* archive) noexcept;

    // textures
    void ReadTexture(const fs::path& filename, ReadFileCallback callback) noexcept;
    bool ParseCompressedTexture(FileBuffer* data, FileBuffer* outData) noexcept;
    void ParseHMDDSCTexture(FileBuffer* data, FileBuffer* outData) noexcept;

    // runtime containers
    void                              WriteRuntimeContainer(RuntimeContainer* runtime_container) noexcept;
    std::shared_ptr<RuntimeContainer> ParseRuntimeContainer(const fs::path&   filename,
                                                            const FileBuffer& buffer) noexcept;

    // shader bundles
    std::unique_ptr<AvalancheDataFormat> ReadAdf(const fs::path& filename) noexcept;

    // stream archive caching
    std::tuple<StreamArchive_t*, StreamArchiveEntry_t>
                           GetStreamArchiveFromFile(const fs::path& file, StreamArchive_t* archive = nullptr) noexcept;
    DictionaryLookupResult LocateFileInDictionary(const fs::path& filename) noexcept;
    DictionaryLookupResult LocateFileInDictionaryByPartOfName(const fs::path& filename) noexcept;

    DirectoryList* GetDirectoryList()
    {
        return m_FileList.get();
    }

    // file callbacks
    void RegisterReadCallback(const std::vector<std::string>& extensions, FileTypeCallback fn);
    void RegisterSaveCallback(const std::vector<std::string>& extensions, FileSaveCallback fn);
};
