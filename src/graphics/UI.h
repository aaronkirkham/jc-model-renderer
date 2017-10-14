#pragma once

#include <StdInc.h>
#include <singleton.h>

struct UIEvents
{
    ksignals::Event<void(const std::string& filename)> FileTreeItemSelected;
};

class UI : public Singleton<UI>
{
private:
    UIEvents m_UIEvents;
    float m_MainMenuBarHeight = 0.0f;

    void RenderFileTreeView();

public:
    UI() = default;
    virtual ~UI() = default;

    virtual UIEvents& Events() { return m_UIEvents; }

    void Render();
};