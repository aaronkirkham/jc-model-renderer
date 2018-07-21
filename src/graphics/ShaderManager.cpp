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

std::shared_ptr<VertexShader_t> ShaderManager::GetVertexShader(const std::string& name)
{
    auto filename = (name + ".vb");
    auto key = fnv_1_32::hash(filename.c_str(), filename.length());

    auto it = m_VertexShaders.find(key);
    if (it != std::end(m_VertexShaders)) {
        return it->second;
    }

    std::stringstream file;
    file << "../assets/shaders/" << name << ".vb";

    // read
    std::ifstream stream(file.str(), std::ios::binary | std::ios::ate);
    if (stream.fail()) {
        DEBUG_LOG("ShaderManager::GetVertexShader - couldn't open \"" << file.str() << "\".");
        Window::Get()->ShowMessageBox("Couldn't open shader.", MB_ICONERROR | MB_OK);
        return nullptr;
    }

    // resize
    auto size = stream.tellg();
    FileBuffer data;
    data.resize(size);

    // read
    stream.seekg(0, std::ios::beg);
    stream.read((char *)data.data(), size);
    stream.close();

    // create the shader instance
    auto shader = std::make_shared<VertexShader_t>();
    shader->m_Code = std::move(data);
    shader->m_Size = size;

    auto result = Renderer::Get()->GetDevice()->CreateVertexShader(shader->m_Code.data(), shader->m_Size, nullptr, &shader->m_Shader);
    assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
    D3D_SET_OBJECT_NAME_A(shader->m_Shader, name.c_str());
#endif

    if (FAILED(result)) {
        return nullptr;
    }

    m_VertexShaders[key] = std::move(shader);
    DEBUG_LOG("ShaderManager: Cached vertex shader '" << name.c_str() << "'");
    return m_VertexShaders[key];
}

std::shared_ptr<PixelShader_t> ShaderManager::GetPixelShader(const std::string& name)
{
    auto filename = (name + ".fb");
    auto key = fnv_1_32::hash(filename.c_str(), filename.length());

    auto it = m_PixelShaders.find(key);
    if (it != std::end(m_PixelShaders)) {
        return it->second;
    }

    std::stringstream file;
    file << "../assets/shaders/" << name << ".fb";

    // read
    std::ifstream stream(file.str(), std::ios::binary | std::ios::ate);
    if (stream.fail()) {
        DEBUG_LOG("ShaderManager::GetPixelShader - couldn't open \"" << file.str() << "\".");
        Window::Get()->ShowMessageBox("Couldn't open shader.", MB_ICONERROR | MB_OK);
        return nullptr;
    }

    // resize
    auto size = stream.tellg();
    FileBuffer data;
    data.resize(size);

    // read
    stream.seekg(0, std::ios::beg);
    stream.read((char *)data.data(), size);
    stream.close();

    // create the shader instance
    auto shader = std::make_shared<PixelShader_t>();
    shader->m_Code = std::move(data);
    shader->m_Size = size;

    auto result = Renderer::Get()->GetDevice()->CreatePixelShader(shader->m_Code.data(), shader->m_Size, nullptr, &shader->m_Shader);
    assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
    D3D_SET_OBJECT_NAME_A(shader->m_Shader, name.c_str());
#endif

    if (FAILED(result)) {
        return nullptr;
    }

    m_PixelShaders[key] = std::move(shader);
    DEBUG_LOG("ShaderManager: Cached pixel shader '" << name.c_str() << "'");
    return m_PixelShaders[key];
}