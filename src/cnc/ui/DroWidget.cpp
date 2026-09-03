#include "cnc/ui/DroWidget.h"

#include "cnc/core/grbl/GrblProtocol.h"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

namespace {

QString colorForState(cnc::GrblState state)
{
    switch (state) {
    case cnc::GrblState::Idle:  return QStringLiteral("#33cc55");
    case cnc::GrblState::Run:
    case cnc::GrblState::Jog:   return QStringLiteral("#6c9bff");
    case cnc::GrblState::Alarm: return QStringLiteral("#ff5555");
    default:                    return QStringLiteral("#ff9800");
    }
}

QString formatCoord(double value)
{
    return QString::number(value, 'f', 3);
}

} // namespace

DroWidget::DroWidget(QWidget* parent)
    : QGroupBox(tr("Coordinates (DRO)"), parent)
{
    auto* layout = new QGridLayout(this);
    int row = 0;

    layout->addWidget(new QLabel(tr("Status:"), this), row, 0);
    m_state = new QLabel(this);
    m_state->setObjectName(QStringLiteral("droState"));
    layout->addWidget(m_state, row, 1, 1, 2);
    ++row;

    // One row per possible axis; rows past the reported axis count stay hidden
    // (a grid row with only hidden widgets collapses to nothing).
    for (int axis = 0; axis < kMaxAxes; ++axis) {
        m_axisName[axis] = new QLabel(cnc::grblAxisLetter(axis) + QStringLiteral(":"), this);
        m_axisValue[axis] = new QLabel(QStringLiteral("0.000"), this);
        m_axisValue[axis]->setObjectName(QStringLiteral("droValue"));

        layout->addWidget(m_axisName[axis], row, 0);
        layout->addWidget(m_axisValue[axis], row, 1);

        // Zero buttons only for the standard X/Y/Z axes.
        if (axis < 3) {
            const QString letter = cnc::grblAxisLetter(axis);
            auto* zero = new QPushButton(tr("Zero %1").arg(letter), this);
            layout->addWidget(zero, row, 2);
            connect(zero, &QPushButton::clicked, this,
                    [this, letter]() { emit zeroAxisRequested(letter); });
        }

        m_axisName[axis]->setVisible(axis < 3);   // default: X/Y/Z
        m_axisValue[axis]->setVisible(axis < 3);
        ++row;
    }

    layout->addWidget(new QLabel(tr("Feed (F):"), this), row, 0);
    m_feed = new QLabel(QStringLiteral("0"), this);
    m_feed->setObjectName(QStringLiteral("droFeed"));
    layout->addWidget(m_feed, row, 1);
    ++row;

    auto* zeroAll = new QPushButton(tr("Zero All"), this);
    auto* home = new QPushButton(tr("Home Machine ($H)"), this);
    auto* probe = new QPushButton(tr("Probe Z (find material)"), this);
    layout->addWidget(zeroAll, row++, 0, 1, 3);
    layout->addWidget(home, row++, 0, 1, 3);
    layout->addWidget(probe, row++, 0, 1, 3);

    connect(zeroAll, &QPushButton::clicked, this, [this]() { emit zeroAxisRequested(QStringLiteral("ALL")); });
    connect(home, &QPushButton::clicked, this, &DroWidget::homeRequested);
    connect(probe, &QPushButton::clicked, this, &DroWidget::probeRequested);

    showDisconnected();
}

void DroWidget::updateStatus(const cnc::GrblStatus& status)
{
    setStateText(status.rawState, colorForState(status.state));

    if (status.hasPosition) {
        const int axes = static_cast<int>(status.position.size());
        for (int axis = 0; axis < kMaxAxes; ++axis) {
            const bool present = axis < axes;
            m_axisName[axis]->setVisible(present);
            m_axisValue[axis]->setVisible(present);
            if (present) {
                m_axisValue[axis]->setText(formatCoord(status.position[axis]));
            }
        }
    }

    if (status.feed) {
        m_feed->setText(tr("%1 mm/min").arg(QString::number(*status.feed, 'f', 0)));
    }
}

void DroWidget::showDisconnected()
{
    setStateText(tr("Disconnected"), QStringLiteral("#888888"));
}

void DroWidget::setStateText(const QString& text, const QString& color)
{
    m_state->setText(text);
    m_state->setStyleSheet(QStringLiteral("color: %1;").arg(color));
}
