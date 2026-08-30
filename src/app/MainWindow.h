#pragma once

#include <QMainWindow>

class QStackedWidget;

// Top-level application shell. Owns the three workspace modules (CAD, CAM, CNC)
// and lets the user switch between them. Contains no domain logic itself.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void setupModules();
    void setupNavigation();

    QStackedWidget* m_stack = nullptr;
    QWidget* m_cadModule = nullptr;
    QWidget* m_camModule = nullptr;
    QWidget* m_cncModule = nullptr;
};
