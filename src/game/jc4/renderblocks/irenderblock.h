#pragma once

#include "../../irenderblock.h"

namespace jc4
{
class IRenderBlock : public ::IRenderBlock
{
  public:
    virtual void Create(FileBuffer* vertices, FileBuffer* indices) = 0;
};
} // namespace jc4
