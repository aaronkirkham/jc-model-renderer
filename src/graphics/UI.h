#pragma once

#include <StdInc.h>
#include <singleton.h>

struct UIEvents
{
    ksignals::Event<void(const fs::path& filename)> FileTreeItemSelected;
    ksignals::Event<void(const fs::path& filename)> SaveFileRequest;
    ksignals::Event<void(const fs::path& filename)> ExportArchiveRequest;
};

class UI : public Singleton<UI>
{
private:
    UIEvents m_UIEvents;
    float m_MainMenuBarHeight = 0.0f;
    std::unordered_map<uint64_t, std::string> m_StatusTexts;

    void RenderFileTreeView();

public:
    UI() = default;
    virtual ~UI() = default;

    virtual UIEvents& Events() { return m_UIEvents; }

    void Render();
    void RenderSpinner(const std::string& str);

    uint64_t PushStatusText(const std::string& str);
    void PopStatusText(uint64_t id);
};