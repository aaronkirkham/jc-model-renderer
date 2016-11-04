#ifndef MATERIALS_H
#define MATERIALS_H

#include <MainWindow.h>
#include <QOpenGLWidget>
#include <QGLWidget>

class Materials
{
private:
    QVector<QString> m_Materials;
    QVector<QOpenGLTexture*> m_Textures;
    bool m_IsCreated = false;

    void LoadTextureToMemory(const QString& filename);

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

        m_Materials.clear();
    }

    void Read(QDataStream& data);

    // Must be called in paint context
    void Create()
    {
        for (auto &material : m_Materials)
        {
            auto image = QGLWidget::convertToGLFormat(QImage(material));
            auto texture = new QOpenGLTexture(image.mirrored().rgbSwapped());
            m_Textures.push_back(texture);
        }

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
