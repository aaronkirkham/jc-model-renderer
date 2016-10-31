#ifndef BUFFER_H
#define BUFFER_H

#include <QVector>
#include <QVector3D>
#include <QtOpenGL>
#include <QDebug>

class VertexBuffer
{
public:
    QOpenGLBuffer m_Buffer;
    uint32_t m_Count = 0;

    void Create(const QVector<GLfloat>& buffer)
    {
        m_Count = buffer.size();

        m_Buffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        m_Buffer.create();
        m_Buffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        m_Buffer.bind();
        m_Buffer.allocate(buffer.constData(), sizeof(GLfloat) * m_Count);
        m_Buffer.release();
    }

    bool IsCreated() { return m_Count != 0; }
};

class IndexBuffer
{
public:
    QOpenGLBuffer m_Buffer;
    uint32_t m_Count = 0;

    template <typename T>
    void Create(const QVector<T>& buffer)
    {
        m_Count = buffer.size();

        m_Buffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        m_Buffer.create();
        m_Buffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        m_Buffer.bind();
        m_Buffer.allocate(buffer.constData(), sizeof(T) * m_Count);
        m_Buffer.release();
    }

    bool IsCreated() { return m_Count != 0; }
};

#endif // BUFFER_H
