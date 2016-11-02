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
        MainWindow::instance()->GetRenderer()->Reset();

        RenderBlockModel rbm;
        Read(data, rbm);

        QString fileType (QByteArray((char *)&rbm.magic, 5));
        if (fileType == "RBMDL")
        {
            qDebug() << QString("RBMDL v%1.%2.%3").arg(QString::number(rbm.versionMajor), QString::number(rbm.versionMinor), QString::number(rbm.versionRevision));
            qDebug() << "Number of blocks:" << rbm.numberOfBlocks;

            // Read all the render blocks
            for (uint32_t i = 0; i < rbm.numberOfBlocks; i++)
            {
                uint32_t typeHash;
                Read(data, typeHash);

                qDebug() << QString("Reading block 0x%1").arg(QString::number(typeHash, 16).toUpper());

                // read the current render block
                auto renderBlock = m_RenderBlockFactory->GetRenderBlock(typeHash);
                if (renderBlock)
                {
                    renderBlock->Read(data);

                    uint32_t checksum;
                    Read(data, checksum);

                    // if we didn't read the file correctly, get out now
                    if (checksum != 0x89ABCDEF)
                    {
                        qDebug() << "ERROR: Read checksum mismatch!";

                        renderBlock->Reset();
                        break;
                    }

                    m_RenderBlocks.push_back(renderBlock);
                }
                else
                {
                    qDebug() << "Unsupported render block type.";
                    break;
                }
            }
        }
        else
        {
            qDebug() << "Unknown file format.";
        }
    });
}
