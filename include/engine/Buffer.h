#ifndef BUFFER_H
#define BUFFER_H

#include <MainWindow.h>

template <typename T>
class Buffer
{
public:
    QOpenGLBuffer m_Buffer;
    uint32_t m_BufferSize = 0;
    uint32_t m_Count = 0;
    QVector<T> m_Data;

    ~Buffer()
    {
        Destroy();
    }

    void Setup(const QVector<T>& data)
    {
        m_Data = data;
        m_Count = m_Data.size();
    }

    void Destroy()
    {
        if (IsCreated())
        {
            m_Buffer.release();
            m_Buffer.destroy();
            m_BufferSize = 0;
            m_Count = 0;
        }
    }

    bool IsCreated()
    {
        return (m_BufferSize > 0);
    }
};

class VertexBuffer : public Buffer<GLfloat>
{
public:
    // This function must be called in the open gl context
    void Create()
    {
        m_Buffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        Q_ASSERT(m_Buffer.create());
        m_Buffer.bind();
        m_Buffer.allocate(m_Data.constData(), sizeof(GLfloat) * m_Count);
        m_BufferSize = m_Buffer.size();
        m_Buffer.release();
    }
};

class IndexBuffer : public Buffer<uint16_t>
{
public:
    // This function must be called in the open gl context
    void Create()
    {
        m_Buffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        Q_ASSERT(m_Buffer.create());
        m_Buffer.bind();
        m_Buffer.allocate(m_Data.constData(), sizeof(uint16_t) * m_Count);
        m_BufferSize = m_Buffer.size();
        m_Buffer.release();
    }
};

#endif // BUFFER_H
