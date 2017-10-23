#pragma once

#include <StdInc.h>
#include <DirectoryList.h>

#include <jc3/renderblocks/IRenderBlock.h>
#include <jc3/Types.h>

#include <jc3/formats/StreamArchive.h>

#include <functional>

using ReadFileCallback = std::function<void(bool, std::vector<uint8_t>)>;

class FileLoader : public Singleton<FileLoader>
{
private:
    std::unique_ptr<DirectoryList> m_FileList = nullptr;
    json m_FileListDictionary;

    StreamArchive_t* ParseStreamArchive(std::istream& stream);
    std::vector<uint8_t> DecompressArchiveFromStream(std::istream& stream);
    void ParseCompressedTexture(std::istream& stream, uint64_t size, std::vector<uint8_t>* output);

public:
    FileLoader();
    virtual ~FileLoader() = default;

    void ReadFile(const fs::path& filename, ReadFileCallback callback);

    // archives
    void ReadArchiveTable(const std::string& filename, JustCause3::ArchiveTable::VfsArchive* output);
    void ReadFileFromArchive(const std::string& archive, uint32_t namehash, ReadFileCallback callback);

    // stream archive
    StreamArchive_t* ReadStreamArchive(const std::vector<uint8_t>& buffer);
    StreamArchive_t* ReadStreamArchive(const fs::path& filename);

    // textures
    void ReadCompressedTexture(const std::vector<uint8_t>& buffer, uint64_t size, std::vector<uint8_t>* output);
    void ReadCompressedTexture(const fs::path& filename, std::vector<uint8_t>* output);

    // runtime containers
    void ReadRuntimeContainer(const std::vector<uint8_t>& buffer);

    // stream archive caching
    std::tuple<StreamArchive_t*, StreamArchiveEntry_t> GetStreamArchiveFromFile(const fs::path& file);

    std::tuple<std::string, uint32_t> LocateFileInDictionary(const fs::path& filename);

    DirectoryList* GetDirectoryList() { return m_FileList.get(); }
};