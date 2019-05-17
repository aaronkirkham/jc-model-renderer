#pragma once

#include "../../irenderblock.h"

namespace jc4
{
class IRenderBlock : public ::IRenderBlock
{
  public:
    // TODO: temp hack. do something else.
    const char* m_Name = "";

    virtual void Load(SAdfDeferredPtr* constants)                          = 0;
    virtual void Create(IBuffer_t* vertex_buffer, IBuffer_t* index_buffer) = 0;
};
} // namespace jc4
