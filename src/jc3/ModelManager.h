#pragma once

#include <singleton.h>
#include <mutex>

#include <Window.h>
#include <graphics/Renderer.h>

class ModelManager : public Singleton<ModelManager>
{
private:
    std::recursive_mutex m_ModelsMutex;
    std::vector<RenderBlockModel*> m_Models;
    std::vector<RenderBlockModel*> m_ArchiveModels;

public:
    ModelManager()
    {
        Renderer::Get()->Events().RenderFrame.connect([&](RenderContext_t* context) {
            std::lock_guard<std::recursive_mutex> _lk{ m_ModelsMutex };

            ImGui::Begin("Model Manager", nullptr, ImVec2(), 0.0f, (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings));
            {
                const auto& window_size = Window::Get()->GetSize();

                uint32_t vertices = 0;
                uint32_t indices = 0;
                uint32_t triangles = 0;

                for (const auto& model : m_Models) {
                    for (const auto& block : model->GetRenderBlocks()) {
                        auto index_count = block->GetIndexBuffer()->m_ElementCount;

                        vertices += block->GetVertexBuffer()->m_ElementCount;
                        indices += index_count;
                        triangles += (index_count / 3);
                    }
                }

                ImGui::SetCursorPos({ 20, (window_size.y - 35) });
                ImGui::Text("Models: %d (%d), Vertices: %d, Indices: %d, Triangles: %d, Textures: %d", m_Models.size(), m_ArchiveModels.size(), vertices, indices, triangles, TextureManager::Get()->GetCacheSize());
            }
            ImGui::End();

            for (const auto& model : m_Models) {
                model->Draw(context);
            }
        });
    }

    virtual ~ModelManager()
    {
        for (auto& model : m_Models) {
            safe_delete(model);
        }

        m_Models.empty();
        m_ArchiveModels.empty();
    }

    void AddModel(RenderBlockModel* model, AvalancheArchive* archive = nullptr)
    {
        std::lock_guard<std::recursive_mutex> _lk{ m_ModelsMutex };

        if (archive) {
            m_ArchiveModels.emplace_back(model);
        }

        m_Models.emplace_back(model);
    }

    void RemoveModel(RenderBlockModel* model, AvalancheArchive* archive = nullptr)
    {
        std::lock_guard<std::recursive_mutex> _lk{ m_ModelsMutex };

        if (archive) {
            m_ArchiveModels.erase(std::remove(m_ArchiveModels.begin(), m_ArchiveModels.end(), model), m_ArchiveModels.end());
        }

        m_Models.erase(std::remove(m_Models.begin(), m_Models.end(), model), m_Models.end());
    }

    std::vector<RenderBlockModel*> GetModels()
    {
        std::lock_guard<std::recursive_mutex> _lk{ m_ModelsMutex };
        auto models = m_Models;

        return models;
    }

    std::vector<RenderBlockModel*> GetArchiveModels()
    {
        std::lock_guard<std::recursive_mutex> _lk{ m_ModelsMutex };
        auto models = m_ArchiveModels;

        return models;
    }
};