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
    Q_ASSERT(m_VertexShader->compileSourceFile(":/shaders/vertexshader.glsl"));

    m_FragmentShader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    Q_ASSERT(m_FragmentShader->compileSourceFile(":/shaders/fragmentshader.glsl"));

    m_Shader->addShader(m_VertexShader);
    m_Shader->addShader(m_FragmentShader);
    m_Shader->link();

    m_MatrixUniform = m_Shader->uniformLocation("mvp_matrix");
    m_VertexLocation = m_Shader->attributeLocation("a_position");
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
    m_Rotation = QVector2D();
    m_IsRotatingModel = false;
    SetFieldOfView(FIELD_OF_VIEW);
}

void Renderer::initializeGL()
{
    initializeOpenGLFunctions();
    CreateShaders();

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_LIGHT0);
    //glEnable(GL_LIGHTING);
    //glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    //glEnable(GL_COLOR_MATERIAL);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glEnable(GL_CULL_FACE);
}

void Renderer::paintGL()
{
    QPainter painter;
    painter.begin(this);
    painter.beginNativePainting();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int32_t triangles = 0, vertices = 0, indices = 0;

    m_Shader->bind();

    // render the current model
    {
        auto renderBlock = RBMLoader::instance()->GetCurrentRenderBlock();
        if (renderBlock)
        {
            auto buffer = renderBlock->GetBuffer();
            if (buffer)
            {
                if (!buffer->IsCreated())
                    buffer->Create(this);

                QMatrix4x4 matrix;
                matrix.translate(0, 0, -10);
                matrix.rotate(m_Rotation.x(), 0, 1, 0);
                matrix.rotate(m_Rotation.y(), 1, 0, 0);

                m_Shader->setUniformValue(m_MatrixUniform, m_Projection * matrix);

                buffer->m_VAO.bind();
                glDrawElements(GL_TRIANGLES, buffer->m_Indices.size(), GL_UNSIGNED_SHORT, 0);
                buffer->m_VAO.release();

                vertices += buffer->m_Vertices.size();
                triangles += (vertices / 3);
                indices += buffer->m_Indices.size();
            }
        }
    }

    m_Shader->release();

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
    if (m_IsRotatingModel)
    {
        auto pos = event->pos();
        m_Rotation.setX(m_Rotation.x() + (pos.x() - m_LastMousePosition.x()) * 0.5f);
        m_Rotation.setY(m_Rotation.y() + (pos.y() - m_LastMousePosition.y()) * 0.5f);

        m_LastMousePosition = pos;

        update();

        MainWindow::instance()->GetInterafce();
    }
}

void Renderer::wheelEvent(QWheelEvent* event)
{
    float fov = qBound(0.0f, (m_Fov + -event->delta() * 0.05f), 168.0f);
    SetFieldOfView(fov);
}
