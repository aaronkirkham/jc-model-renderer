#include <MainWindow.h>

// TODO: Use the correct context so we don't have to call this in the paint thread
void Buffer::Create(uint8_t texCoordStride)
{
    if (!m_IsCreated)
    {
        auto renderer = MainWindow::instance()->GetRenderer();

        m_VAO.create();
        m_VAO.bind();

        // bind vertices
        {
            if (m_Vertices.size() > 0)
            {
                m_VertexBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
                auto result = m_VertexBuffer->create();
                Q_ASSERT(result);
                m_VertexBuffer->setUsagePattern(QOpenGLBuffer::StaticDraw);
                m_VertexBuffer->bind();
                m_VertexBuffer->allocate(m_Vertices.constData(), m_Vertices.size() * sizeof(GLfloat));

                renderer->glEnableVertexAttribArray(renderer->GetVertexLocation());
                renderer->glVertexAttribPointer(renderer->GetVertexLocation(), 3, GL_FLOAT, GL_FALSE, 0, 0);
            }
        }

        // bind texcoords
        {
            if (m_TexCoords.size() > 0)
            {
                m_TextureBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
                auto result = m_TextureBuffer->create();
                Q_ASSERT(result);
                m_TextureBuffer->setUsagePattern(QOpenGLBuffer::StaticDraw);
                m_TextureBuffer->bind();
                m_TextureBuffer->allocate(m_TexCoords.constData(), m_TexCoords.size() * sizeof(GLfloat));

                renderer->glEnableVertexAttribArray(renderer->GetTexCoordLocation());
                renderer->glVertexAttribPointer(renderer->GetTexCoordLocation(), texCoordStride, GL_FLOAT, GL_FALSE, 0, 0);
            }
        }

        // bind indices
        {
            if (m_Indices.size() > 0)
            {
                m_IndexBuffer = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
                auto result = m_IndexBuffer->create();
                Q_ASSERT(result);
                m_IndexBuffer->setUsagePattern(QOpenGLBuffer::StaticDraw);
                m_IndexBuffer->bind();
                m_IndexBuffer->allocate(m_Indices.constData(), m_Indices.size() * sizeof(uint16_t));
            }
        }

        m_VAO.release();

        if (m_VertexBuffer)
            m_VertexBuffer->release();

        if (m_TextureBuffer)
            m_TextureBuffer->release();

        if (m_IndexBuffer)
            m_IndexBuffer->release();

        m_IsCreated = true;
    }
}
