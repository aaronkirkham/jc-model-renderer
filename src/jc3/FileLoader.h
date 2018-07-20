#pragma once

#include <StdInc.h>
#include <DirectoryList.h>

#include <jc3/renderblocks/IRenderBlock.h>
#include <jc3/Types.h>

#include <jc3/formats/StreamArchive.h>
#include <jc3/formats/AvalancheDataFormat.h>

#include <functional>

using ReadFileCallback = std::function<void(bool, FileBuffer)>;
using FileTypeCallback = std::function<void(const fs::path& filename, const FileBuffer& data)>;

enum ReadFileFlags : uint8_t {
    NO_TEX_CACHE = 1,
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

    // batch reading
    std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<ReadFileCallback>>> m_Batches;
    std::unordered_map<std::string, std::vector<ReadFileCallback>> m_PathBatches;
    std::recursive_mutex m_BatchesMutex;

    StreamArchive_t* ParseStreamArchive(std::istream& stream);
    bool DecompressArchiveFromStream(std::istream& stream, FileBuffer* output) noexcept;
    //void ParseCompressedTexture(std::istream& stream, uint64_t size, FileBuffer* output);
    bool ParseCompressedTexture(const FileBuffer* ddsc_buffer, const FileBuffer* hmddsc_buffer, FileBuffer* output) noexcept;

public:
    bool m_UseBatches = false;

    FileLoader();
    virtual ~FileLoader() = default;

    void ReadFile(const fs::path& filename, ReadFileCallback callback, uint8_t flags = 0) noexcept;
    void ReadFileBatched(const fs::path& filename, ReadFileCallback callback) noexcept;
    void RunFileBatches() noexcept;

    // archives
    bool ReadArchiveTable(const fs::path& filename, JustCause3::ArchiveTable::VfsArchive* output) noexcept;
    bool ReadFileFromArchive(const std::string& directory, const std::string& archive, uint32_t namehash, FileBuffer* output) noexcept;

    // stream archive
    StreamArchive_t* ReadStreamArchive(const FileBuffer& buffer) noexcept;
    StreamArchive_t* ReadStreamArchive(const fs::path& filename) noexcept;

    // textures
    bool ReadCompressedTexture(const FileBuffer* buffer, const FileBuffer* hmddsc_buffer, FileBuffer* output) noexcept;
    bool ReadCompressedTexture(const fs::path& filename, FileBuffer* output) noexcept;

    // runtime containers
    RuntimeContainer* ReadRuntimeContainer(const FileBuffer& buffer) noexcept;

    // shader bundles
    AvalancheDataFormat* ReadAdf(const fs::path& filename) noexcept;

    // stream archive caching
    std::tuple<StreamArchive_t*, StreamArchiveEntry_t> GetStreamArchiveFromFile(const fs::path& file) noexcept;
    std::tuple<std::string, std::string, uint32_t> LocateFileInDictionary(const fs::path& filename) noexcept;
    DirectoryList* GetDirectoryList() { return m_FileList.get(); }

    // file read callbacks
    void RegisterCallback(const std::string& filetype, FileTypeCallback fn);
    void RegisterCallback(const std::vector<std::string>& filetypes, FileTypeCallback fn);
};