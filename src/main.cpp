#include "app/MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("CutWorks");
    app.setStyle("Fusion");

    MainWindow window;
    window.showMaximized();

    return app.exec();
}
