#pragma once

#include <StdInc.h>
#include <jc3/formats/StreamArchive.h>

class ExportedEntity
{
private:
    fs::path m_File = "";
    StreamArchive_t* m_StreamArchive = nullptr;

public:
    ExportedEntity(const fs::path& file);
    ExportedEntity(const fs::path& filename, const std::vector<uint8_t>& buffer);
    virtual ~ExportedEntity();

    StreamArchive_t* GetStreamArchive() { return m_StreamArchive; }
};