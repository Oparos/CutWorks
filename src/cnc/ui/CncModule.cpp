#include "cnc/ui/CncModule.h"

#include "cnc/core/GrblController.h"
#include "cnc/core/JobStreamer.h"
#include "cnc/core/gcode/GCodeParser.h"
#include "cnc/core/gcode/Simulator.h"
#include "cnc/ui/ConfigWidget.h"
#include "cnc/ui/ConnectionWidget.h"
#include "cnc/ui/ConsoleWidget.h"
#include "cnc/ui/DroWidget.h"
#include "cnc/ui/GCodeEditorWidget.h"
#include "cnc/ui/JogWidget.h"
#include "cnc/ui/OverrideWidget.h"
#include "cnc/ui/ThcWidget.h"
#include "cnc/ui/render/GCodeScene.h"
#include "cnc/ui/render/GCodeView.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QScrollArea>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace {
constexpr int kPortPollIntervalMs = 1000;
constexpr int kLeftPanelMaxWidth = 340;
// Assumed rapid rate for time estimation when the machine's $110 is unknown.
constexpr double kRapidRateMmMin = 3000.0;
} // namespace

CncModule::CncModule(QWidget* parent)
    : QWidget(parent)
    , m_controller(new cnc::GrblController(this))
    , m_streamer(new cnc::JobStreamer(this))
{
    // The module itself holds keyboard focus so the arrow keys can jog.
    setFocusPolicy(Qt::StrongFocus);

    buildUi();
    wireSignals();

    m_portTimer = new QTimer(this);
    connect(m_portTimer, &QTimer::timeout, this, &CncModule::refreshPorts);
    m_portTimer->start(kPortPollIntervalMs);

    refreshPorts();
}

void CncModule::buildUi()
{
    auto* mainLayout = new QHBoxLayout(this);

    // Left: connection + DRO + jog, in a scroll area so the growing control
    // column still fits on small screens.
    auto* leftPanel = new QWidget;
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    m_connection = new ConnectionWidget(leftPanel);
    m_dro = new DroWidget(leftPanel);
    m_jog = new JogWidget(leftPanel);
    m_override = new OverrideWidget(leftPanel);
    m_thc = new ThcWidget(leftPanel);

    leftLayout->addWidget(m_connection);
    leftLayout->addWidget(m_dro);
    leftLayout->addWidget(m_jog);
    leftLayout->addWidget(m_override);
    leftLayout->addWidget(m_thc);
    leftLayout->addStretch();

    auto* leftScroll = new QScrollArea(this);
    leftScroll->setWidget(leftPanel);
    leftScroll->setWidgetResizable(true);
    leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    leftScroll->setFrameShape(QFrame::NoFrame);
    leftScroll->setMaximumWidth(kLeftPanelMaxWidth);

    // Right: a tabbed area — the workspace (preview + job + console) and settings.
    m_editor = new GCodeEditorWidget(this);
    m_console = new ConsoleWidget(this);
    m_config = new ConfigWidget(this);

    m_scene = new GCodeScene(this);
    m_view = new GCodeView(this);
    m_view->setScene(m_scene);

    auto* topRow = new QHBoxLayout();
    topRow->addWidget(m_view, 3);
    topRow->addWidget(m_editor, 1);

    auto* workspace = new QWidget(this);
    auto* workspaceLayout = new QVBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);
    workspaceLayout->addLayout(topRow, 3);
    workspaceLayout->addWidget(m_console, 1);

    auto* tabs = new QTabWidget(this);
    tabs->addTab(workspace, tr("Workspace"));
    tabs->addTab(m_config, tr("Machine Configuration"));

    mainLayout->addWidget(leftScroll);
    mainLayout->addWidget(tabs, 1);
}

void CncModule::wireSignals()
{
    // --- Connection UI intents -> backend ---
    connect(m_connection, &ConnectionWidget::connectRequested,
            m_controller, &cnc::GrblController::connectTo);
    connect(m_connection, &ConnectionWidget::disconnectRequested,
            m_controller, &cnc::GrblController::disconnectFromMachine);
    connect(m_connection, &ConnectionWidget::refreshRequested,
            this, &CncModule::refreshPorts);

    connect(m_console, &ConsoleWidget::commandEntered,
            m_controller, &cnc::GrblController::sendCommand);

    connect(m_dro, &DroWidget::zeroAxisRequested,
            m_controller, &cnc::GrblController::zeroAxis);
    connect(m_dro, &DroWidget::homeRequested,
            m_controller, &cnc::GrblController::homeCycle);
    connect(m_dro, &DroWidget::probeRequested,
            m_controller, &cnc::GrblController::probeMaterialZ);

    connect(m_jog, &JogWidget::jogRequested,
            m_controller, &cnc::GrblController::jog);
    connect(m_jog, &JogWidget::jogCancelRequested,
            m_controller, &cnc::GrblController::cancelJog);

    connect(m_override, &OverrideWidget::overrideRequested,
            m_controller, &cnc::GrblController::feedOverride);

    connect(m_thc, &ThcWidget::modeChanged,
            m_controller, &cnc::GrblController::setThcMode);
    connect(m_thc, &ThcWidget::enabledChanged,
            m_controller, &cnc::GrblController::setThcEnabled);

    // --- Backend events -> UI ---
    connect(m_controller, &cnc::GrblController::connectionChanged,
            this, [this](bool connected) {
                m_connection->setConnected(connected);
                if (!connected) {
                    m_dro->showDisconnected();
                    m_streamer->reset();  // drop any running job if the link is lost
                }
            });
    connect(m_controller, &cnc::GrblController::statusUpdated,
            m_dro, &DroWidget::updateStatus);
    connect(m_controller, &cnc::GrblController::lineLogged,
            m_console, &ConsoleWidget::appendMessage);

    // --- G-code job: editor <-> streamer <-> controller ---
    connect(m_editor, &GCodeEditorWidget::fileLoaded,
            m_streamer, &cnc::JobStreamer::setGcode);
    connect(m_editor, &GCodeEditorWidget::lineEdited,
            m_streamer, &cnc::JobStreamer::updateLine);
    connect(m_editor, &GCodeEditorWidget::pauseRequested,
            m_streamer, &cnc::JobStreamer::pauseOrResume);
    connect(m_editor, &GCodeEditorWidget::stopRequested,
            m_streamer, &cnc::JobStreamer::stop);
    connect(m_editor, &GCodeEditorWidget::dryRunToggled,
            m_streamer, &cnc::JobStreamer::setDryRun);

    // Guard: a job may only start when the machine is connected.
    connect(m_editor, &GCodeEditorWidget::playRequested, this, [this]() {
        if (m_controller->isConnected()) {
            m_streamer->start();
        }
        else {
            m_console->appendMessage(tr("Connect to the machine before running a job."),
                                     cnc::LogCategory::Warning);
        }
    });
    connect(m_editor, &GCodeEditorWidget::runFromHereRequested, this, [this](int line) {
        if (m_controller->isConnected()) {
            m_streamer->startFromLine(line);
        }
        else {
            m_console->appendMessage(tr("Connect to the machine before running a job."),
                                     cnc::LogCategory::Warning);
        }
    });

    connect(m_streamer, &cnc::JobStreamer::sendCommandRequested,
            m_controller, &cnc::GrblController::sendCommand);
    connect(m_streamer, &cnc::JobStreamer::currentLineChanged,
            m_editor, &GCodeEditorWidget::highlightLine);
    connect(m_streamer, &cnc::JobStreamer::stateChanged,
            m_editor, &GCodeEditorWidget::onJobStateChanged);
    connect(m_streamer, &cnc::JobStreamer::finished, this, [this]() {
        m_console->appendMessage(tr("Job finished."), cnc::LogCategory::Info);
    });
    connect(m_controller, &cnc::GrblController::responseReceived,
            m_streamer, &cnc::JobStreamer::onResponse);

    // --- Settings editor ($$) <-> controller ---
    connect(m_config, &ConfigWidget::readRequested, this, [this]() {
        m_config->clearSettings();
        m_controller->requestSettings();
    });
    connect(m_config, &ConfigWidget::saveRequested, this, [this](const QStringList& commands) {
        for (const QString& command : commands) {
            m_controller->sendCommand(command);
        }
    });
    connect(m_controller, &cnc::GrblController::settingReceived,
            m_config, &ConfigWidget::addSetting);

    // --- Preview: parse the loaded program, draw it, estimate time/size ---
    connect(m_editor, &GCodeEditorWidget::fileLoaded, this,
            [this](const QStringList& lines, const QString&) {
                const cnc::Toolpath path = cnc::GCodeParser().parse(lines);
                m_scene->setToolpath(path);
                m_view->zoomToFit();

                const cnc::JobEstimate est = cnc::estimateJob(path, kRapidRateMmMin);
                const int minutes = static_cast<int>(est.seconds) / 60;
                const int seconds = static_cast<int>(est.seconds) % 60;
                m_console->appendMessage(
                    tr("Estimated time: %1 min %2 s   |   size: %3 x %4 mm")
                        .arg(minutes)
                        .arg(seconds, 2, 10, QChar('0'))
                        .arg(est.bounds.width(), 0, 'f', 1)
                        .arg(est.bounds.height(), 0, 'f', 1),
                    cnc::LogCategory::Info);
            });

    // Move the tool marker on the preview as status reports arrive.
    connect(m_controller, &cnc::GrblController::statusUpdated, this,
            [this](const cnc::GrblStatus& status) {
                if (status.hasPosition && status.position.size() >= 2) {
                    m_scene->setToolPosition(status.position[0], status.position[1]);
                }
            });
}

void CncModule::refreshPorts()
{
    m_connection->setPorts(m_controller->availablePorts());
}

void CncModule::keyPressEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) {
        return;  // hold = one continuous jog, not a stream of key repeats
    }

    switch (event->key()) {
    case Qt::Key_Up:       m_jog->jogInDirection(0, 1, 0);  return;
    case Qt::Key_Down:     m_jog->jogInDirection(0, -1, 0); return;
    case Qt::Key_Left:     m_jog->jogInDirection(-1, 0, 0); return;
    case Qt::Key_Right:    m_jog->jogInDirection(1, 0, 0);  return;
    case Qt::Key_PageUp:   m_jog->jogInDirection(0, 0, 1);  return;
    case Qt::Key_PageDown: m_jog->jogInDirection(0, 0, -1); return;
    default:               break;
    }
    QWidget::keyPressEvent(event);
}

void CncModule::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) {
        return;
    }

    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        m_jog->stopJog();
        return;
    default:
        break;
    }
    QWidget::keyReleaseEvent(event);
}
