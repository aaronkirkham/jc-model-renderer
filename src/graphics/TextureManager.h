#pragma once

#include <StdInc.h>
#include <singleton.h>
#include <graphics/DDSTextureLoader.h>

class Texture
{
private:
    ID3D11Resource* m_Texture = nullptr;
    ID3D11ShaderResourceView* m_SRV = nullptr;
    fs::path m_Filename = "";

public:
    Texture(const fs::path& filename);
    virtual ~Texture();

    bool LoadFromBuffer(const std::vector<uint8_t>& buffer);
    void Use(uint32_t slot);

    bool IsLoaded() { return (m_Texture != nullptr && m_SRV != nullptr); }
    const fs::path& GetPath() { return m_Filename; }
};

class TextureManager : public Singleton<TextureManager>
{
private:
    std::unordered_map<uint32_t, std::shared_ptr<Texture>> m_Textures;
    std::vector<uint32_t> m_LastUsedTextures;

public:
    TextureManager() = default;
    virtual ~TextureManager() = default;

    std::shared_ptr<Texture> GetTexture(const fs::path& filename);
    void Flush();

    static DDS_PIXELFORMAT GetPixelFormat(DXGI_FORMAT format);

    const std::unordered_map<uint32_t, std::shared_ptr<Texture>>& GetCache() { return m_Textures; }
    std::size_t GetCacheSize() { return m_Textures.size(); }
};