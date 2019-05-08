#pragma once

#include <array>
#include <map>

#include "../import_export/IImportExporter.h"
#include "../singleton.h"

#include <ksignals.h>

struct ImDrawList;
struct BoundingBox;
struct RenderContext_t;
class Texture;
class IRenderBlock;
class AvalancheArchive;

struct UIEvents {
    ksignals::Event<void(const std::filesystem::path& file, AvalancheArchive* archive)> FileTreeItemSelected;
    ksignals::Event<void(const std::filesystem::path& file, const std::filesystem::path& directory, bool is_dropzone)>
                                                                                        SaveFileRequest;
    ksignals::Event<void(IImportExporter* importer, ImportFinishedCallback callback)>   ImportFileRequest;
    ksignals::Event<void(const std::filesystem::path& file, IImportExporter* exporter)> ExportFileRequest;
};

enum ContextMenuFlags {
    ContextMenuFlags_None    = 0,
    ContextMenuFlags_File    = 1 << 0,
    ContextMenuFlags_Archive = 1 << 1,
    ContextMenuFlags_Texture = 1 << 2,
};

enum TreeViewTab {
    TreeViewTab_FileExplorer = 0,
    TreeViewTab_Archives,
    TreeViewTab_Models,
    TreeViewTab_Total,
};

enum DragDropPayloadType {
    DragDropPayload_Unknown = 0,
    DragDropPayload_Texture,
    DragDropPayload_Multi,
};

struct DragDropPayload {
    DragDropPayloadType                type;
    std::vector<std::filesystem::path> data;
};

using ContextMenuCallback               = std::function<void(const std::filesystem::path& filename)>;
static constexpr auto MIN_SIDEBAR_WIDTH = 400.0f;

class UI : public Singleton<UI>
{
  private:
    UIEvents                                      m_UIEvents;
    float                                         m_MainMenuBarHeight = 0.0f;
    std::recursive_mutex                          m_StatusTextsMutex;
    std::map<uint64_t, std::string>               m_StatusTexts;
    std::map<std::string, ContextMenuCallback>    m_ContextMenuCallbacks;
    TreeViewTab                                   m_TabToSwitch          = TreeViewTab_FileExplorer;
    uint8_t                                       m_CurrentActiveGBuffer = 0;
    std::array<bool, 2>                           m_SceneMouseState      = {false};
    bool                                          m_IsDragDrop           = false;
    std::vector<std::filesystem::path>            m_CurrentDragDropPayload;
    std::string                                   m_CurrentDragDropPayloadTooltip = "";
    bool                                          m_ShowGameSelection             = true;
    std::array<std::unique_ptr<Texture>, 2>       m_GameSelectionIcons            = {nullptr};
    std::vector<std::unique_ptr<DragDropPayload>> m_DragDropPayloads;

    struct {
        bool                  ShowExportSettings = false;
        IImportExporter*      Exporter           = nullptr;
        std::filesystem::path Filename           = "";
    } m_ExportSettings;

    struct {
        bool                     ShowSettings = false;
        std::shared_ptr<Texture> Texture      = nullptr;
    } m_NewTextureSettings;

    ImDrawList* m_SceneDrawList = nullptr;
    float       m_SceneWidth    = 0.0f;
    float       m_SidebarWidth  = MIN_SIDEBAR_WIDTH;

    void RenderMenuBar();

    void RenderSceneView(const glm::vec2& window_size);
    void RenderFileTreeView(const glm::vec2& window_size);

    void RenderModelsTab_RBM();
    void RenderModelsTab_AMF();

  public:
    UI();
    virtual ~UI() = default;

    virtual UIEvents& Events()
    {
        return m_UIEvents;
    }

    void Render(RenderContext_t* context);
    void RenderSpinner(const std::string& str);
    void RenderContextMenu(const std::filesystem::path& filename, uint32_t unique_id_extra = 0, uint32_t flags = 0);

    void RenderBlockTexture(IRenderBlock* render_block, const std::string& title, std::shared_ptr<Texture> texture);

    uint64_t PushStatusText(const std::string& str);
    void     PopStatusText(uint64_t id);

    DragDropPayload* GetDropPayload();
    DragDropPayload* GetDropPayload(DragDropPayloadType type);

    void SwitchToTab(const TreeViewTab tab)
    {
        m_TabToSwitch = tab;
    }

    bool IsDragDropping() const
    {
        return m_IsDragDrop;
    }

    void RegisterContextMenuCallback(const std::vector<std::string>& extensions, ContextMenuCallback fn);

#undef DrawText
    void DrawText(const std::string& text, const glm::vec3& position, const glm::vec4& colour, bool center);
    void DrawBoundingBox(const BoundingBox& bb, const glm::vec4& colour);

    inline void ShowGameSelection()
    {
        m_ShowGameSelection = true;
    }
};
