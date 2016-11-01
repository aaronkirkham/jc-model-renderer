#include <MainWindow.h>
#include <QApplication>

RBMLoader::RBMLoader()
{
    m_File = new QFile;
    m_RenderBlockFactory = new RenderBlockFactory;
}

RBMLoader::~RBMLoader()
{
    delete m_RenderBlockFactory;

    if (m_File)
    {
        m_File->close();
        delete m_File;
    }
}

void RBMLoader::OpenFile(const QString& filename)
{
    // cleanup if we already have an open file
    if (m_File)
        m_File->close();

    m_File->setFileName(filename);
    if (!m_File->open(QIODevice::ReadOnly))
        return;

    SetReadOffset(0);

    // Read the header and initial block information
    RenderBlockModel rbm;
    Read(rbm);

    qDebug() << QString("RBMDL v%1.%2.%3").arg(QString::number(rbm.versionMajor), QString::number(rbm.versionMinor), QString::number(rbm.versionRevision));
    qDebug() << "Number of blocks:" << rbm.numberOfBlocks;

    // Read all the render blocks
    for (uint32_t i = 0; i < rbm.numberOfBlocks; i++)
    {
        uint32_t typeHash;
        Read(typeHash);

        qDebug() << QString("Reading block 0x%1").arg(QString::number(typeHash, 16).toUpper());

        // read the render block if we found the type
        if (m_CurrentRenderBlock = m_RenderBlockFactory->GetRenderBlock(typeHash))
            m_CurrentRenderBlock->Read(this);
        else
            QMessageBox::critical(MainWindow::instance(), "RBM Renderer", "Unsupported Render Block type.");
    }
}
