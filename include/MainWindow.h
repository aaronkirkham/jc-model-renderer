#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QFile>
#include <QFileDialog>
#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMessageBox>
#include <QThread>
#include <QDebug>

#include <functional>
#include <gl/GLU.h>
#include <gl/GL.h>

#include "Singleton.h"
#include "Renderer.h"
#include "FileReader.h"
#include "Buffer.h"
#include "engine/RBMLoader.h"
#include "engine/RenderBlockFactory.h"
#include "engine/Materials.h"
#include "engine/renderblocks/IRenderBlock.h"
#include "engine/renderblocks/RenderBlockGeneralJC3.h"
#include "engine/renderblocks/RenderBlockCharacter.h"

namespace Ui {
class MainWindow;
}

QT_FORWARD_DECLARE_CLASS(Renderer)

class MainWindow : public QMainWindow, public Singleton<MainWindow>
{
    Q_OBJECT

private:
    Ui::MainWindow* m_Interface = nullptr;
    Renderer* m_Renderer = nullptr;
    QString m_LastOpenedDirectory;

private slots:
    void SelectModelFile();

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    Ui::MainWindow* GetInterafce() { return m_Interface; }
    Renderer* GetRenderer() { return m_Renderer; }
    const QString& GetLastOpenedDirectory() { return m_LastOpenedDirectory; }
};

#endif // MAINWINDOW_H
