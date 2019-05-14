#pragma once

#include "../../directory_list.h"
#include "../../factory.h"
#include "../../graphics/types.h"
#include "../types.h"

struct ArchiveEntry_t {
    std::string m_Filename;
    uint32_t    m_Offset;
    uint32_t    m_Size;
    bool        m_Patched = false;
};

class AvalancheArchive : public Factory<AvalancheArchive>
{
    using ParseCallback_t      = std::function<void(bool)>;
    using CompressCallback_t   = std::function<void(bool, FileBuffer)>;
    using DecompressCallback_t = std::function<void(bool, FileBuffer)>;

  private:
    std::filesystem::path          m_Filename = "";
    std::unique_ptr<DirectoryList> m_FileList = nullptr;
    FileBuffer                     m_Buffer;
    std::vector<ArchiveEntry_t>    m_Entries;
    bool                           m_UsingTOC          = false;
    bool                           m_HasUnsavedChanged = false;
    bool                           m_FromExternalSource = false;

    void WriteFileToBuffer(const std::filesystem::path& filename, const FileBuffer& data);

    template <typename T> void CopyToBuffer(FileBuffer* buffer, uint64_t* offset, T& val)
    {
        std::memcpy(buffer->data() + *offset, (char*)&val, sizeof(T));
        *offset += sizeof(T);
    }

    void CopyAlignedStringToBuffer(FileBuffer* buffer, uint64_t* offset, const std::string& val, uint32_t alignment)
    {
        static uint8_t ALIGN_BYTE = 0x0;
        std::memcpy(buffer->data() + *offset, val.data(), val.length());
        std::memcpy(buffer->data() + *offset + val.length(), &ALIGN_BYTE, alignment);

        *offset += (val.length() + (sizeof(decltype(ALIGN_BYTE)) * alignment));
    }

    uint32_t CalcStringLength(const std::string& val, uint32_t* out_alignment = nullptr)
    {
        const auto len       = val.length();
        const auto alignment = jc::DISTANCE_TO_BOUNDARY(len, 4);

        if (out_alignment) {
            *out_alignment = alignment;
        }

        return len + alignment;
    }

  public:
    AvalancheArchive(const std::filesystem::path& filename);
    AvalancheArchive(const std::filesystem::path& filename, const FileBuffer& buffer, bool external = false);
    virtual ~AvalancheArchive() = default;

    virtual std::string GetFactoryKey() const
    {
        return m_Filename.string();
    }

    void Add(const std::filesystem::path& filename, const std::filesystem::path& root = "");
    bool HasFile(const std::filesystem::path& filename);

    static void ReadFileCallback(const std::filesystem::path& filename, const FileBuffer& data, bool external);
    static bool SaveFileCallback(const std::filesystem::path& filename, const std::filesystem::path& path);

    void Parse(const FileBuffer& buffer, ParseCallback_t callback = nullptr);
    void ParseSARC(const FileBuffer& buffer, ParseCallback_t callback = nullptr);

    void ParseTOC(const FileBuffer& buffer);
    bool WriteTOC(const std::filesystem::path& filename);

    void Compress(CompressCallback_t callback);
    void Decompress(const FileBuffer& buffer, DecompressCallback_t callback);

    DirectoryList* GetDirectoryList()
    {
        return m_FileList.get();
    }

    bool IsUsingTOC() const
    {
        return m_UsingTOC;
    }

    bool HasUnsavedChanges() const
    {
        return m_HasUnsavedChanged;
    }

    const std::filesystem::path& GetFilePath()
    {
        return m_Filename;
    }

    const std::vector<ArchiveEntry_t>& GetEntries()
    {
        return m_Entries;
    }

    FileBuffer GetEntryBuffer(const std::string& filename)
    {
        const auto it = std::find_if(m_Entries.begin(), m_Entries.end(),
                                     [&](const ArchiveEntry_t& item) { return item.m_Filename == filename; });
        if (it == m_Entries.end()) {
            return {};
        }

        const auto start = m_Buffer.begin() + (*it).m_Offset;
        const auto end   = start + (*it).m_Size;
        return {start, end};
    }

    FileBuffer GetEntryBuffer(const ArchiveEntry_t& entry)
    {
        assert(entry.m_Offset != 0 && entry.m_Offset != -1);
        assert((entry.m_Offset + entry.m_Size) <= m_Buffer.size());

        const auto start = m_Buffer.begin() + entry.m_Offset;
        const auto end   = start + entry.m_Size;
        return {start, end};
    }
};
