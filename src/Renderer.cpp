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

    m_Shader->bind();

    // render the current model
    {
        auto renderBlock = RBMLoader::instance()->GetCurrentRenderBlock();
        if (renderBlock)
        {
            auto vertexBuffer = renderBlock->GetVertexBuffer();
            auto indexBuffer = renderBlock->GetIndexBuffer();
            if (vertexBuffer && vertexBuffer->IsCreated())
            {
                QMatrix4x4 matrix;
                matrix.translate(0, 0, -10);
                matrix.rotate(m_Rotation.x(), 0, 1, 0);
                matrix.rotate(m_Rotation.y(), 1, 0, 0);

                m_Shader->setUniformValue(m_MatrixUniform, m_Projection * matrix);

                vertexBuffer->m_Buffer.bind();
                indexBuffer->m_Buffer.bind();

                m_Shader->enableAttributeArray(m_VertexLocation);
                m_Shader->setAttributeBuffer(m_VertexLocation, GL_FLOAT, 0, 3);

                glDrawElements(GL_TRIANGLES, indexBuffer->m_Count, GL_UNSIGNED_SHORT, 0);
            }
        }
    }

    m_Shader->release();

    painter.endNativePainting();
    painter.end();
}

void Renderer::resizeGL(int w, int h)
{
    static const qreal zNear = 0.1, zFar = 100.0, fov = 65.0;
    qreal aspect = qreal(w) / qreal(h ? h : 1);

    glViewport(0, 0, w, h);

    m_Projection.setToIdentity();
    m_Projection.perspective(fov, aspect, zNear, zFar);
}

void Renderer::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_LastMousePosition = event->pos();
        m_IsRotatingModel = true;
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

void Renderer::CreateShaders()
{
    m_Shader = new QOpenGLShaderProgram(this);

    m_VertexShader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    Q_ASSERT(m_VertexShader->compileSourceFile("I:/Qt projects/jc3-rbm-renderer/resources/shaders/vertexshader.glsl"));

    m_FragmentShader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    Q_ASSERT(m_FragmentShader->compileSourceFile("I:/Qt projects/jc3-rbm-renderer/resources/shaders/fragmentshader.glsl"));

    m_Shader->addShader(m_VertexShader);
    m_Shader->addShader(m_FragmentShader);
    m_Shader->link();

    m_MatrixUniform = m_Shader->uniformLocation("mvp_matrix");
    m_VertexLocation = m_Shader->attributeLocation("a_position");
}
