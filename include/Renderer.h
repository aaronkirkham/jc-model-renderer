#ifndef RENDERER_H
#define RENDERER_H

#include <MainWindow.h>

class Renderer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

private:
    QOpenGLShaderProgram* m_Shader = nullptr;
    QOpenGLShader* m_VertexShader = nullptr;
    QOpenGLShader* m_FragmentShader = nullptr;

    QMatrix4x4 m_Projection;
    QVector2D m_Rotation;
    QPoint m_LastMousePosition;
    bool m_IsRotatingModel = false;

    int m_MatrixUniform = -1;
    int m_VertexLocation = -1;

    void CreateShaders();
    bool ShouldUpdateScene() { return m_IsRotatingModel; }

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;

    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);

public:
    explicit Renderer(QWidget* parent = nullptr);
    ~Renderer();
};

#endif // RENDERER_H
