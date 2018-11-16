#pragma once

#include "IWidget.h"
#include <array>

class Widget_Viewport : public IWidget
{
  public:
    Widget_Viewport();

    virtual bool Begin() override;
    virtual void Render(RenderContext_t*) override;

  private:
    int32_t             m_RenderTarget = 0;
    std::array<bool, 3> m_MouseState = {false};

    void HandleInput(const float delta_time);
};
