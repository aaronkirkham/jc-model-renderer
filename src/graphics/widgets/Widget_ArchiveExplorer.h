#pragma once

#include "IWidget.h"

class Widget_ArchiveExplorer : public IWidget
{
  public:
    Widget_ArchiveExplorer();

    virtual void Render(RenderContext_t*) override;
};
