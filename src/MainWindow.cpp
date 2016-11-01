#include <MainWindow.h>
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), m_Interface(new Ui::MainWindow)
{
    m_Interface->setupUi(this);

    connect(m_Interface->actionOpenFile, SIGNAL(triggered(bool)), this, SLOT(SelectModelFile()));
    connect(m_Interface->actionExit, SIGNAL(triggered(bool)), qApp, SLOT(quit()));

    m_Renderer = new Renderer(this);
    m_Renderer->setGeometry(0, 20, 1024, 768);
}

MainWindow::~MainWindow()
{
    delete m_Renderer;
    delete m_Interface;
}

void MainWindow::SelectModelFile()
{
    // E:/jc3-unpack
    auto file = QFileDialog::getOpenFileName(this, "Select RBM File", "E:/jc3-unpack/locations/rom/region_01/air_races/ar_r1_02_unpack/models/jc_design_tools/racing_arrows", "*.rbm");
    qDebug() << "Selected file" << file;

    RBMLoader::instance()->OpenFile(file);

    // TODO: Save the last opened directory so we can just re-select that folder after.
}
