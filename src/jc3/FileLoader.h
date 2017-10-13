#pragma once

#include <StdInc.h>
#include <jc3/renderblocks/IRenderBlock.h>
#include <jc3/Types.h>

class DirectoryList
{
private:
    json structure;

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
        split(filename, structure);
    }

    const json& GetStructure() { return structure; }
};

class FileLoader : public Singleton<FileLoader>
{
private:
    std::unique_ptr<DirectoryList> m_FileList = nullptr;

    json m_FileListDictionary;
    bool m_LoadingDictionary = false;

    inline std::string TrimArchiveName(const std::string& filename);

    void ParseStreamArchive(std::istream& stream, std::vector<JustCause3::StreamArchive::FileEntry>* output);

public:
    FileLoader();
    virtual ~FileLoader() = default;

    void ReadArchiveTable(const std::string& filename, JustCause3::ArchiveTable::VfsArchive* output);
    void ParseStreamArchive(const std::vector<uint8_t>& data, std::vector<JustCause3::StreamArchive::FileEntry>* output);
    void ReadStreamArchive(const std::string& filename, std::vector<JustCause3::StreamArchive::FileEntry>* output);

    void ReadCompressedTexture(const fs::path& filename, std::vector<uint8_t>* output);

    std::string GetFileName(uint32_t hash);
    std::string GetArchiveName(const std::string& filename);

    bool IsLoadingDictionary() const { return m_LoadingDictionary; }
    DirectoryList* GetDirectoryList() { return m_FileList.get(); }
};