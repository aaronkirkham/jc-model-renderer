#include <graphics/ShaderManager.h>
#include <graphics/Renderer.h>
#include <Window.h>
#include <fnv1.h>

#include <streambuf>

void ShaderManager::Shutdown()
{
    m_VertexShaders.clear();
    m_PixelShaders.clear();
}

std::shared_ptr<VertexShader_t> ShaderManager::GetVertexShader(const std::string& name, const void* buffer, uint64_t size)
{
    auto filename = (name + ".vs");
    auto key = fnv_1_32::hash(filename.c_str(), filename.length());

    auto it = m_VertexShaders.find(key);
    if (it != std::end(m_VertexShaders)) {
        return it->second;
    }

    // copy the buffer
    FileBuffer data;
    data.resize(size);
    memcpy(data.data(), buffer, size);

    // create the shader instance
    auto shader = std::make_shared<VertexShader_t>();
    shader->m_Code = std::move(data);
    shader->m_Size = size;

    auto result = Renderer::Get()->GetDevice()->CreateVertexShader(shader->m_Code.data(), shader->m_Size, nullptr, &shader->m_Shader);
    assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
    D3D_SET_OBJECT_NAME_A(shader->m_Shader, "ShaderManager::GetVertexShader");
#endif

    if (FAILED(result)) {
        return nullptr;
    }

    m_VertexShaders[key] = std::move(shader);
    DEBUG_LOG("ShaderManager: Cached vertex shader '" << name.c_str() << "'");
    return m_VertexShaders[key];
}

std::shared_ptr<PixelShader_t> ShaderManager::GetPixelShader(const std::string& name, const void* buffer, uint64_t size)
{
    auto filename = (name + ".ps");
    auto key = fnv_1_32::hash(filename.c_str(), filename.length());

    auto it = m_PixelShaders.find(key);
    if (it != std::end(m_PixelShaders)) {
        return it->second;
    }

    // copy the buffer
    FileBuffer data;
    data.resize(size);
    memcpy(data.data(), buffer, size);

    // create the shader instance
    auto shader = std::make_shared<PixelShader_t>();
    shader->m_Code = std::move(data);
    shader->m_Size = size;

    auto result = Renderer::Get()->GetDevice()->CreatePixelShader(shader->m_Code.data(), shader->m_Size, nullptr, &shader->m_Shader);
    assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
    D3D_SET_OBJECT_NAME_A(shader->m_Shader, "ShaderManager::GetPixelShader");
#endif

    if (FAILED(result)) {
        return nullptr;
    }

    m_PixelShaders[key] = std::move(shader);
    DEBUG_LOG("ShaderManager: Cached pixel shader '" << name.c_str() << "'");
    return m_PixelShaders[key];
}