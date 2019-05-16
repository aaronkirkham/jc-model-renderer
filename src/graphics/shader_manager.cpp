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

    std::thread([&, filename = std::move(filename), status_text_id] {
        m_ShaderBundle = FileLoader::Get()->ReadAdf(filename);

        // read the shader bundle data
        jc::AvalancheDataFormat::SInstanceInfo instance_info;
        m_ShaderBundle->GetInstance(0, &instance_info);
        m_ShaderBundle->ReadInstance(instance_info.m_NameHash, 0xF2923B32, (void**)&m_ShaderLibrary);

        UI::Get()->PopStatusText(status_text_id);

        // exit now if the shader bundle wasn't loaded
        if (!m_ShaderBundle) {
            Window::Get()->ShowMessageBox("Failed to load shader bundle. Some features won't be available.\n\nPlease "
                                          "make sure your Just Cause directory is valid.",
                                          MB_ICONWARNING | MB_OK);
            return;
        }

#if DUMP_SHADERS
        std::filesystem::path path = "C:/users/aaron/desktop/shaders_jc4";
        std::filesystem::create_directories(path);

        std::filesystem::path bin_path = path / "bin";
        std::filesystem::create_directories(bin_path);

        static const auto DumpShader = [&](const std::string& shader_name, const std::string& extension,
                                           const FileBuffer& shader_data) {
            ID3DBlob* blob = nullptr;
            auto      hr   = D3DDisassemble(shader_data.data(), shader_data.size(), 0, nullptr, &blob);
            if (FAILED(hr)) {
                __debugbreak();
            }

            auto shader_path = bin_path / shader_name;
            shader_path += extension;
            std::ofstream stream2(shader_path, std::ios::binary);
            stream2.write((char*)shader_data.data(), shader_data.size());
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

        for (const auto& member : m_ShaderBundle->GetMember("VertexShaders")->m_Members) {
            const auto& shader_name = m_ShaderBundle->GetMember(member.get(), "Name")->m_StringData;
            const auto& shader_data = m_ShaderBundle->GetMember(member.get(), "BinaryData")->m_Data;

            DumpShader(shader_name, ".vb", shader_data);

#if 0
            // ID3D11VertexShader* shader = nullptr;
            // auto result = Renderer::Get()->GetDevice()->CreateVertexShader(shader_data.data(),
            // shader_data.size(),
            //                                                               nullptr, &shader);
            // assert(SUCCEEDED(result));

            ID3D11ShaderReflection* reflector = nullptr;
            auto                    result =
                D3DReflect(shader_data.data(), shader_data.size(), IID_ID3D11ShaderReflection, (void**)&reflector);

            D3D11_SHADER_DESC desc{};
            reflector->GetDesc(&desc);

            if (desc.ConstantBuffers == 0) {
                continue;
            }

            for (UINT i = 0; i < desc.ConstantBuffers; ++i) {
                D3D11_SHADER_BUFFER_DESC buffer_desc{};
                auto                     cb = reflector->GetConstantBufferByIndex(i);
                cb->GetDesc(&buffer_desc);

                SPDLOG_INFO(buffer_desc.Name);
            }
#endif
        }

        for (const auto& member : m_ShaderBundle->GetMember("FragmentShaders")->m_Members) {
            const auto& shader_name = m_ShaderBundle->GetMember(member.get(), "Name")->m_StringData;
            const auto& shader_data = m_ShaderBundle->GetMember(member.get(), "BinaryData")->m_Data;

            DumpShader(shader_name, ".fb", shader_data);
        }
#endif
    }).detach();
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
