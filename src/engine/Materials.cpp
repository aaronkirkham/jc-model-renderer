#include <MainWindow.h>
#include <QBuffer>
#include <dds/ddsheader.h>

void Materials::Read(QDataStream& data)
{
    uint32_t materialCount;
    RBMLoader::instance()->Read(data, materialCount);

    for (uint32_t i = 0; i < materialCount; i++)
    {
        QString material;
        RBMLoader::instance()->Read(data, material);

        // uses a shared texture
        // TODO: Load shared textures
        if (material.startsWith("textures/"))
            continue;

        auto currentFile = MainWindow::instance()->GetCurrentOpenFile();
        auto modelsIndex = currentFile.indexOf("models/");
        if (modelsIndex != -1)
        {
            auto materialPath = currentFile.mid(0, modelsIndex).append(material);
            if (materialPath.endsWith("ddsc"))
            {
                m_Materials.push_back(materialPath);

                auto texture = TextureCache::instance()->GetTexture(materialPath);

                if (texture)
                    m_PreLoadedTextures.push_back(texture);
            }
        }
    }

    // unknown stuff
    uint32_t unk[4];
    RBMLoader::instance()->Read(data, unk);
}
