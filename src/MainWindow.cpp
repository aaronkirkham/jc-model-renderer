#include <MainWindow.h>

#include <QMimeData>

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
    // TODO: Get the path to the game folder.
    if (m_LastOpenedDirectory.isEmpty())
        //m_LastOpenedDirectory = "E:/jc3-unpack/editor/entities/gameobjects/main_character_unpack/models/jc_characters/main_characters/rico";
        m_LastOpenedDirectory = "E:/jc3-unpack/locations/rom/region_01/air_races/ar_r1_02_unpack/models/jc_design_tools/racing_arrows";

    auto file = QFileDialog::getOpenFileName(this, "Select RBM File", m_LastOpenedDirectory, "*.rbm");

    auto fileInfo = QFileInfo(file);
    m_LastOpenedDirectory = fileInfo.absolutePath();

    setWindowTitle("Just Cause 3 RBM Renderer - " + fileInfo.fileName());

    RBMLoader::instance()->ReadFile(file);
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls())
    {
        QStringList pathList;
        QList<QUrl> urlList = mimeData->urls();

        if(urlList.empty()) {
            return;
        }

        RBMLoader::instance()->ReadFile(urlList.at(0).toLocalFile());
        event->acceptProposedAction();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    event->acceptProposedAction();
}
