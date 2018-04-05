#pragma once

#include <StdInc.h>
#include <singleton.h>
#include <graphics/DDSTextureLoader.h>
#include <thread>

class Texture
{
private:
    ID3D11Resource* m_Texture = nullptr;
    ID3D11ShaderResourceView* m_SRV = nullptr;
    fs::path m_Filename = "";
    FileBuffer m_Buffer;

    FileBuffer m_DDSCBuffer;
    FileBuffer m_HMDDSCBuffer;

    bool m_HasDDSC = false;
    bool m_HasHMDDSC = false;
    bool m_IsTryingDDSC = true;
    bool m_IsTryingHMDDSC = false;

    std::thread m_WaitThread;

public:
    Texture() = default;
    Texture(const fs::path& filename);
    virtual ~Texture();

    bool LoadFromBuffer(const FileBuffer& buffer);
    bool LoadFromFile(const fs::path& filename);
    void Use(uint32_t slot);

    bool IsLoaded() { return (m_Texture != nullptr && m_SRV != nullptr); }
    const fs::path& GetPath() { return m_Filename; }

    ID3D11Resource* GetResource() { return m_Texture; }
    ID3D11ShaderResourceView* GetSRV() { return m_SRV; }

    const FileBuffer& GetBuffer() { return m_Buffer; }
};

class TextureManager : public Singleton<TextureManager>
{
private:
    std::unordered_map<uint32_t, std::shared_ptr<Texture>> m_Textures;
    std::vector<uint32_t> m_LastUsedTextures;
    std::unique_ptr<Texture> m_MissingTexture;

public:
    TextureManager();
    virtual ~TextureManager() = default;

    void Shutdown();

    const std::shared_ptr<Texture>& GetTexture(const fs::path& filename, bool create_if_not_exists = true);
    void Flush();
    void Empty();

    static DDS_PIXELFORMAT GetPixelFormat(DXGI_FORMAT format);

    const std::unordered_map<uint32_t, std::shared_ptr<Texture>>& GetCache() { return m_Textures; }
    std::size_t GetCacheSize() { return m_Textures.size(); }

    Texture* GetMissingTexture() { return m_MissingTexture.get(); }
};