#include <graphics/TextureManager.h>
#include <graphics/Renderer.h>
#include <Window.h>
#include <jc3/FileLoader.h>
#include <fnv1.h>

Texture::Texture(const fs::path& filename)
    : m_Filename(filename)
{
    FileLoader::Get()->ReadFile(m_Filename, [this](bool success, std::vector<uint8_t> data) {
        if (success) {
            std::vector<uint8_t> buffer;
            if (m_Filename.extension() == ".ddsc") {
                FileLoader::Get()->ReadCompressedTexture(data, std::numeric_limits<uint64_t>::max(), &buffer);
            }
            else {
                buffer = std::move(data);
            }

            LoadFromBuffer(buffer);
        }
        // failed, let's look for a HMDDSC file.
        else {
            DEBUG_LOG("[ERROR] can't find texture " << m_Filename.string() << " :(");

            if (m_Filename.extension() == ".ddsc") {
                auto new_filename = m_Filename.replace_extension(".hmddsc");
                DEBUG_LOG("looking for " << new_filename.string());

                FileLoader::Get()->ReadFile(new_filename, [this](bool success, std::vector<uint8_t> data) {
                    if (success) {
                        std::vector<uint8_t> buffer;
                        FileLoader::Get()->ReadCompressedTexture(data, std::numeric_limits<uint64_t>::max(), &buffer);

                        LoadFromBuffer(buffer);
                    }
                    else {
                        DEBUG_LOG("[ERROR 2] can't find HMDDSC texture " << m_Filename.string() << " :(:(");
                    }
                });
            }
        }
    });
}

Texture::~Texture()
{
    DEBUG_LOG("Deleting texture '" << m_Filename.string().c_str() << "'...");

    safe_release(m_SRV);
    safe_release(m_Texture);
}

bool Texture::LoadFromBuffer(const std::vector<uint8_t>& buffer)
{
    if (buffer.empty()) {
        return false;
    }

    safe_release(m_SRV);
    safe_release(m_Texture);

    auto result = DirectX::CreateDDSTextureFromMemory(Renderer::Get()->GetDevice(), buffer.data(), buffer.size(), &m_Texture, &m_SRV);
    //assert(SUCCEEDED(result));

    if (FAILED(result)) {
        DEBUG_LOG("[ERROR] Failed to create texture '" << m_Filename.filename().string().c_str() << "'.");
        return false;
    }

    DEBUG_LOG("Texture::Create '" << m_Filename.filename().string().c_str() << "', m_Texture=" << m_Texture << ", SRV=" << m_SRV);

    return SUCCEEDED(result);
}

void Texture::Use(uint32_t slot)
{
    assert(m_SRV != nullptr);
    Renderer::Get()->GetDeviceContext()->PSSetShaderResources(slot, 1, &m_SRV);
}

std::shared_ptr<Texture> TextureManager::GetTexture(const fs::path& filename)
{
    auto name = filename.filename().string();
    auto key = fnv_1_32::hash(name.c_str(), name.length());

    if (std::find(std::begin(m_LastUsedTextures), std::end(m_LastUsedTextures), key) == std::end(m_LastUsedTextures)) {
        m_LastUsedTextures.emplace_back(key);
    }

    // if we have already cached this texture, use that
    auto it = m_Textures.find(key);
    if (it != std::end(m_Textures)) {
        DEBUG_LOG("INFO: Using cached texture '" << name.c_str() << "' (refcount: " << it->second.use_count() << ")");
        return it->second;
    }

    m_Textures[key] = std::make_shared<Texture>(filename);
    return m_Textures[key];
}

void TextureManager::Flush()
{
    for (auto it = m_LastUsedTextures.begin(); it != m_LastUsedTextures.end();) {
        auto texture = *it;

        auto texture_it = m_Textures.find(texture);
        if (texture_it != std::end(m_Textures) && texture_it->second.use_count() == 1) {
            m_Textures.erase(texture);
            it = m_LastUsedTextures.erase(it);
        }
        else {
            ++it;
        }
    }
}

DDS_PIXELFORMAT TextureManager::GetPixelFormat(DXGI_FORMAT format)
{
    DDS_PIXELFORMAT pixelFormat;
    pixelFormat.size = 32;

    switch (format)
    {
    case DXGI_FORMAT_BC1_UNORM: {
        pixelFormat.flags = DDS_FOURCC;
        pixelFormat.RGBBitCount = 0;
        pixelFormat.RBitMask = 0;
        pixelFormat.GBitMask = 0;
        pixelFormat.BBitMask = 0;
        pixelFormat.ABitMask = 0;
        pixelFormat.fourCC = 0x31545844; // DXT1
        break;
    }

    case DXGI_FORMAT_BC2_UNORM: {
        pixelFormat.flags = DDS_FOURCC;
        pixelFormat.RGBBitCount = 0;
        pixelFormat.RBitMask = 0;
        pixelFormat.GBitMask = 0;
        pixelFormat.BBitMask = 0;
        pixelFormat.ABitMask = 0;
        pixelFormat.fourCC = 0x33545844; // DXT3
        break;
    }

    case DXGI_FORMAT_BC3_UNORM: {
        pixelFormat.flags = DDS_FOURCC;
        pixelFormat.RGBBitCount = 0;
        pixelFormat.RBitMask = 0;
        pixelFormat.GBitMask = 0;
        pixelFormat.BBitMask = 0;
        pixelFormat.ABitMask = 0;
        pixelFormat.fourCC = 0x35545844; // DXT5
        break;
    }

    case DXGI_FORMAT_R8G8B8A8_UNORM: {
        pixelFormat.flags = (DDS_RGB | DDS_ALPHA);
        pixelFormat.RGBBitCount = 32; // A8B8G8R8
        pixelFormat.RBitMask = 0x00FF0000;
        pixelFormat.GBitMask = 0x0000FF00;
        pixelFormat.BBitMask = 0x000000FF;
        pixelFormat.ABitMask = 0xFF000000;
        pixelFormat.fourCC = 0;
        break;
    }

    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC7_UNORM: {
        pixelFormat.fourCC = 0x30315844;
        break;
    }
    }

    return pixelFormat;
}