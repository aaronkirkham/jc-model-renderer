#pragma once

#include <StdInc.h>
#include <graphics/Types.h>
#include <singleton.h>

class DebugRenderer : public Singleton<DebugRenderer>
{
  public:
    DebugRenderer()          = default;
    virtual ~DebugRenderer() = default;
};
