#include "Widget_ArchiveExplorer.h"
#include "Widget_FileExplorer.h"
#include <jc3/FileLoader.h>
#include <jc3/formats/AvalancheArchive.h>

Widget_ArchiveExplorer::Widget_ArchiveExplorer()
{
    m_Title = "Archives";
}

void Widget_ArchiveExplorer::Render(RenderContext_t* context)
{
    for (auto it = AvalancheArchive::Instances.begin(); it != AvalancheArchive::Instances.end();) {
        const auto& archive            = (*it).second;
        const auto& filename_with_path = archive->GetFilePath();
        const auto& filename           = filename_with_path.filename();

        bool       is_still_open = true;
        const bool open          = ImGui::CollapsingHeader(filename.string().c_str(), &is_still_open);

        // context menu
        UI::Get()->RenderContextMenu(filename_with_path, 0, ContextMenuFlags::CTX_ARCHIVE);

        // draw the archive directory list
        const auto directory_list = archive->GetDirectoryList();
        if (open && directory_list) {
            Widget_FileExplorer::RenderTreeView(directory_list->GetTree(), archive.get());
        }

        // handle close button
        if (!is_still_open) {
            std::lock_guard<std::recursive_mutex> _lk{AvalancheArchive::InstancesMutex};
            it = AvalancheArchive::Instances.erase(it);

            // flush texture and shader managers
            TextureManager::Get()->Flush();
            ShaderManager::Get()->Empty();
            continue;
        }

        ++it;
    }

    // drag drop target
    if (const auto payload = UI::Get()->GetDropPayload(DROPPAYLOAD_UNKNOWN)) {
        DEBUG_LOG("Widget_ArchiveExplorer DropPayload: " << payload->data);
    }
}