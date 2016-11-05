#include <MainWindow.h>
#include <QPainter>

Renderer::Renderer(QWidget* parent) : QOpenGLWidget(parent)
{ }

Renderer::~Renderer()
{
    delete m_FragmentShader;
    delete m_VertexShader;
    delete m_Shader;
}

void Renderer::CreateShaders()
{
    m_Shader = new QOpenGLShaderProgram(this);

    m_VertexShader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    auto result = m_VertexShader->compileSourceFile(":/shaders/vertexshader.glsl");
    Q_ASSERT(result);

    m_FragmentShader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    result = m_FragmentShader->compileSourceFile(":/shaders/fragmentshader.glsl");
    Q_ASSERT(result);

    m_Shader->addShader(m_VertexShader);
    m_Shader->addShader(m_FragmentShader);
    m_Shader->link();

    m_MatrixUniform = m_Shader->uniformLocation("mvp_matrix");
    m_VertexLocation = m_Shader->attributeLocation("a_position");
    m_TexCoordLocation = m_Shader->attributeLocation("a_texcoord");
    m_TextureUniform = m_Shader->uniformLocation("texture");
}

void Renderer::SetFieldOfView(float fov)
{
    m_Fov = fov;

    m_Projection.setToIdentity();
    m_Projection.perspective(m_Fov, static_cast<float>(m_Resolution.width() / m_Resolution.height()), NEAR_PLANE, FAR_PLANE);

    update();
}

void Renderer::Reset()
{
    m_Position = QVector3D(0, 0, -10);
    m_Rotation = QVector2D();
    m_IsRotatingModel = false;
    SetFieldOfView(FIELD_OF_VIEW);
}

void Renderer::SetFlag(RendererFlag flag, bool toggle)
{
    m_Flags.setFlag(flag, toggle);
    update();
}

void Renderer::initializeGL()
{
    initializeOpenGLFunctions();
    CreateShaders();

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void Renderer::paintGL()
{
    QPainter painter;
    painter.begin(this);
    painter.beginNativePainting();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int32_t triangles = 0, vertices = 0, indices = 0;

    if (m_Flags & RendererFlag::ENABLE_WIREFRAME)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    m_Shader->bind();

    // render the current model
    {
        std::lock_guard<std::recursive_mutex> _lk { RBMLoader::instance()->m_RenderBlockMutex };

        for (auto &renderBlock : RBMLoader::instance()->GetRenderBlocks())
        {
            auto buffer = renderBlock->GetBuffer();
            auto materials = renderBlock->GetMaterials();
            if (buffer)
            {
                if (!buffer->IsCreated())
                    buffer->Create(renderBlock->GetTexCoordSize());

                // TODO: Once we have textures loading correctly, we need to also wait for all the textures to be decompressed
                // and be ready before we create the textures.
                if (!materials->IsCreated())
                    materials->Create();

                QMatrix4x4 matrix;
                matrix.translate(m_Position);
                matrix.rotate(m_Rotation.x(), 0, 1, 0);
                matrix.rotate(m_Rotation.y(), 1, 0, 0);

                m_Shader->setUniformValue(m_MatrixUniform, m_Projection * matrix);

                buffer->m_VAO.bind();

                if (!(m_Flags & RendererFlag::DISABLE_TEXTURES))
                    materials->bind();

                glDrawElements(GL_TRIANGLES, buffer->m_Indices.size(), GL_UNSIGNED_SHORT, 0);

                if (!(m_Flags & RendererFlag::DISABLE_TEXTURES))
                    materials->release();

                buffer->m_VAO.release();

                vertices += buffer->m_Vertices.size();
                indices += buffer->m_Indices.size();
                triangles += (indices / 3);
            }
        }
    }

    m_Shader->release();

    // turn off wireframes after rendering if they're enabled
    if (m_Flags & RendererFlag::ENABLE_WIREFRAME)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    painter.endNativePainting();

    QFont font = painter.font();
    font.setPixelSize(14);
    painter.setFont(font);

    painter.setPen(Qt::white);
    painter.drawText(10, 758, QString("Drawing %1 triangles (%2 vertices, %3 indices)").arg(QString::number(triangles), QString::number(vertices), QString::number(indices)));

    painter.end();
}

void Renderer::resizeGL(int w, int h)
{
    m_Resolution.setWidth(w);
    m_Resolution.setHeight(h);

    float aspect = static_cast<float>(w / h);

    glViewport(0, 0, w, h);

    m_Projection.setToIdentity();
    m_Projection.perspective(m_Fov, aspect, NEAR_PLANE, FAR_PLANE);
}

void Renderer::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_LastMousePosition = event->pos();
        m_IsRotatingModel = true;
    }
    else if (event->button() == Qt::MiddleButton)
    {
        Reset();
    }
}

void Renderer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_IsRotatingModel = false;
    }
}

void Renderer::mouseMoveEvent(QMouseEvent* event)
{
    if (m_IsRotatingModel && event->modifiers() != Qt::AltModifier)
    {
        auto pos = event->pos();
        m_Rotation.setX(m_Rotation.x() + (pos.x() - m_LastMousePosition.x()) * 0.5f);
        m_Rotation.setY(m_Rotation.y() + (pos.y() - m_LastMousePosition.y()) * 0.5f);

        m_LastMousePosition = pos;

        update();
    }
    else if(m_IsRotatingModel && event->modifiers() == Qt::AltModifier)
    {
        auto pos = event->pos();
        m_Position.setX(m_Position.x() + (pos.x() - m_LastMousePosition.x()) * 0.005f);
        m_Position.setY(m_Position.y() - (pos.y() - m_LastMousePosition.y()) * 0.005f);

        m_LastMousePosition = pos;

        update();
    }
}

void Renderer::wheelEvent(QWheelEvent* event)
{
    float scale = event->modifiers() == Qt::ControlModifier ? 0.05f : 0.005f;
    m_Position.setZ(m_Position.z() + event->delta() * scale);
    update();
}
