#pragma once

#include <StdInc.h>
#include <memory.h>
#include <singleton.h>

#include <import_export/IImportExporter.h>

struct ImDrawList;
struct BoundingBox;
struct RenderContext_t;
class Texture;
class IRenderBlock;
class AvalancheArchive;

struct UIEvents {
    ksignals::Event<void(const fs::path& file, AvalancheArchive* archive)>            FileTreeItemSelected;
    ksignals::Event<void(const fs::path& file, const fs::path& directory)>            SaveFileRequest;
    ksignals::Event<void(IImportExporter* importer, ImportFinishedCallback callback)> ImportFileRequest;
    ksignals::Event<void(const fs::path& file, IImportExporter* exporter)>            ExportFileRequest;
};

enum ContextMenuFlags {
    CTX_FILE    = (1 << 0),
    CTX_ARCHIVE = (1 << 1),
    CTX_TEXTURE = (1 << 2),
};

enum TreeViewTab {
    TAB_FILE_EXPLORER,
    TAB_ARCHIVES,
    TAB_MODELS,
    TAB_TOTAL,
};

enum DragDropPayloadType {
    DROPPAYLOAD_UNKNOWN,
};

struct DragDropPayload {
    DragDropPayloadType type;
    const char*         data;
};

using ContextMenuCallback = std::function<void(const fs::path& filename)>;
static constexpr auto MIN_SIDEBAR_WIDTH = 400.0f;

class UI : public Singleton<UI>
{
  private:
    UIEvents                                   m_UIEvents;
    float                                      m_MainMenuBarHeight = 0.0f;
    std::recursive_mutex                       m_StatusTextsMutex;
    std::map<uint64_t, std::string>            m_StatusTexts;
    std::map<std::string, ContextMenuCallback> m_ContextMenuCallbacks;
    TreeViewTab                                m_TabToSwitch          = TAB_FILE_EXPLORER;
    uint8_t                                    m_CurrentActiveGBuffer = 0;

    bool        m_IsDragDrop = false;
    std::string m_DragDropPayload;

    ImDrawList* m_SceneDrawList = nullptr;
    float       m_SceneWidth    = 0.0f;
    float       m_SidebarWidth  = MIN_SIDEBAR_WIDTH;

    void RenderFileTreeView();

  public:
    UI();
    virtual ~UI() = default;

    virtual UIEvents& Events()
    {
        return m_UIEvents;
    }

    void Render(RenderContext_t* context);
    void RenderSpinner(const std::string& str);
    void RenderContextMenu(const fs::path& filename, uint32_t unique_id_extra = 0, uint32_t flags = 0);

    void RenderBlockTexture(IRenderBlock* render_block, const std::string& title, std::shared_ptr<Texture> texture);

    uint64_t PushStatusText(const std::string& str);
    void     PopStatusText(uint64_t id);

    DragDropPayload* GetDropPayload(DragDropPayloadType payload_type);

    void SwitchToTab(const TreeViewTab tab)
    {
        m_TabToSwitch = tab;
    }

    void RegisterContextMenuCallback(const std::vector<std::string>& extensions, ContextMenuCallback fn);

#undef DrawText
    void DrawText(const std::string& text, const glm::vec3& position, const glm::vec4& colour, bool center);
    void DrawBoundingBox(const BoundingBox& bb, const glm::vec4& colour);
};
