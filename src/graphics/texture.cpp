#include <fstream>

#include "dds_texture_loader.h"
#include "renderer.h"
#include "texture.h"

#include "../window.h"

#include "../game/hashlittle.h"

Texture::Texture(const std::filesystem::path& filename)
    : m_Filename(filename)
    , m_NameHash(hashlittle(m_Filename.generic_string().c_str()))
{
}

Texture::Texture(const std::filesystem::path& filename, FileBuffer* buffer)
    : m_Filename(filename)
    , m_NameHash(hashlittle(m_Filename.generic_string().c_str()))
{
    LoadFromBuffer(buffer);
}

Texture::~Texture()
{
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
        SPDLOG_INFO("Deleting existing texture before creating new one...");
    }
#endif

    SAFE_RELEASE(m_SRV);
    SAFE_RELEASE(m_Texture);

    // create the dds texture resources
    auto result = DirectX::CreateDDSTextureFromMemoryEx(Renderer::Get()->GetDevice(), buffer->data(), buffer->size(), 0,
                                                        D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, false,
                                                        &m_Texture, &m_SRV);

    // get the texture size
    if (SUCCEEDED(result)) {
        ID3D11Texture2D* _tex = nullptr;
        result                = m_Texture->QueryInterface(&_tex);
        assert(SUCCEEDED(result));

        _tex->GetDesc(&m_Desc);
        _tex->Release();

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        auto& fn = m_Filename.filename().string();
        D3D_SET_OBJECT_NAME_A(m_Texture, fn.c_str());
        fn += " (SRV)";
        D3D_SET_OBJECT_NAME_A(m_SRV, fn.c_str());
#endif
    }

    // store the buffer
    m_Buffer = *buffer;

    if (FAILED(result)) {
        SPDLOG_ERROR("Failed to create texture \"{}\"", m_Filename.filename().string());
        return false;
    }

    return SUCCEEDED(result);
}

bool Texture::LoadFromFile(const std::filesystem::path& filename)
{
    std::ifstream stream(filename, std::ios::binary);
    if (stream.fail()) {
        SPDLOG_ERROR("Failed to create texture from file \"{}\"", filename.filename().string());
        Window::Get()->ShowMessageBox("Failed to open texture \"" + filename.generic_string() + "\".");
        return false;
    }

    const auto size = std::filesystem::file_size(filename);

    FileBuffer buffer;
    buffer.resize(size);
    stream.read((char*)buffer.data(), size);

    SPDLOG_INFO("Read {} bytes from \"{}\"", size, filename.filename().string());

    auto result = LoadFromBuffer(&buffer);
    stream.close();
    return result;
}

void Texture::Use(uint32_t slot, SamplerState_t* sampler)
{
    if (m_SRV) {
        Renderer::Get()->GetDeviceContext()->PSSetShaderResources(slot, 1, &m_SRV);

        if (sampler) {
            Renderer::Get()->SetSamplerState(sampler, slot);
        }
    }
}

void Texture::UseVS(uint32_t slot)
{
    if (m_SRV) {
        Renderer::Get()->GetDeviceContext()->VSSetShaderResources(slot, 1, &m_SRV);
    }
}

void Texture::SetFileName(const std::filesystem::path& filename)
{
    m_Filename = filename;
}

const std::filesystem::path& Texture::GetFileName() const
{
    return m_Filename;
}

bool Texture::IsLoaded() const
{
    return (m_Texture != nullptr && m_SRV != nullptr);
}

const uint32_t Texture::GetHash() const
{
    return m_NameHash;
}

const FileBuffer* Texture::GetBuffer() const
{
    return &m_Buffer;
}

ID3D11Resource* Texture::GetResource() const
{
    return m_Texture;
}

ID3D11ShaderResourceView* Texture::GetSRV() const
{
    return m_SRV;
}

D3D11_TEXTURE2D_DESC* Texture::GetDesc()
{
    return &m_Desc;
}
