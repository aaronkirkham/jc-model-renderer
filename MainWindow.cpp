#include "MainWindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <qDebug>

static const char* model = "D:/Steam/steamapps/common/Just Cause 3/jc3mp/models/jc_design_tools/racing_arrows/general_red_outter_body_lod1.rbm";

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->actionOpenFile, SIGNAL(triggered(bool)), this, SLOT(OnClickLoadFile()));

    setWindowTitle("Just Cause 3 - RBM Renderer");
    setMinimumSize(1024, 788);
    setMaximumSize(1024, 788);

    m_Renderer = new Renderer(this);
    m_Renderer->setGeometry(0, 20, 1024, 768);
    m_Renderer->show();
}

MainWindow::~MainWindow()
{
    delete m_Renderer;
    delete ui;
}

void MainWindow::OnClickLoadFile()
{
    auto file = QFileDialog::getOpenFileName(this, "Select RBM File", "D:/Steam/steamapps/common/Just Cause 3/jc3mp/models/jc_design_tools/racing_arrows", "*.rbm");

    // load file
    //m_ModelLoader->OpenFile(file);
}
