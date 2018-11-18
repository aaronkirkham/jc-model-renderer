#include "Widget_ModelExplorer.h"
#include <Window.h>
#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>

Widget_ModelExplorer::Widget_ModelExplorer()
{
    m_Title = "Models";
    m_Flags |= ImGuiWindowFlags_NoFocusOnAppearing;
}

void Widget_ModelExplorer::Render(RenderContext_t* context)
{
    const auto& window_size  = Window::Get()->GetSize();
    const auto  aspect_ratio = (window_size.x / window_size.y);

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

                const bool tree_open = ImGui::TreeNode(label.c_str());

                // draw render block context menu
                if (ImGui::BeginPopupContextItem()) {
                    render_block->DrawContextMenu();
                    ImGui::EndPopup();
                }

                // draw render block info
                if (tree_open) {
                    // show import button if the block doesn't have a vertex buffer
                    if (!render_block->GetVertexBuffer()) {
#ifdef ENABLE_OBJ_IMPORT
                        static auto red = ImVec4{1, 0, 0, 1};
                        ImGui::TextColored(red, "This model doesn't have any mesh!");

                        if (ImGuiCustom::BeginButtonDropDown("Import Mesh From")) {
                            const auto& importers = ImportExportManager::Get()->GetImporters(".rbm");
                            for (const auto& importer : importers) {
                                if (ImGui::MenuItem(importer->GetName(), importer->GetExportExtension())) {
                                    UI::Events().ImportFileRequest(importer, [&](bool success, std::any data) {
                                        auto& [vertices, uvs, normals, indices] =
                                            std::any_cast<std::tuple<floats_t, floats_t, floats_t, uint16s_t>>(data);

                                        render_block->SetData(&vertices, &indices, &uvs);
                                        render_block->Create();

                                        Renderer::Get()->AddToRenderList(render_block);
                                    });
                                }
                            }

                            ImGuiCustom::EndButtonDropDown();
                        }
#endif
                    } else {
                        // draw model attributes ui
                        render_block->DrawUI();
                    }

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
