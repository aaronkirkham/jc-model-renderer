#include <Renderer.h>
#include <engine/RBMLoader.h>

Renderer::Renderer(QWidget* parent) : QOpenGLWidget(parent)
{
    m_ModelLoader = new RBMLoader;
}

Renderer::~Renderer()
{
    delete m_FragmentShader;
    delete m_VertexShader;
    delete m_Shader;
    delete m_ModelLoader;
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

    //m_ModelLoader->OpenFile("D:/Steam/steamapps/common/Just Cause 3/jc3mp/models/jc_design_tools/racing_arrows/general_red_outter_body_lod1.rbm");
    m_ModelLoader->OpenFile("D:/Steam/steamapps/common/Just Cause 3/jc3mp/models/jc_design_tools/racing_arrows/races_teal_arrow_body_lod1.rbm");
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
        auto renderBlock = m_ModelLoader->GetCurrentRenderBlock();
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

    update();
}

void Renderer::resizeGL(int w, int h)
{
    const qreal zNear = 0.1, zFar = 100.0, fov = 65.0;
    qreal aspect = qreal(w) / qreal(h ? h : 1);

    glViewport(0, 0, w, h);

    m_Projection.setToIdentity();
    m_Projection.perspective(fov, aspect, zNear, zFar);
}

void Renderer::mousePressEvent(QMouseEvent* event)
{
    m_LastMousePosition = event->pos();
}

void Renderer::mouseMoveEvent(QMouseEvent* event)
{
    auto pos = event->pos();
    m_Rotation.setX(m_Rotation.x() + (pos.x() - m_LastMousePosition.x()) * 0.5f);
    m_Rotation.setY(m_Rotation.y() + (pos.y() - m_LastMousePosition.y()) * 0.5f);

    m_LastMousePosition = event->pos();
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
