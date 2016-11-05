#ifndef MATERIALS_H
#define MATERIALS_H

#include <MainWindow.h>
#include <QOpenGLWidget>
#include <QGLWidget>

#pragma pack(push, 1)
struct DDSC
{
    uint32_t magic;
    uint16_t version;
    uint8_t unknown;
    uint8_t dimension;
    uint32_t format;
    uint16_t width;
    uint16_t height;
    uint16_t depth;
    uint16_t flags;
    uint8_t mipCount;
    uint8_t headerMipCount;
    uint8_t unknown2[6];
    uint32_t unknown3;
};
#pragma pack(pop)

class Materials
{
private:
    QVector<QString> m_Materials;
    QVector<QImage*> m_PreLoadedTextures;
    QVector<QOpenGLTexture*> m_Textures;
    bool m_IsCreated = false;

public:
    Materials() = default;
    virtual ~Materials() = default;

    void Reset()
    {
        // Texture is not valid in the current context.
        // Texture has not been destroyed
        // TODO: Need to call this from the opengl thread.
        for (auto &texture : m_Textures)
        {
            texture->destroy();
            delete texture;
        }

        // free the texture
        for (auto &material : m_Materials)
            TextureCache::instance()->FreeTexture(material);

        m_Materials.clear();
        m_PreLoadedTextures.clear();
        m_Textures.clear();
    }

    void Read(QDataStream& data);

    // Must be called in paint context
    void Create()
    {
        for (auto &texture : m_PreLoadedTextures)
            m_Textures.push_back(new QOpenGLTexture(*texture));

        m_IsCreated = true;
    }

    inline void bind()
    {
        for (auto &texture : m_Textures)
            texture->bind();
    }

    inline void release()
    {
        for (auto &texture : m_Textures)
            texture->release();
    }

    bool IsCreated() { return m_IsCreated; }
    const QVector<QOpenGLTexture*>& GetTextures() { return m_Textures; }
};

#endif // MATERIALS_H
