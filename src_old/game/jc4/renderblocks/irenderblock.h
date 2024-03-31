#pragma once

#include "../../irenderblock.h"

#include <any>

namespace ava::AvalancheDataFormat
{
struct SAdfDeferredPtr;
};

namespace jc4
{
class IRenderBlock : public ::IRenderBlock
{
  public:
    // TODO: temp hack. do something else.
    const char* m_Name = "";

    ava::AvalancheDataFormat::SAdfDeferredPtr* m_Attributes = nullptr;

    // uint32_t m_AttributesType = 0;
    // std::any m_Attributes;

    virtual void Load(ava::AvalancheDataFormat::SAdfDeferredPtr* constants)                            = 0;
    virtual void Create(const std::string& type_id, IBuffer_t* vertex_buffer, IBuffer_t* index_buffer) = 0;
};
} // namespace jc4
