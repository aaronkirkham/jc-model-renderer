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
#include <QMouseEvent>
#include <QMessageBox>
#include <QDebug>

#include <gl/GLU.h>
#include <gl/GL.h>

#include "Singleton.h"
#include "Renderer.h"
#include "engine/Buffer.h"
#include "engine/RBMLoader.h"
#include "engine/RenderBlockFactory.h"
#include "engine/renderblocks/IRenderBlock.h"
#include "engine/renderblocks/RenderBlockGeneralJC3.h"

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

private slots:
    void SelectModelFile();

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    Ui::MainWindow* GetInterafce() { return m_Interface; }
};

#endif // MAINWINDOW_H
