#ifndef BUFFER_H
#define BUFFER_H

#include <MainWindow.h>

QT_FORWARD_DECLARE_CLASS(Renderer)

class Buffer
{
public:
    QOpenGLVertexArrayObject m_VAO;
    QOpenGLBuffer* m_VertexBuffer = nullptr;
    QOpenGLBuffer* m_TextureBuffer = nullptr;
    QOpenGLBuffer* m_IndexBuffer = nullptr;
    QVector<GLfloat> m_Vertices;
    QVector<GLfloat> m_TexCoords;
    QVector<uint16_t> m_Indices;
    bool m_IsCreated = false;

    ~Buffer()
    {
        Destroy();
    }

    void Create();

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

            // destroy texture buffer
            if (m_TextureBuffer)
            {
                m_TextureBuffer->destroy();

                delete m_TextureBuffer;
                m_TextureBuffer = nullptr;
            }

            // destroy index buffer
            if (m_IndexBuffer)
            {
                m_IndexBuffer->destroy();

                delete m_IndexBuffer;
                m_IndexBuffer = nullptr;
            }

            m_IsCreated = false;
        }

        m_Vertices.clear();
        m_TexCoords.clear();
        m_Indices.clear();
    }

    bool IsCreated()
    {
        return m_IsCreated;
    }
};

#endif // BUFFER_H
