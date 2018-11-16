#pragma once

#include <StdInc.h>
#include <memory.h>
#include <singleton.h>

#include "widgets/IWidget.h"
#include <import_export/IImportExporter.h>

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

enum DragDropPayloadType {
    DROPPAYLOAD_UNKNOWN,
};

struct DragDropPayload {
    DragDropPayloadType type;
    const char*         data;
};

using ContextMenuCallback = std::function<void(const fs::path& filename)>;

struct ImDrawList;
struct BoundingBox;
class Texture;
class UI : public Singleton<UI>
{
  private:
    UIEvents m_UIEvents;

    std::recursive_mutex                       m_StatusTextsMutex;
    std::map<uint64_t, std::string>            m_StatusTexts;
    std::map<std::string, ContextMenuCallback> m_ContextMenuCallbacks;

    std::vector<std::unique_ptr<IWidget>> m_Widgets;
    bool                                  m_IsDragDrop = false;
    std::string                           m_DragDropPayload;

  public:
    UI();
    virtual ~UI() = default;

    ImDrawList* SceneDrawList = nullptr;

    virtual UIEvents& Events()
    {
        return m_UIEvents;
    }

    void Render(RenderContext_t* context);
    void RenderSpinner(const std::string& str);
    void RenderContextMenu(const fs::path& filename, uint32_t unique_id_extra = 0, uint32_t flags = 0);

    void RenderBlockTexture(const std::string& title, Texture* texture);

    uint64_t PushStatusText(const std::string& str);
    void     PopStatusText(uint64_t id);

    const char* GetFileTypeIcon(const fs::path& filename);

    DragDropPayload* GetDropPayload(DragDropPayloadType payload_type);

    void RegisterContextMenuCallback(const std::vector<std::string>& extensions, ContextMenuCallback fn);

    // TODO: move these back into some DebugRenderer class
    //  want to use a shader for it so we can draw stuff in the 3d world
#undef DrawText
    void DrawText(const std::string& text, const glm::vec3& position, const glm::vec4& colour, bool center);
    void DrawBoundingBox(const BoundingBox& bb, const glm::vec4& colour);
};
