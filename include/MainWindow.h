#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Renderer.h"
#include "engine/RenderBlockFactory.h"
#include "engine/RBMLoader.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::MainWindow* ui = nullptr;
    Renderer* m_Renderer = nullptr;

public slots:
    void OnClickLoadFile();

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
};

#endif // MAINWINDOW_H
