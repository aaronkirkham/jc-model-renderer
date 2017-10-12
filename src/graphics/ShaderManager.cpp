#include <graphics/ShaderManager.h>
#include <graphics/Renderer.h>
#include <Window.h>
#include <fnv1.h>

#include <streambuf>

std::shared_ptr<VertexShader_t> ShaderManager::GetVertexShader(const fs::path& file)
{
    auto key = fnv_1_32::hash(file.string().c_str(), file.string().length());

    auto it = m_VertexShaders.find(key);
    if (it != std::end(m_VertexShaders)) {
        return it->second;
    }

    // read the shader file contents
    std::ifstream contents(file, std::ios::binary);
    std::vector<uint8_t> bytes = { std::istreambuf_iterator<char>(contents), std::istreambuf_iterator<char>() };

    // create the shader instance
    VertexShader_t shader;
    shader.m_Shader = nullptr;
    shader.m_Code = std::move(bytes);
    shader.m_Size = shader.m_Code.size();

    auto result = Renderer::Get()->GetDevice()->CreateVertexShader(shader.m_Code.data(), shader.m_Size, nullptr, &shader.m_Shader);
    assert(SUCCEEDED(result));

    if (FAILED(result)) {
        return nullptr;
    }

    m_VertexShaders[key] = std::make_shared<VertexShader_t>(shader);

    Window::Get()->DebugString("VertexShader: Cached vertex shader '%s'.", file.filename().string().c_str());

    return m_VertexShaders[key];
}

std::shared_ptr<PixelShader_t> ShaderManager::GetPixelShader(const fs::path& file)
{
    auto key = fnv_1_32::hash(file.string().c_str(), file.string().length());

    auto it = m_PixelShaders.find(key);
    if (it != std::end(m_PixelShaders)) {
        return it->second;
    }

    // read the shader file contents
    std::ifstream contents(file, std::ios::binary);
    std::vector<uint8_t> bytes = { std::istreambuf_iterator<char>(contents), std::istreambuf_iterator<char>() };

    // create the shader instance
    PixelShader_t shader;
    shader.m_Shader = nullptr;
    shader.m_Code = std::move(bytes);
    shader.m_Size = shader.m_Code.size();

    auto result = Renderer::Get()->GetDevice()->CreatePixelShader(shader.m_Code.data(), shader.m_Size, nullptr, &shader.m_Shader);
    assert(SUCCEEDED(result));

    if (FAILED(result)) {
        return nullptr;
    }

    m_PixelShaders[key] = std::make_shared<PixelShader_t>(shader);

    Window::Get()->DebugString("PixelShader: Cached pixel shader '%s'.", file.filename().string().c_str());

    return m_PixelShaders[key];
}