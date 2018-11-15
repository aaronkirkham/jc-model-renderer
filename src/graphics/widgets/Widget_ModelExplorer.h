#pragma once

#include "IWidget.h"

class Widget_ModelExplorer : public IWidget
{
  public:
    Widget_ModelExplorer();

    virtual void Render(RenderContext_t*) override;
};
