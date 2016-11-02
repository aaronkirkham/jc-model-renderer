#ifndef FILEREADER_H
#define FILEREADER_H

#include <MainWindow.h>

using FileReaderCallback = std::function<void(QDataStream&)>;

class FileRequest : public QObject
{
    Q_OBJECT

private:
    FileReaderCallback m_Callback = nullptr;
    QFile* m_File = nullptr;

public slots:
    void work();

signals:
    void finished();

public:
    FileRequest(const QString& filename, FileReaderCallback callback);
    ~FileRequest();
};

class FileReader : public QObject, public Singleton<FileReader>
{
    Q_OBJECT

private:
    QThread m_WorkerThread;

public:
    FileReader() = default;
    ~FileReader();

    void RequestFile(const QString& filename, FileReaderCallback callback);
};

#endif // FILEREADER_H
