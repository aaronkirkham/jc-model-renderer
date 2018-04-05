#include <graphics/TextureManager.h>
#include <graphics/Renderer.h>
#include <Window.h>
#include <jc3/FileLoader.h>
#include <fnv1.h>

#include <graphics/UI.h>

Texture::Texture(const fs::path& filename)
    : m_Filename(filename)
{
    if (m_Filename.extension() == ".ddsc") {
        FileLoader::Get()->ReadFile(filename, [this](bool success, FileBuffer data) {
            DEBUG_LOG("Texture::Texture READFILE -> Finished reading DDSC buffer.");

            // TODO: only if success
            // TODO: what about if we load custom textures from disk??

            if (success) {
                m_DDSCBuffer = std::move(data);
                m_HasDDSC = true;
            }

            m_IsTryingDDSC = false;
        }, NO_TEX_CACHE);

        // try the HMDDSC file

        auto hmddsc_filename = m_Filename.replace_extension(".hmddsc");
        m_IsTryingHMDDSC = true;

        FileLoader::Get()->ReadFile(hmddsc_filename, [this](bool success, FileBuffer data) {
            DEBUG_LOG("Texture::Texture READFILE -> Finished reading HMDDSC buffer.");

            if (success) {
                m_HMDDSCBuffer = std::move(data);
                m_HasHMDDSC = true;
            }

            m_IsTryingHMDDSC = false;
        }, NO_TEX_CACHE);

        // launch the wait thread
        m_WaitThread = std::thread([this] {
            std::stringstream status_text;
            status_text << "Loading texture \"" << m_Filename << "\"...";
            const auto status_text_id = UI::Get()->PushStatusText(status_text.str());

            while (true) {

                // wait for hmddsc if we need to
                if (!m_IsTryingDDSC && !m_IsTryingHMDDSC) {

                    // do we have a buffer?
                    if (m_HasDDSC) {
                        FileBuffer out;
                        if (FileLoader::Get()->ReadCompressedTexture(&m_DDSCBuffer, (m_HasHMDDSC ? &m_HMDDSCBuffer : nullptr), &out)) {

                            DEBUG_LOG("Texture::Texture (" << m_Filename.string() << ") -> finished parsing compressed texture.");
                            DEBUG_LOG(" -> Has DDSC source ? " << (m_HasDDSC ? "yes" : "no"));
                            DEBUG_LOG(" -> Has HMDDSC source ? " << (m_HasHMDDSC ? "yes" : "no"));

                            LoadFromBuffer(out);
                        }
                    }

                    UI::Get()->PopStatusText(status_text_id);

                    break;

                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });

        m_WaitThread.detach();
    }
}

Texture::~Texture()
{
    DEBUG_LOG("Deleting texture '" << m_Filename.string().c_str() << "'...");

    safe_release(m_SRV);
    safe_release(m_Texture);
}

bool Texture::LoadFromBuffer(const FileBuffer& buffer)
{
    if (buffer.empty()) {
        return false;
    }

    safe_release(m_SRV);
    safe_release(m_Texture);

    // create the dds texture resources
    auto result = DirectX::CreateDDSTextureFromMemory(Renderer::Get()->GetDevice(), buffer.data(), buffer.size(), &m_Texture, &m_SRV);
    //assert(SUCCEEDED(result));

    // store the buffer
    m_Buffer = std::move(buffer);

    if (FAILED(result)) {
        DEBUG_LOG("[ERROR] Failed to create texture '" << m_Filename.filename().string() << "'.");
        return false;
    }

    DEBUG_LOG("Texture::Create - '" << m_Filename.filename().string() << "', m_Texture=" << m_Texture << ", SRV=" << m_SRV);

    return SUCCEEDED(result);
}

bool Texture::LoadFromFile(const fs::path& filename)
{
    std::ifstream stream(filename.c_str(), std::ios::binary | std::ios::ate);
    if (stream.fail()) {
        DEBUG_LOG("[ERROR] Failed to create texture from file '" << filename.filename().string() << "'.");
        return false;
    }

    const auto length = stream.tellg();
    stream.seekg(0, std::ios::beg);

    FileBuffer buffer;
    buffer.resize(length);
    stream.read((char *)buffer.data(), length);

    DEBUG_LOG("Texture::LoadFromFile - Read " << length << " bytes from " << filename.filename().string());

    auto result = LoadFromBuffer(buffer);
    stream.close();
    return result;
}

void Texture::Use(uint32_t slot)
{
    assert(m_SRV != nullptr);
    Renderer::Get()->GetDeviceContext()->PSSetShaderResources(slot, 1, &m_SRV);
}

TextureManager::TextureManager()
{
    m_MissingTexture = std::make_unique<Texture>();
    m_MissingTexture->LoadFromFile("E:/jc3-rbm-renderer/assets/missing-texture.dds");
}

void TextureManager::Shutdown()
{
    m_Textures.clear();
    m_LastUsedTextures.clear();
    m_MissingTexture = nullptr;
}

const std::shared_ptr<Texture>& TextureManager::GetTexture(const fs::path& filename, bool create_if_not_exists)
{
    auto name = filename.filename().stem().string();
    auto key = fnv_1_32::hash(name.c_str(), name.length());

    if (std::find(m_LastUsedTextures.begin(), m_LastUsedTextures.end(), key) == m_LastUsedTextures.end()) {
        m_LastUsedTextures.emplace_back(key);
    }

    // if we have already cached this texture, use that
    auto it = m_Textures.find(key);
    if (it != m_Textures.end()) {
        DEBUG_LOG("TextureManager::GetTexture - Using cached texture '" << name.c_str() << "' (refcount: " << it->second.use_count() << ")");
        return it->second;
    }

    // do we want to create the texture?
    if (create_if_not_exists) {
        m_Textures[key] = std::make_shared<Texture>(filename);
    }

    return m_Textures[key];
}

void TextureManager::Flush()
{
    for (auto it = m_LastUsedTextures.begin(); it != m_LastUsedTextures.end();) {
        auto texture = *it;

        auto texture_it = m_Textures.find(texture);
        if (texture_it != m_Textures.end() && texture_it->second.use_count() == 1) {
            m_Textures.erase(texture);
            it = m_LastUsedTextures.erase(it);
        }
        else {
            ++it;
        }
    }
}

void TextureManager::Empty()
{
    m_Textures.clear();
    m_LastUsedTextures.clear();
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