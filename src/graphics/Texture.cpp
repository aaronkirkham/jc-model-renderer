#include <Window.h>
#include <graphics/DDSTextureLoader.h>
#include <graphics/Renderer.h>
#include <graphics/Texture.h>
#include <jc3/hashlittle.h>

Texture::Texture(const fs::path& filename)
    : m_Filename(filename)
    , m_NameHash(hashlittle(m_Filename.generic_string().c_str()))
{
}

Texture::~Texture()
{
    DEBUG_LOG("Texture::~Texture - Deleting texture '" << m_Filename.filename() << "'...");

    SAFE_RELEASE(m_SRV);
    SAFE_RELEASE(m_Texture);
}

bool Texture::LoadFromBuffer(FileBuffer* buffer)
{
    if (buffer->empty()) {
        return false;
    }

#ifdef DEBUG
    if (m_SRV || m_Texture) {
        DEBUG_LOG("Texture::LoadFromBuffer - texture already exists. deleting old one...");
    }
#endif

    SAFE_RELEASE(m_SRV);
    SAFE_RELEASE(m_Texture);

    // create the dds texture resources
    const auto renderer = Renderer::Get();
    auto       result   = DirectX::CreateDDSTextureFromMemoryEx(renderer->GetDevice(), renderer->GetDeviceContext(),
                                                        buffer->data(), buffer->size(), 0, D3D11_USAGE_DEFAULT,
                                                        D3D11_BIND_SHADER_RESOURCE, 0, 0, false, &m_Texture, &m_SRV);

    // get the texture size
    if (SUCCEEDED(result)) {
        ID3D11Texture2D* _tex = nullptr;
        result                = m_Texture->QueryInterface(&_tex);
        assert(SUCCEEDED(result));

        D3D11_TEXTURE2D_DESC desc;
        _tex->GetDesc(&desc);
        _tex->Release();

        m_Size = {desc.Width, desc.Height};

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        auto& fn = m_Filename.filename().string();
        D3D_SET_OBJECT_NAME_A(m_Texture, fn.c_str());
        fn += " (SRV)";
        D3D_SET_OBJECT_NAME_A(m_Texture, fn.c_str());
#endif
    }

    // store the buffer
    m_Buffer = *buffer;

    if (FAILED(result)) {
        DEBUG_LOG("[ERROR] Texture::LoadFromBuffer - Failed to create texture '" << m_Filename.filename() << "'.");
        return false;
    }

    DEBUG_LOG("Texture::Create - '" << m_Filename.filename() << "', m_Texture=" << m_Texture << ", SRV=" << m_SRV);

    return SUCCEEDED(result);
}

bool Texture::LoadFromFile(const fs::path& filename)
{
    m_Filename = filename;

    const auto size = fs::file_size(filename);

    std::ifstream stream(filename.c_str(), std::ios::binary);
    if (stream.fail()) {
        DEBUG_LOG("[ERROR] Failed to create texture from file '" << filename.filename() << "'.");
        return false;
    }

    FileBuffer buffer;
    buffer.resize(size);
    stream.read((char*)buffer.data(), size);

    DEBUG_LOG("Texture::LoadFromFile - Read " << size << " bytes from " << filename.filename());

    auto result = LoadFromBuffer(&buffer);
    stream.close();
    return result;
}

void Texture::Use(uint32_t slot)
{
    assert(m_SRV != nullptr);
    Renderer::Get()->GetDeviceContext()->PSSetShaderResources(slot, 1, &m_SRV);
}

bool Texture::IsLoaded() const
{
    return (m_Texture != nullptr && m_SRV != nullptr);
}

const fs::path& Texture::GetPath() const
{
    return m_Filename;
}

const uint32_t Texture::GetHash() const
{
    return m_NameHash;
}

const glm::vec2& Texture::GetSize() const
{
    return m_Size;
}

const FileBuffer& Texture::GetBuffer() const
{
    return m_Buffer;
}

ID3D11Resource* Texture::GetResource() const
{
    return m_Texture;
}

ID3D11ShaderResourceView* Texture::GetSRV() const
{
    return m_SRV;
}
