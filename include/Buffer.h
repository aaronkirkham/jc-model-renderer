#ifndef BUFFER_H
#define BUFFER_H

#include <MainWindow.h>

QT_FORWARD_DECLARE_CLASS(Renderer)

class Buffer
{
public:
    QOpenGLVertexArrayObject m_VAO;
    QOpenGLBuffer* m_VertexBuffer = nullptr;
    QOpenGLBuffer* m_IndexBuffer = nullptr;
    QVector<GLfloat> m_Vertices;
    QVector<uint16_t> m_Indices;
    bool m_IsCreated = false;

    ~Buffer()
    {
        Destroy();
    }

    void Create(Renderer* renderer);

    void Destroy()
    {
        if (IsCreated())
        {
            m_VAO.release();
            m_VAO.destroy();

            // destroy vertex buffer
            if (m_VertexBuffer)
            {
                m_VertexBuffer->destroy();

                delete m_VertexBuffer;
                m_VertexBuffer = nullptr;
            }

            // destroy index buffer
            if (m_IndexBuffer)
            {
                m_IndexBuffer->destroy();

                delete m_IndexBuffer;
                m_IndexBuffer = nullptr;
            }

            m_Vertices.clear();
            m_Indices.clear();

            m_IsCreated = false;
        }
    }

    bool IsCreated()
    {
        return m_IsCreated;
    }
};

#endif // BUFFER_H
