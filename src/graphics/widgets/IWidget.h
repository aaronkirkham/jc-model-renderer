#pragma once

#include <imgui.h>
#include <string>

struct RenderContext_t;
class IWidget
{
  protected:
    std::string m_Title   = "";
    bool        m_Visible = true;

  public:
    IWidget()          = default;
    virtual ~IWidget() = default;

    virtual bool Begin()
    {
        if (!m_Visible) {
            return false;
        }

        ImGui::Begin(m_Title.c_str());
        return true;
    }

    virtual void End()
    {
        if (!m_Visible) {
            return;
        }

        ImGui::End();
    }

    virtual void Render(RenderContext_t*) = 0;
};
