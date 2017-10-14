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
    std::vector<uint8_t> m_SARCBytes;
    std::vector<StreamArchiveEntry_t> m_Files;
};