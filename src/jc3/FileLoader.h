#pragma once

#include <StdInc.h>
#include <DirectoryList.h>

#include <jc3/renderblocks/IRenderBlock.h>
#include <jc3/Types.h>

#include <jc3/formats/StreamArchive.h>

#include <functional>

using ReadFileCallback = std::function<void(bool, FileBuffer)>;
using FileTypeCallback = std::function<void(const fs::path& filename, const FileBuffer& data)>;

class RuntimeContainer;
class FileLoader : public Singleton<FileLoader>
{
private:
    std::unique_ptr<DirectoryList> m_FileList = nullptr;
    json m_FileListDictionary;

    std::unordered_map<std::string, std::vector<FileTypeCallback>> m_FileTypeCallbacks;

    StreamArchive_t* ParseStreamArchive(std::istream& stream);
    bool DecompressArchiveFromStream(std::istream& stream, FileBuffer* output) noexcept;
    void ParseCompressedTexture(std::istream& stream, uint64_t size, FileBuffer* output);

public:
    FileLoader();
    virtual ~FileLoader() = default;

    void ReadFile(const fs::path& filename, ReadFileCallback callback) noexcept;

    // archives
    bool ReadArchiveTable(const std::string& filename, JustCause3::ArchiveTable::VfsArchive* output) noexcept;
    bool ReadFileFromArchive(const std::string& directory, const std::string& archive, uint32_t namehash, FileBuffer* output) noexcept;

    // stream archive
    StreamArchive_t* ReadStreamArchive(const FileBuffer& buffer) noexcept;
    StreamArchive_t* ReadStreamArchive(const fs::path& filename) noexcept;

    // textures
    bool ReadCompressedTexture(const FileBuffer& buffer, uint64_t size, FileBuffer* output) noexcept;
    bool ReadCompressedTexture(const fs::path& filename, FileBuffer* output) noexcept;

    // runtime containers
    RuntimeContainer* ReadRuntimeContainer(const FileBuffer& buffer) noexcept;

    // stream archive caching
    std::tuple<StreamArchive_t*, StreamArchiveEntry_t> GetStreamArchiveFromFile(const fs::path& file) noexcept;

    std::tuple<std::string, std::string, uint32_t> LocateFileInDictionary(const fs::path& filename) noexcept;

    DirectoryList* GetDirectoryList() { return m_FileList.get(); }

    // file read callbacks
    void RegisterCallback(const std::string& filetype, FileTypeCallback fn);
    void RegisterCallback(const std::vector<std::string>& filetypes, FileTypeCallback fn);
};