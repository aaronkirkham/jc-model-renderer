#include "FileReader.h"

FileReader::~FileReader()
{
    m_WorkerThread.quit();
    m_WorkerThread.wait();
}

void FileReader::RequestFile(const QString& filename, FileReaderCallback callback)
{
    auto request = new FileRequest(filename, callback);
    request->moveToThread(&m_WorkerThread);

    connect(&m_WorkerThread, &QThread::started, request, &FileRequest::work);
    connect(&m_WorkerThread, &QThread::finished, request, &FileRequest::deleteLater);
    connect(request, &FileRequest::finished, &m_WorkerThread, &QThread::quit);
    connect(request, &FileRequest::finished, request, &FileRequest::deleteLater);

    m_WorkerThread.start();
}

FileRequest::FileRequest(const QString& filename, FileReaderCallback callback)
{
    m_File = new QFile(filename);
    m_Callback = callback;
}

FileRequest::~FileRequest()
{
    if (m_File->isOpen())
        m_File->close();

    delete m_File;
}

void FileRequest::work()
{
    if (m_Callback && m_File->open(QIODevice::ReadOnly))
        m_Callback(QDataStream(&m_File->readAll(), QIODevice::ReadOnly));

    // todo: error handling if the file fails to open
    emit finished();
}
