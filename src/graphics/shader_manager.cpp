#include <fstream>

#include "../fnv1.h"
#include "../game/file_loader.h"
#include "../settings.h"
#include "../window.h"
#include "renderer.h"
#include "shader_manager.h"
#include "types.h"

void ShaderManager::Init()
{
#if 0
    const auto status_text_id = UI::Get()->PushStatusText("Loading \"Shaders_F.shader_bundle\"...");

    std::thread([&, status_text_id] {
        auto& filename = Window::Get()->GetJustCauseDirectory();
        filename /= "Shaders_F.shader_bundle";

        m_ShaderBundle = FileLoader::Get()->ReadAdf(filename);

        // exit now if the shader bundle wasn't loaded
        if (!m_ShaderBundle) {
            Window::Get()->ShowMessageBox(
                "Failed to load shader bundle.\n\nPlease make sure your Just Cause directory is valid.",
                MB_ICONERROR | MB_OK);
        }

        UI::Get()->PopStatusText(status_text_id);
    }).detach();
#endif
}

void ShaderManager::Shutdown()
{
    Empty();
}

void ShaderManager::Empty()
{
    m_VertexShaders.clear();
    m_PixelShaders.clear();
}

std::shared_ptr<VertexShader_t> ShaderManager::GetVertexShader(const std::string& name)
{
    auto filename = (name + ".vb");
    auto key      = fnv_1_32::hash(filename.c_str(), filename.length());

    auto it = m_VertexShaders.find(key);
    if (it != std::end(m_VertexShaders)) {
        return it->second;
    }

    assert(m_ShaderBundle);
    const auto& vertex_shaders = m_ShaderBundle->GetMember("VertexShaders");

    // look for the correct member
    for (const auto& member : vertex_shaders->m_Members) {
        if (member->m_Members[0]->m_StringData == name) {
            const auto& shader_data = member->m_Members[1]->m_Members[0]->m_Data;

            auto shader    = std::make_shared<VertexShader_t>();
            shader->m_Code = shader_data;
            shader->m_Size = shader_data.size();

            auto result = Renderer::Get()->GetDevice()->CreateVertexShader(shader->m_Code.data(), shader->m_Size,
                                                                           nullptr, &shader->m_Shader);
            assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
            D3D_SET_OBJECT_NAME_A(shader->m_Shader, name.c_str());
#endif

            if (FAILED(result)) {
                return nullptr;
            }

            m_VertexShaders[key] = std::move(shader);
            LOG_INFO("Cached vertex shader \"{}\"", name);
            return m_VertexShaders[key];
        }
    }

    return nullptr;
}

std::shared_ptr<PixelShader_t> ShaderManager::GetPixelShader(const std::string& name)
{
    auto filename = (name + ".fb");
    auto key      = fnv_1_32::hash(filename.c_str(), filename.length());

    auto it = m_PixelShaders.find(key);
    if (it != std::end(m_PixelShaders)) {
        return it->second;
    }

    assert(m_ShaderBundle);
    const auto& fragment_shaders = m_ShaderBundle->GetMember("FragmentShaders");

    // look for the correct member
    for (const auto& member : fragment_shaders->m_Members) {
        if (member->m_Members[0]->m_StringData == name) {
            const auto& shader_data = member->m_Members[1]->m_Members[0]->m_Data;

            auto shader    = std::make_shared<PixelShader_t>();
            shader->m_Code = shader_data;
            shader->m_Size = shader_data.size();

            auto result = Renderer::Get()->GetDevice()->CreatePixelShader(shader->m_Code.data(), shader->m_Size,
                                                                          nullptr, &shader->m_Shader);
            assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
            D3D_SET_OBJECT_NAME_A(shader->m_Shader, name.c_str());
#endif

            if (FAILED(result)) {
                return nullptr;
            }

            m_PixelShaders[key] = std::move(shader);
            LOG_INFO("Cached pixel shader \"{}\"", name);
            return m_PixelShaders[key];
        }
    }

    return nullptr;
}