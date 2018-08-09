#pragma once

#include <StdInc.h>
#include <singleton.h>
#include <memory.h>

class IImportExporter;
class AvalancheArchive;
struct UIEvents
{
    ksignals::Event<void(const fs::path& file, AvalancheArchive* archive)> FileTreeItemSelected;
    ksignals::Event<void(const fs::path& file, const fs::path& directory)> SaveFileRequest;
    ksignals::Event<void(const fs::path& file, IImportExporter* exporter)> ExportFileRequest;
};

enum ContextMenuFlags {
    CTX_FILE            = (1 << 0),
    CTX_ARCHIVE         = (1 << 1),
    CTX_TEXTURE         = (1 << 2),
};

using ContextMenuCallback = std::function<void(const fs::path& filename)>;

class UI : public Singleton<UI>
{
private:
    UIEvents m_UIEvents;
    float m_MainMenuBarHeight = 0.0f;
    std::recursive_mutex m_StatusTextsMutex;
    std::map<uint64_t, std::string> m_StatusTexts;
    std::map<std::string, ContextMenuCallback> m_ContextMenuCallbacks;
    std::string m_TabToSwitch = "";

    void RenderFileTreeView();

public:
    UI() = default;
    virtual ~UI() = default;

    virtual UIEvents& Events() { return m_UIEvents; }

    void Render();
    void RenderSpinner(const std::string& str);
    void RenderContextMenu(const fs::path& filename, uint32_t unique_id_extra = 0, uint32_t flags = 0);

    uint64_t PushStatusText(const std::string& str);
    void PopStatusText(uint64_t id);

    void SwitchToTab(const std::string& tab) { m_TabToSwitch = tab; }

    void RegisterContextMenuCallback(const std::vector<std::string>& extensions, ContextMenuCallback fn);
};