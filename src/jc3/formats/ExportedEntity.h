#pragma once

#include <StdInc.h>

class ExportedEntity
{
public:
    ExportedEntity(const fs::path& file);
    virtual ~ExportedEntity();
};