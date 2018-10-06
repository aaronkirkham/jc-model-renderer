#pragma once

#include <StdInc.h>
#include <graphics/DDSTextureLoader.h>
#include <singleton.h>
#include <thread>

class Texture
{
  private:
    ID3D11Resource*           m_Texture  = nullptr;
    ID3D11ShaderResourceView* m_SRV      = nullptr;
    fs::path                  m_Filename = "";
    glm::vec2                 m_Size     = glm::vec2(0);
    FileBuffer                m_Buffer;

  public:
    Texture(const fs::path& filename);
    virtual ~Texture();

    bool LoadFromBuffer(const FileBuffer& buffer);
    bool LoadFromFile(const fs::path& filename);
    void Use(uint32_t slot);

    bool IsLoaded() const
    {
        return (m_Texture != nullptr && m_SRV != nullptr);
    }
    const fs::path& GetPath()
    {
        return m_Filename;
    }

    ID3D11Resource* GetResource()
    {
        return m_Texture;
    }
    ID3D11ShaderResourceView* GetSRV()
    {
        return m_SRV;
    }

    const glm::vec2& GetSize() const
    {
        return m_Size;
    }
    const FileBuffer& GetBuffer()
    {
        return m_Buffer;
    }
};

class TextureManager : public Singleton<TextureManager>
{
  private:
    std::unordered_map<uint32_t, std::shared_ptr<Texture>> m_Textures;
    std::vector<uint32_t>                                  m_LastUsedTextures;
    std::unique_ptr<Texture>                               m_MissingTexture;
    std::vector<std::shared_ptr<Texture>>                  m_PreviewTextures;

  public:
    enum TextureCreateFlags {
        NO_CREATE            = (1u << 0),
        CREATE_IF_NOT_EXISTS = (1u << 1),
        IS_UI_RENDERABLE     = (1u << 2),
    };

    TextureManager();
    virtual ~TextureManager() = default;

    void Shutdown();

    std::shared_ptr<Texture> GetTexture(const fs::path& filename, uint8_t flags = CREATE_IF_NOT_EXISTS);
    std::shared_ptr<Texture> GetTexture(const fs::path& filename, FileBuffer* buffer,
                                        uint8_t flags = CREATE_IF_NOT_EXISTS);

    bool HasTexture(const fs::path& filename);

    void Flush();
    void Delete(std::shared_ptr<Texture> texture);
    void Empty();

    void PreviewTexture(std::shared_ptr<Texture> texture);

    static DDS_PIXELFORMAT GetPixelFormat(DXGI_FORMAT format);

    const std::unordered_map<uint32_t, std::shared_ptr<Texture>>& GetCache()
    {
        return m_Textures;
    }
    std::size_t GetCacheSize()
    {
        return m_Textures.size();
    }

    Texture* GetMissingTexture()
    {
        return m_MissingTexture.get();
    }
};
