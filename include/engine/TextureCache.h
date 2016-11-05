#ifndef TEXTURECACHE_H
#define TEXTURECACHE_H

#include <MainWindow.h>
#include <dds/ddsheader.h>

class TextureCache : public Singleton<TextureCache>
{
private:
    QMap<uint32_t, QImage> m_CachedTextures;
    QMap<uint32_t, int32_t> m_CachedTexturesRefCount;

    QImage* CacheTexture(const QString& filename);
    DDSPixelFormat GetDDCSPixelFormat(uint32_t& format);

public:
    TextureCache() = default;
    ~TextureCache();

    QImage* GetTexture(const QString& filename);
    void FreeTexture(const QString& filename);
};

#endif
