#ifndef RENDERER_H
#define RENDERER_H

#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>

#include <gl/GLU.h>
#include <gl/GL.h>

#include "engine/RBMLoader.h"

class Renderer : public QOpenGLWidget, protected QOpenGLFunctions
{
private:
    RBMLoader* m_ModelLoader = nullptr;

    QOpenGLShaderProgram* m_Shader = nullptr;
    QOpenGLShader* m_VertexShader = nullptr;
    QOpenGLShader* m_FragmentShader = nullptr;

    int m_MatrixUniform;
    int m_VertexAttribute;
    int m_ColorAttribute;

    void CreateShaders();

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;

public:
    explicit Renderer(QWidget* parent = nullptr);
    ~Renderer();
};

#endif // RENDERER_H
