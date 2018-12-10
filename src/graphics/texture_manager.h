#pragma once

#include <unordered_map>

#include "../singleton.h"
#include "texture.h"

struct DDS_PIXELFORMAT;

class TextureManager : public Singleton<TextureManager>
{
  private:
    std::unordered_map<uint32_t, std::shared_ptr<Texture>> m_Textures;
    std::vector<uint32_t>                                  m_LastUsedTextures;
    std::vector<std::shared_ptr<Texture>>                  m_PreviewTextures;

  public:
    enum TextureCreateFlags {
        TextureCreateFlags_NoCreate          = (1 << 0),
        TextureCreateFlags_CreateIfNotExists = (1 << 1),
        TextureCreateFlags_IsUIRenderable    = (1 << 2),
    };

    TextureManager();
    virtual ~TextureManager() = default;

    void Shutdown();

    std::shared_ptr<Texture> GetTexture(const std::filesystem::path& filename,
                                        uint8_t                      flags = TextureCreateFlags_CreateIfNotExists);
    std::shared_ptr<Texture> GetTexture(const std::filesystem::path& filename, FileBuffer* buffer,
                                        uint8_t flags = TextureCreateFlags_CreateIfNotExists);

    bool HasTexture(const std::filesystem::path& filename);

    void Flush();
    void Delete(std::shared_ptr<Texture> texture);
    void Empty();

    void PreviewTexture(std::shared_ptr<Texture> texture);

    static DDS_PIXELFORMAT GetPixelFormat(DXGI_FORMAT format);

    const std::unordered_map<uint32_t, std::shared_ptr<Texture>>& GetCache()
    {
        return m_Textures;
    }

    std::size_t GetCacheSize() const
    {
        return m_Textures.size();
    }
};
