#include <MainWindow.h>

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
                LoadTextureToMemory(materialPath);
        }

#if 0
        // temp, testing the actual texture rendering.
        if (i == 0) {
            material = "E:/jc3-unpack/locations/rom/region_01/air_races/ar_r1_02_unpack/models/jc_design_tools/racing_arrows/textures/racing_arrows_dif0.dds";

            m_Materials.push_back(material);
        }
#endif
    }

    // unknown stuff
    uint32_t unk[4];
    RBMLoader::instance()->Read(data, unk);
}

void Materials::LoadTextureToMemory(const QString& filename)
{
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly))
    {
        QDataStream data(&file.readAll(), QIODevice::ReadOnly);
        file.close();

        // TODO: Get dds data from ddsc file
    }
    else
        qDebug() << "ERROR: Failed to load file" << filename;
}
