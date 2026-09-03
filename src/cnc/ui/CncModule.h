#pragma once

#include <QWidget>

class QTimer;
class ConnectionWidget;
class ConfigWidget;
class ConsoleWidget;
class DroWidget;
class GCodeEditorWidget;
class GCodeScene;
class GCodeView;
class JogWidget;
class OverrideWidget;
class ThcWidget;

namespace cnc {
class GrblController;
class JobStreamer;
}

// Entry-point widget for the CNC workspace. Composes the machine-control widgets
// and wires them to the backend GrblController / JobStreamer. Holds no protocol
// logic itself.
class CncModule : public QWidget
{
    Q_OBJECT

public:
    explicit CncModule(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    void buildUi();
    void wireSignals();
    void refreshPorts();

    cnc::GrblController* m_controller = nullptr;
    cnc::JobStreamer* m_streamer = nullptr;

    ConnectionWidget* m_connection = nullptr;
    DroWidget* m_dro = nullptr;
    JogWidget* m_jog = nullptr;
    OverrideWidget* m_override = nullptr;
    ThcWidget* m_thc = nullptr;
    ConsoleWidget* m_console = nullptr;
    GCodeEditorWidget* m_editor = nullptr;
    ConfigWidget* m_config = nullptr;
    GCodeScene* m_scene = nullptr;
    GCodeView* m_view = nullptr;

    QTimer* m_portTimer = nullptr;
};
