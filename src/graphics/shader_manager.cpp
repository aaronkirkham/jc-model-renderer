#include <fstream>

#include "../fnv1.h"
#include "../game/file_loader.h"
#include "../settings.h"
#include "../window.h"
#include "renderer.h"
#include "shader_manager.h"
#include "types.h"

extern bool g_IsJC4Mode;

#if DUMP_SHADERS
#pragma comment(lib, "D3dcompiler.lib")
#include <d3dcompiler.h>
#endif

void ShaderManager::Init()
{
    auto& filename = Window::Get()->GetJustCauseDirectory();
    filename /= g_IsJC4Mode ? "ShadersDX11_F.shader_bundle" : "Shaders_F.shader_bundle";

    std::string status_text    = "Loading \"" + filename.filename().string() + "\"...";
    const auto  status_text_id = UI::Get()->PushStatusText(status_text);

    // read the shader bundle
    FileLoader::Get()->ReadFileFromDisk(filename, [&, filename, status_text_id](bool success, FileBuffer data) {
        if (success) {
            m_ShaderBundle = AvalancheDataFormat::make(filename, std::move(data));

            // read the shader bundle data
            jc::AvalancheDataFormat::SInstanceInfo instance_info;
            m_ShaderBundle->GetInstance(0, &instance_info);
            m_ShaderBundle->ReadInstance(instance_info.m_NameHash, 0xF2923B32, (void**)&m_ShaderLibrary);
        } else {
            Window::Get()->ShowMessageBox("Failed to load shader bundle. Some features won't be available.\n\nPlease "
                                          "make sure your Just Cause directory is valid.",
                                          MB_ICONWARNING | MB_OK);
        }

        UI::Get()->PopStatusText(status_text_id);
    });

#if DUMP_SHADERS
    std::filesystem::path path = "C:/users/aaron/desktop/shaders_jc4";
    std::filesystem::create_directories(path);

    std::filesystem::path bin_path = path / "bin";
    std::filesystem::create_directories(bin_path);

    static const auto DumpShader = [&](const std::string& shader_name, const std::string& extension, const void* data,
                                       uint64_t data_size) {
        ID3DBlob* blob = nullptr;
        auto      hr   = D3DDisassemble(data, data_size, 0, nullptr, &blob);
        if (FAILED(hr)) {
            __debugbreak();
        }

        auto shader_path = bin_path / shader_name;
        shader_path += extension;
        std::ofstream stream2(shader_path, std::ios::binary);
        stream2.write((char*)data, data_size);
        stream2.close();

        std::string str((const char*)blob->GetBufferPointer(), blob->GetBufferSize());
        // str = str.substr(0, str.find("vs_5_0"));

        auto file_path = path / shader_name;
        file_path += extension;
        file_path += ".txt";
        std::ofstream stream(file_path);
        stream << str;
        stream.close();

        blob->Release();
    };

    // vertex shaders
    for (uint32_t i = 0; i < m_ShaderLibrary->m_VertexShaders.m_Count; ++i) {
        const auto& shader = m_ShaderLibrary->m_VertexShaders.m_Data[i];
        DumpShader(shader.m_Name, ".vb", shader.m_BinaryData.m_Data, shader.m_BinaryData.m_Count);
    }

    // fragment shaders
    for (uint32_t i = 0; i < m_ShaderLibrary->m_FragmentShaders.m_Count; ++i) {
        const auto& shader = m_ShaderLibrary->m_FragmentShaders.m_Data[i];
        DumpShader(shader.m_Name, ".fb", shader.m_BinaryData.m_Data, shader.m_BinaryData.m_Count);
    }
#endif
}

void ShaderManager::Shutdown()
{
    Empty();
    SAFE_DELETE(m_ShaderLibrary);
    m_ShaderBundle = nullptr;
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

    // ensure the shader bundle is loaded
    if (!m_ShaderBundle || !m_ShaderLibrary) {
        SPDLOG_ERROR("Can't create vertex shader because shader bundle isn't loaded!");
        return nullptr;
    }

    // find the shader in the library
    for (uint32_t i = 0; i < m_ShaderLibrary->m_VertexShaders.m_Count; ++i) {
        const auto shader = m_ShaderLibrary->m_VertexShaders.m_Data[i];
        if (name == shader.m_Name) {
            auto vs    = std::make_shared<VertexShader_t>();
            vs->m_Code = shader.m_BinaryData.m_Data;
            vs->m_Size = shader.m_BinaryData.m_Count;

            const auto result = Renderer::Get()->GetDevice()->CreateVertexShader(
                shader.m_BinaryData.m_Data, shader.m_BinaryData.m_Count, nullptr, &vs->m_Shader);
            if (FAILED(result)) {
#ifdef DEBUG
                SPDLOG_ERROR("Failed to create vertex shader!");
                __debugbreak();
#endif
                return nullptr;
            }

#ifdef RENDERER_REPORT_LIVE_OBJECTS
            D3D_SET_OBJECT_NAME_A(vs->m_Shader, name.c_str());
#endif

            m_VertexShaders[key] = std::move(vs);
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

    // ensure the shader bundle is loaded
    if (!m_ShaderBundle || !m_ShaderLibrary) {
        SPDLOG_ERROR("Can't create pixel shader because shader bundle isn't loaded!");
        return nullptr;
    }

    // find the shader in the library
    for (uint32_t i = 0; i < m_ShaderLibrary->m_FragmentShaders.m_Count; ++i) {
        const auto shader = m_ShaderLibrary->m_FragmentShaders.m_Data[i];
        if (name == shader.m_Name) {
            auto ps    = std::make_shared<PixelShader_t>();
            ps->m_Code = shader.m_BinaryData.m_Data;
            ps->m_Size = shader.m_BinaryData.m_Count;

            const auto result = Renderer::Get()->GetDevice()->CreatePixelShader(
                shader.m_BinaryData.m_Data, shader.m_BinaryData.m_Count, nullptr, &ps->m_Shader);
            if (FAILED(result)) {
#ifdef DEBUG
                SPDLOG_ERROR("Failed to create pixel shader!");
                __debugbreak();
#endif
                return nullptr;
            }

#ifdef RENDERER_REPORT_LIVE_OBJECTS
            D3D_SET_OBJECT_NAME_A(vs->m_Shader, name.c_str());
#endif

            m_PixelShaders[key] = std::move(ps);
            return m_PixelShaders[key];
        }
    }

    return nullptr;
}
