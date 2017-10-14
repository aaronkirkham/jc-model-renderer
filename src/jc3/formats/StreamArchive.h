#pragma once

#include <vector>

struct StreamArchiveEntry_t
{
    std::string m_Filename;
    uint32_t m_Offset;
    uint32_t m_Size;
};

struct StreamArchive_t
{
    fs::path m_Filename;
    std::vector<uint8_t> m_SARCBytes;
    std::vector<StreamArchiveEntry_t> m_Files;

    std::vector<uint8_t> ReadEntryFromArchive(const StreamArchiveEntry_t& entry)
    {
        auto start = m_SARCBytes.begin() + entry.m_Offset;
        auto end = m_SARCBytes.begin() + entry.m_Offset + entry.m_Size;

        return std::vector<uint8_t>(start, end);
    }
};