#pragma once

#include <StdInc.h>
#include <singleton.h>
#include <graphics/Types.h>

class DebugRenderer : public Singleton<DebugRenderer>
{
public:
    DebugRenderer() = default;
    virtual ~DebugRenderer() = default;
};