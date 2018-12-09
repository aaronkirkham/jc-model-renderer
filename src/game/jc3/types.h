#pragma once

// jc3 types

#pragma pack(push, 1)
namespace jc3
{
namespace ArchiveTable
{
    struct TabFileHeader {
        uint32_t m_Magic     = 0x424154; // "TAB"
        uint16_t m_Version   = 2;
        uint16_t m_Endian    = 1;
        int32_t  m_Alignment = 0;
    };

    static_assert(sizeof(TabFileHeader) == 0xC, "JC3 TabFileHeader alignment is wrong!");

    struct VfsTabEntry {
        uint32_t m_NameHash;
        uint32_t m_Offset;
        uint32_t m_Size;
    };

    static_assert(sizeof(VfsTabEntry) == 0xC, "JC3 VfsTabEntry alignment is wrong!");

    struct VfsArchive {
        uint64_t                 m_Magic;
        int64_t                  m_Size;
        int64_t                  m_Offset;
        uint32_t                 m_Index;
        uint16_t                 m_Version;
        std::vector<VfsTabEntry> m_Entries;
    };
}; // namespace ArchiveTable
}; // namespace jc3
#pragma pack(pop)
