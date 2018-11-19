#pragma once

#include <StdInc.h>
#include <graphics/Texture.h>
#include <singleton.h>
#include <thread>

struct DDS_PIXELFORMAT;
class TextureManager : public Singleton<TextureManager>
{
  private:
    std::unordered_map<uint32_t, std::shared_ptr<Texture>> m_Textures;
    std::vector<uint32_t>                                  m_LastUsedTextures;
    std::vector<std::shared_ptr<Texture>>                  m_PreviewTextures;

  public:
    enum TextureCreateFlags {
        NO_CREATE            = 0x1,
        CREATE_IF_NOT_EXISTS = 0x2,
        IS_UI_RENDERABLE     = 0x4,
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

    std::size_t GetCacheSize() const
    {
        return m_Textures.size();
    }
};
