#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>
#include <jc3/Types.h>

#include <jc3/formats/StreamArchive.h>

#include <functional>

class DirectoryList
{
private:
    json m_Structure;

    void split(std::string str, json& current)
    {
        auto current_pos = str.find('/');
        if (current_pos != std::string::npos) {
            auto directory = str.substr(0, current_pos);
            str.erase(0, current_pos + 1);

            if (current.is_object() || current.is_null()) {
                split(str, current[directory]);
            }
            else {
                auto temp = current;
                current = nullptr;
                current["/"] = temp;
                split(str, current[directory]);
            }
        }
        else {
            if (!current.is_object()) {
                current.emplace_back(str);
            }
            else {
                current["/"].emplace_back(str);
            }
        }
    }

public:
    DirectoryList() = default;
    virtual ~DirectoryList() = default;

    void Add(const std::string& filename) {
        split(filename, m_Structure);
    }

    json* GetStructure() { return &m_Structure; }
};

using DictionaryLookupCallback = std::function<void(bool, std::string, uint32_t, std::string)>;

class FileLoader : public Singleton<FileLoader>
{
private:
    std::unique_ptr<DirectoryList> m_FileList = nullptr;

    json m_FileListDictionary;
    bool m_LoadingDictionary = false;

    StreamArchive_t* ParseStreamArchive(std::istream& stream);
    std::vector<uint8_t> DecompressArchiveFromStream(std::istream& stream);

public:
    FileLoader();
    virtual ~FileLoader() = default;

    // archives
    void ReadArchiveTable(const std::string& filename, JustCause3::ArchiveTable::VfsArchive* output);
    void ReadFileFromArchive(const std::string& archive, const std::string& filename, uint32_t namehash);

    // stream archive
    StreamArchive_t* ReadStreamArchive(const std::vector<uint8_t>& buffer);
    StreamArchive_t* ReadStreamArchive(const fs::path& filename);

    // textures
    void ReadCompressedTexture(const fs::path& filename, std::vector<uint8_t>* output);


    // shit
    std::string GetFileName(uint32_t hash);
    std::string GetArchiveName(const std::string& filename);

    void LocateFileInDictionary(const std::string& filename, DictionaryLookupCallback callback);

    bool IsLoadingDictionary() const { return m_LoadingDictionary; }
    DirectoryList* GetDirectoryList() { return m_FileList.get(); }
};