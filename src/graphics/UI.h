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

using ContextMenuCallback = std::function<void(const fs::path& filename)>;

class UI : public Singleton<UI>
{
private:
    UIEvents m_UIEvents;
    float m_MainMenuBarHeight = 0.0f;
    std::recursive_mutex m_StatusTextsMutex;
    std::map<uint64_t, std::string> m_StatusTexts;
    std::map<std::string, ContextMenuCallback> m_ContextMenuCallbacks;

    void RenderFileTreeView();

public:
    UI() = default;
    virtual ~UI() = default;

    virtual UIEvents& Events() { return m_UIEvents; }

    void Render();
    void RenderSpinner(const std::string& str);
    void RenderContextMenu(const fs::path& filename, uint32_t unique_id_extra = 0);

    uint64_t PushStatusText(const std::string& str);
    void PopStatusText(uint64_t id);

    void RegisterContextMenuCallback(const std::string& extension, ContextMenuCallback callback) { m_ContextMenuCallbacks[extension] = callback; }
};