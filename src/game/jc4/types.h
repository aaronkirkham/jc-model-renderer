#pragma once

// jc4 types

#pragma pack(push, 1)
namespace jc4
{
namespace ArchiveTable
{
    struct TabFileHeader {
        uint32_t m_Magic     = 0x424154; // "TAB"
        uint16_t m_Version   = 2;
        uint16_t m_Endian    = 1;
        int32_t  m_Alignment = 0;
        uint32_t _unknown    = 0;
        uint32_t _unknown2   = 0;
        uint32_t _unknown3   = 0;
    };

    static_assert(sizeof(TabFileHeader) == 0x18, "JC4 TabFileHeader alignment is wrong!");

    enum CompressionType : uint8_t {
        CompressionType_None  = 0,
        CompressionType_Zlib  = 1,
        CompressionType_Oodle = 4,
    };

    struct VfsTabEntry {
        uint32_t        m_NameHash;
        uint32_t        m_Offset;
        uint32_t        m_CompressedSize;
        uint32_t        m_UncompressedSize;
        uint8_t         _unknown;
        uint8_t         _unknown2;
        CompressionType m_CompressionType;
        uint8_t         _unknown3;
    };

    static_assert(sizeof(VfsTabEntry) == 0x14, "JC4 VfsTabEntry alignment is wrong!");

    struct VfsArchive {
        uint64_t                 m_Magic;
        int64_t                  m_Size;
        int64_t                  m_Offset;
        uint32_t                 m_Index;
        uint16_t                 m_Version;
        std::vector<VfsTabEntry> m_Entries;
    };
}; // namespace ArchiveTable
}; // namespace jc4
#pragma pack(pop)
