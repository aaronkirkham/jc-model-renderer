#include <Window.h>
#include <fnv1.h>
#include <graphics/Renderer.h>
#include <graphics/ShaderManager.h>

#include <streambuf>

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

#ifdef DEBUG
    fs::path file = "../assets/shaders/" + name + ".vb";
#else
    fs::path file = "assets/shaders/" + name + ".vb";
#endif

    // read
    std::ifstream stream(file, std::ios::binary);
    if (stream.fail()) {
        DEBUG_LOG("[ERROR] ShaderManager::GetVertexShader - couldn't open \"" << file << "\".");
        Window::Get()->ShowMessageBox("Failed to open shader \"" + file.generic_string() + "\".", MB_ICONERROR | MB_OK);
        return nullptr;
    }

    const auto file_size = fs::file_size(file);

    // resize
    FileBuffer data;
    data.resize(file_size);

    // read
    stream.read((char*)data.data(), file_size);
    stream.close();

    // create the shader instance
    auto shader    = std::make_shared<VertexShader_t>();
    shader->m_Code = std::move(data);
    shader->m_Size = file_size;

    auto result = Renderer::Get()->GetDevice()->CreateVertexShader(shader->m_Code.data(), shader->m_Size, nullptr,
                                                                   &shader->m_Shader);
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
    auto key      = fnv_1_32::hash(filename.c_str(), filename.length());

    auto it = m_PixelShaders.find(key);
    if (it != std::end(m_PixelShaders)) {
        return it->second;
    }

#ifdef DEBUG
    fs::path file = "../assets/shaders/" + name + ".fb";
#else
    fs::path file = "assets/shaders/" + name + ".fb";
#endif

    // read
    std::ifstream stream(file, std::ios::binary);
    if (stream.fail()) {
        DEBUG_LOG("[ERROR] ShaderManager::GetPixelShader - couldn't open \"" << file << "\".");
        Window::Get()->ShowMessageBox("Failed to open shader \"" + file.generic_string() + "\".", MB_ICONERROR | MB_OK);
        return nullptr;
    }

    const auto file_size = fs::file_size(file);

    // resize
    FileBuffer data;
    data.resize(file_size);

    // read
    stream.read((char*)data.data(), file_size);
    stream.close();

    // create the shader instance
    auto shader    = std::make_shared<PixelShader_t>();
    shader->m_Code = std::move(data);
    shader->m_Size = file_size;

    auto result = Renderer::Get()->GetDevice()->CreatePixelShader(shader->m_Code.data(), shader->m_Size, nullptr,
                                                                  &shader->m_Shader);
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
