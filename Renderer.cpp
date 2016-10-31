#include "Renderer.h"
#include "engine/RBMLoader.h"

Renderer::Renderer(QWidget* parent) : QOpenGLWidget(parent)
{
    m_ModelLoader = new RBMLoader;

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(50);
}

Renderer::~Renderer()
{
    delete m_ModelLoader;
}

void Renderer::initializeGL()
{
    initializeOpenGLFunctions();
    CreateShaders();

    glClearColor(0.2, 0.2, 0.2, 1.0);
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_LIGHT0);
    //glEnable(GL_LIGHTING);
    //glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    //glEnable(GL_COLOR_MATERIAL);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_CULL_FACE);

    m_ModelLoader->OpenFile("D:/Steam/steamapps/common/Just Cause 3/jc3mp/models/jc_design_tools/racing_arrows/general_red_outter_body_lod1.rbm");
}

void DrawGround(void)
{
    const GLfloat fExtent = 100.0f;
    const GLfloat fStep = 0.5f;
    GLfloat y = -0.4f;
    GLfloat iLine;

    glLineWidth(1);
    glColor3f(0.5, 0.5, 0.5);
    glBegin(GL_LINES);

    for (iLine = -fExtent; iLine <= fExtent; iLine += fStep)
    {
        glVertex3f(iLine, y, fExtent);    // Draw Z lines
        glVertex3f(iLine, y, -fExtent);

        glVertex3f(fExtent, y, iLine); // Draw X lines
        glVertex3f(-fExtent, y, iLine);
    }

    glEnd();
}

static float angle = 0.0f;
static int frame = 0;
static VertexBuffer vertex_buffer;
void Renderer::paintGL()
{
    QPainter painter;
    painter.begin(this);

    painter.beginNativePainting();

    glViewport(0, 0, 1024, 768);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_Shader->bind();

    // render triangle
    {
        QMatrix4x4 matrix;
        matrix.perspective(65.0f, 1.333f, 0.1f, 100.0f);
        //matrix.rotate(angle, 0, 1, 0);
        matrix.translate(0, 0, -5);

        m_Shader->setUniformValue(m_MatrixUniform, matrix);

        if (!vertex_buffer.IsCreated()) {
            QVector<GLfloat> vertices { 0, 1, 0, -1, -1, 0, 1, -1, 0 };
            vertex_buffer.Create(vertices);
        }

        m_Shader->enableAttributeArray(m_VertexAttribute);

        vertex_buffer.m_Buffer.bind();
        m_Shader->setAttributeBuffer(m_VertexAttribute, GL_FLOAT, 0, 3);
        vertex_buffer.m_Buffer.release();

        glDrawArrays(GL_TRIANGLES, 0, 3);

        m_Shader->disableAttributeArray(m_VertexAttribute);
    }


    // testing
    {
        auto renderBlock = m_ModelLoader->GetCurrentRenderBlock();
        if (renderBlock)
        {
            auto buffer = renderBlock->GetVertexBuffer();
            if (buffer && buffer->IsCreated())
            {
                QMatrix4x4 matrix;
                matrix.perspective(65.0f, 1.333f, 0.1f, 100.0f);
                matrix.translate(-3, 0, -5);

                m_Shader->setUniformValue(m_MatrixUniform, matrix);


                m_Shader->enableAttributeArray(m_VertexAttribute);

                buffer->m_Buffer.bind();
                m_Shader->setAttributeBuffer(m_VertexAttribute, GL_FLOAT, 0, 3);
                buffer->m_Buffer.release();

                glDrawArrays(GL_TRIANGLES, 0, 3);

                m_Shader->disableAttributeArray(m_VertexAttribute);
            }
        }
    }

    m_Shader->release();

    painter.endNativePainting();
    painter.end();

    angle += 1.0f;
    frame++;

    update();
}

void Renderer::resizeGL(int w, int h)
{
}

void Renderer::CreateShaders()
{
    m_Shader = new QOpenGLShaderProgram(this);

    m_VertexShader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    Q_ASSERT(m_VertexShader->compileSourceFile("I:/Qt projects/jc3-rbmdl-viewer/shaders/vertexshader.glsl"));

    m_FragmentShader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    Q_ASSERT(m_FragmentShader->compileSourceFile("I:/Qt projects/jc3-rbmdl-viewer/shaders/fragmentshader.glsl"));

    m_Shader->addShader(m_VertexShader);
    m_Shader->addShader(m_FragmentShader);
    m_Shader->link();

    m_MatrixUniform = m_Shader->uniformLocation("m_ModelViewProjection");
    m_VertexAttribute = m_Shader->attributeLocation("vertex");
    //m_ColorAttribute = m_Shader->attributeLocation("colour");
}
