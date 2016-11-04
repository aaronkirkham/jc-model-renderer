#ifndef RENDERER_H
#define RENDERER_H

#include <MainWindow.h>

static const float FIELD_OF_VIEW = 65.0f;
static const float NEAR_PLANE = 0.1f;
static const float FAR_PLANE = 1000.0f;

class Renderer : public QOpenGLWidget, public QOpenGLFunctions
{
    Q_OBJECT

private:
    QOpenGLShaderProgram* m_Shader = nullptr;
    QOpenGLShader* m_VertexShader = nullptr;
    QOpenGLShader* m_FragmentShader = nullptr;

    float m_Fov = FIELD_OF_VIEW;
    QSize m_Resolution;

    QMatrix4x4 m_Projection;
    QVector3D m_Position;
    QVector2D m_Rotation;
    QPoint m_LastMousePosition;
    bool m_IsRotatingModel = false;

    int32_t m_MatrixUniform = -1;
    int32_t m_VertexLocation = -1;
    int32_t m_TexCoordLocation = -1;
    int32_t m_TextureUniform = -1;

    void CreateShaders();

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

public:
    explicit Renderer(QWidget* parent = nullptr);
    ~Renderer();

    void SetFieldOfView(float fov);
    void Reset();

    int32_t GetVertexLocation() { return m_VertexLocation; }
    int32_t GetTexCoordLocation() { return m_TexCoordLocation; }
};

#endif // RENDERER_H
