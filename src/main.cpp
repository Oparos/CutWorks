#include "app/MainWindow.h"

#include <QApplication>
#include <QFile>

namespace {

// Load the embedded theme (compiled into the binary via app.qrc), so styling
// works identically on every machine without shipping a separate .qss file.
void applyTheme(QApplication& app)
{
    QFile themeFile(QStringLiteral(":/theme/theme.qss"));
    if (themeFile.open(QFile::ReadOnly | QFile::Text)) {
        app.setStyleSheet(QString::fromUtf8(themeFile.readAll()));
    }
}

} // namespace

int main(int argc, char* argv[])
{
    // Smooth fractional high-DPI scaling (125% / 150% / 175% displays).
    // Must be set before the QApplication is constructed.
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("CutWorks");
    app.setStyle("Fusion");
    applyTheme(app);

    MainWindow window;
    window.showMaximized();

    return app.exec();
}
