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
    FileBuffer DecompressArchiveFromStream(std::istream& stream);
    void ParseCompressedTexture(std::istream& stream, uint64_t size, FileBuffer* output);

public:
    FileLoader();
    virtual ~FileLoader() = default;

    void ReadFile(const fs::path& filename, ReadFileCallback callback);

    // archives
    void ReadArchiveTable(const std::string& filename, JustCause3::ArchiveTable::VfsArchive* output);
    void ReadFileFromArchive(const std::string& archive, uint32_t namehash, ReadFileCallback callback);

    // stream archive
    StreamArchive_t* ReadStreamArchive(const FileBuffer& buffer);
    StreamArchive_t* ReadStreamArchive(const fs::path& filename);

    // textures
    void ReadCompressedTexture(const FileBuffer& buffer, uint64_t size, FileBuffer* output);
    void ReadCompressedTexture(const fs::path& filename, FileBuffer* output);

    // runtime containers
    RuntimeContainer* ReadRuntimeContainer(const FileBuffer& buffer);

    // stream archive caching
    std::tuple<StreamArchive_t*, StreamArchiveEntry_t> GetStreamArchiveFromFile(const fs::path& file);

    std::tuple<std::string, uint32_t> LocateFileInDictionary(const fs::path& filename);

    DirectoryList* GetDirectoryList() { return m_FileList.get(); }

    // file read callbacks
    void RegisterCallback(const std::string& filetype, FileTypeCallback fn);
    void RegisterCallback(const std::vector<std::string>& filetypes, FileTypeCallback fn);
};