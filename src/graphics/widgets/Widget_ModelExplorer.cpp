#include "Widget_ModelExplorer.h"
#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>

Widget_ModelExplorer::Widget_ModelExplorer()
{
    m_Title = "Models";
}

void Widget_ModelExplorer::Render(RenderContext_t* context)
{
    for (auto it = RenderBlockModel::Instances.begin(); it != RenderBlockModel::Instances.end();) {
        const auto& filename_with_path = (*it).second->GetPath();
        const auto& filename           = (*it).second->GetFileName();

        bool       is_still_open = true;
        const bool open          = ImGui::CollapsingHeader(filename.c_str(), &is_still_open);

        // context menu
        UI::Get()->RenderContextMenu(filename_with_path);

        if (open) {
            uint32_t index = 0;
            for (const auto& render_block : (*it).second->GetRenderBlocks()) {
                // unique id
                std::string label(render_block->GetTypeName());
                label.append("##" + filename + "-" + std::to_string(index));

                // TODO: transparent style if disabled

                //
                const bool tree_open = ImGui::TreeNode(label.c_str());

                // draw render block info
                if (tree_open) {
                    ImGui::Text("RenderBlock");

                    ImGui::TreePop();
                }

                ++index;
            }
        }

        // handle close button
        if (!is_still_open) {
            std::lock_guard<std::recursive_mutex> _lk{RenderBlockModel::InstancesMutex};
            it = RenderBlockModel::Instances.erase(it);
            continue;
        }

        ++it;
    }
}
