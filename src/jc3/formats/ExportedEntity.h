#pragma once

#include <StdInc.h>
#include <jc3/Types.h>

class ExportedEntity
{
private:
    fs::path m_File = "";
    std::vector<JustCause3::AvalancheArchiveChunk> m_ArchiveChunks;

public:
    ExportedEntity(const fs::path& file);
    virtual ~ExportedEntity();
};