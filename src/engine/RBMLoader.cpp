#include <MainWindow.h>
#include <QApplication>

RBMLoader::RBMLoader()
{
    m_RenderBlockFactory = new RenderBlockFactory;
}

RBMLoader::~RBMLoader()
{
    delete m_RenderBlockFactory;
}

void RBMLoader::ReadFile(const QString& filename)
{
    FileReader::instance()->RequestFile(filename, [=](QDataStream& data) {
        RenderBlockModel rbm;
        Read(data, rbm);

        qDebug() << QString("RBMDL v%1.%2.%3").arg(QString::number(rbm.versionMajor), QString::number(rbm.versionMinor), QString::number(rbm.versionRevision));
        qDebug() << "Number of blocks:" << rbm.numberOfBlocks;

        // Read all the render blocks
        for (uint32_t i = 0; i < rbm.numberOfBlocks; i++)
        {
            uint32_t typeHash;
            Read(data, typeHash);

            qDebug() << QString("Reading block 0x%1").arg(QString::number(typeHash, 16).toUpper());

            // read the render block if we found the type
            if (m_CurrentRenderBlock = m_RenderBlockFactory->GetRenderBlock(typeHash))
                m_CurrentRenderBlock->Read(data);
            else
                QMessageBox::critical(MainWindow::instance(), "RBM Renderer", "Unsupported Render Block type.");
        }
    });
}
