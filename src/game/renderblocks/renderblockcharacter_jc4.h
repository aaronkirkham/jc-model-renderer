#pragma once

#include "irenderblock.h"

namespace jc4
{
class RenderBlockCharacter : public IRenderBlock
{
  private:
  public:
    RenderBlockCharacter() = default;
    virtual ~RenderBlockCharacter()
    {
        //
    }

    virtual const char* GetTypeName() override final
    {
        return "RenderBlockCharacter";
    }

    virtual uint32_t GetTypeHash() const override final
    {
        return RenderBlockFactory::RB_CHARACTER;
    }

    virtual bool IsOpaque() override final
    {
        return true;
    }

    virtual void Create() override final {}

    virtual void Read(std::istream& stream) override final {}
    virtual void Write(std::ostream& stream) override final {}

    virtual void SetData(vertices_t* vertices, uint16s_t* indices, materials_t* materials) override final
    {
        //
    }

    virtual std::tuple<vertices_t, uint16s_t> GetData() override final
    {
        return {};
    }

    virtual void Setup(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;

        IRenderBlock::Setup(context);
    }

    virtual void Draw(RenderContext_t* context) override final
    {
        if (!m_Visible)
            return;
    }

    virtual void DrawContextMenu() override final
    {
        //
    }

    virtual void DrawUI() override final
    {
        //
    }
};
}; // namespace jc4
