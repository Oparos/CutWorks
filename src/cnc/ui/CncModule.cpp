#include "cnc/ui/CncModule.h"

#include "cnc/core/GrblController.h"
#include "cnc/core/JobStreamer.h"
#include "cnc/ui/ConnectionWidget.h"
#include "cnc/ui/ConsoleWidget.h"
#include "cnc/ui/DroWidget.h"
#include "cnc/ui/GCodeEditorWidget.h"
#include "cnc/ui/JogWidget.h"
#include "cnc/ui/OverrideWidget.h"
#include "cnc/ui/ThcWidget.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

namespace {
constexpr int kPortPollIntervalMs = 1000;
constexpr int kLeftPanelMaxWidth = 340;
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

    // Right: the G-code job on top, the console below it.
    m_editor = new GCodeEditorWidget(this);
    m_console = new ConsoleWidget(this);

    auto* rightLayout = new QVBoxLayout();
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(m_editor, 3);
    rightLayout->addWidget(m_console, 1);

    mainLayout->addWidget(leftScroll);
    mainLayout->addLayout(rightLayout, 1);
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
