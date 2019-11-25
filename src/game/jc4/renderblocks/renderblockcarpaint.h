#pragma once

#include "irenderblock.h"

namespace jc4
{
class RenderBlockCarPaint : public IRenderBlock
{
  private:
  public:
    RenderBlockCarPaint() = default;
    virtual ~RenderBlockCarPaint();

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockCarPaint";
    }

    virtual uint32_t GetTypeHash() const override final;

    virtual bool IsOpaque() override final
    {
        return true;
    }

    virtual void Load(ava::AvalancheDataFormat::SAdfDeferredPtr* constants) override final
    {
        //
    }

    virtual void Create(const std::string& type_id, IBuffer_t* vertex_buffer, IBuffer_t* index_buffer)
    {
        m_VertexBuffer = vertex_buffer;
        m_IndexBuffer  = index_buffer;

        switch (vertex_buffer->m_ElementStride) {
            case 0: {
                break;
            }

#ifdef DEBUG
            default: {
                __debugbreak();
                break;
            }
#endif
        }
    }

    virtual void DrawContextMenu() override final {}
    virtual void DrawUI() override final {}

    virtual void SetData(vertices_t* vertices, uint16s_t* indices, materials_t* materials) override final {}
    virtual std::tuple<vertices_t, uint16s_t> GetData() override final
    {
        return {};
    }

	rb_textures_t GetTextures() override final
    {
        rb_textures_t result;
        result.push_back({"diffuse", m_Textures[0]});
        return result;
    }
};
} // namespace jc4
