#include <MainWindow.h>

#include <QMimeData>

#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), m_Interface(new Ui::MainWindow)
{
    m_Interface->setupUi(this);

    connect(m_Interface->actionOpenFile, &QAction::triggered, this, &MainWindow::SelectModelFile);
    connect(m_Interface->actionExit, &QAction::triggered, qApp, &QApplication::quit);

    connect(m_Interface->wireframeCheckBox, &QCheckBox::released, this, &MainWindow::ChangeRendererSettings);
    connect(m_Interface->showNormalsCheckBox, &QCheckBox::released, this, &MainWindow::ChangeRendererSettings);
    connect(m_Interface->disableTexturesCheckBox, &QCheckBox::released, this, &MainWindow::ChangeRendererSettings);

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
    if (!file.isEmpty())
    {
        auto fileInfo = QFileInfo(file);
        m_LastOpenedDirectory = fileInfo.absolutePath();
        m_CurrentOpenFile = fileInfo.absoluteFilePath();

        setWindowTitle("Just Cause 3 RBM Renderer - " + fileInfo.fileName());

        RBMLoader::instance()->ReadFile(file);
    }
}

void MainWindow::ChangeRendererSettings()
{
    auto widget = (QCheckBox *)sender();
    RendererFlag flag;

    if (widget == m_Interface->wireframeCheckBox)
        flag = RendererFlag::ENABLE_WIREFRAME;
    else if (widget == m_Interface->showNormalsCheckBox)
        flag = RendererFlag::ENABLE_NORMALS;
    else if (widget == m_Interface->disableTexturesCheckBox)
        flag = RendererFlag::DISABLE_TEXTURES;

    m_Renderer->SetFlag(flag, widget->isChecked());
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

        auto fileInfo = QFileInfo(urlList.at(0).toLocalFile());
        setWindowTitle("Just Cause 3 RBM Renderer - " + fileInfo.fileName());
        m_CurrentOpenFile = fileInfo.absoluteFilePath();

        RBMLoader::instance()->ReadFile(fileInfo.filePath());
        event->acceptProposedAction();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    event->acceptProposedAction();
}
