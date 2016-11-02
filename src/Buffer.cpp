#include <MainWindow.h>

void Buffer::Create(Renderer* renderer)
{
    m_VAO.create();
    m_VAO.bind();

    m_VertexBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    Q_ASSERT(m_VertexBuffer->create());
    m_VertexBuffer->setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_VertexBuffer->bind();
    m_VertexBuffer->allocate(m_Vertices.constData(), m_Vertices.size() * sizeof(GLfloat));

    renderer->glEnableVertexAttribArray(0);
    renderer->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    m_IndexBuffer = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    Q_ASSERT(m_IndexBuffer->create());
    m_IndexBuffer->setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_IndexBuffer->bind();
    m_IndexBuffer->allocate(m_Indices.constData(), m_Indices.size() * sizeof(uint16_t));

    m_VAO.release();

    m_IsCreated = true;
}
