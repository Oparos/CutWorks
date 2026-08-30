#include "app/MainWindow.h"

#include "cad/CadModule.h"
#include "cam/CamModule.h"
#include "cnc/CncModule.h"

#include <QAction>
#include <QStackedWidget>
#include <QToolBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("CutWorks"));

    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    setupModules();
    setupNavigation();
}

void MainWindow::setupModules()
{
    m_cadModule = new CadModule(this);
    m_camModule = new CamModule(this);
    m_cncModule = new CncModule(this);

    m_stack->addWidget(m_cadModule);
    m_stack->addWidget(m_camModule);
    m_stack->addWidget(m_cncModule);
}

void MainWindow::setupNavigation()
{
    auto* navBar = new QToolBar(tr("Navigation"), this);
    navBar->setMovable(false);
    navBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addToolBar(Qt::TopToolBarArea, navBar);

    auto* actCad = navBar->addAction(tr("1. CAD (Design)"));
    auto* actCam = navBar->addAction(tr("2. CAM (Manufacture)"));
    auto* actCnc = navBar->addAction(tr("3. CNC (Machine Control)"));

    connect(actCad, &QAction::triggered, this,
            [this]() { m_stack->setCurrentWidget(m_cadModule); });
    connect(actCam, &QAction::triggered, this,
            [this]() { m_stack->setCurrentWidget(m_camModule); });
    connect(actCnc, &QAction::triggered, this,
            [this]() { m_stack->setCurrentWidget(m_cncModule); });
}
