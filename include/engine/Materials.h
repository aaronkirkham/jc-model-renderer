#ifndef MATERIALS_H
#define MATERIALS_H

#include <MainWindow.h>
#include <QOpenGLWidget>

class Materials
{
private:
    QVector<QString> m_Materials;
    //QVector<QOpenGLTexture*> m_Textures;

public:
    Materials() = default;
    virtual ~Materials() = default;

    void Reset()
    {
        /*for (auto &texture : m_Textures)
        {
            texture->destroy();
            delete texture;
        }*/

        m_Materials.clear();
    }

    void Read(QDataStream& data)
    {
        uint32_t materialCount;
        RBMLoader::instance()->Read(data, materialCount);

        for (uint32_t i = 0; i < materialCount; i++)
        {
            QString material;
            RBMLoader::instance()->Read(data, material);

            qDebug() << material;

            m_Materials.push_back(material);

            //auto image = QGLWidget::convertToGLFormat(QImage(material));
            //auto texture = new QOpenGLTexture(image);
            //m_Textures.push_back(texture);
        }

        //qDebug() << m_Textures;

        // unknown stuff
        uint32_t unk[4];
        RBMLoader::instance()->Read(data, unk);
    }
};

#endif // MATERIALS_H
