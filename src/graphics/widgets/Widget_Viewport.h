#pragma once

#include "IWidget.h"

class Widget_Viewport : public IWidget
{
  public:
    Widget_Viewport();

    virtual bool Begin() override;
    virtual void End() override;
    virtual void Render(RenderContext_t*) override;
};
