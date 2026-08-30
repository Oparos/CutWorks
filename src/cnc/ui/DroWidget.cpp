#include "cnc/ui/DroWidget.h"

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

    m_state = new QLabel(this);
    m_state->setObjectName(QStringLiteral("droState"));

    auto makeValue = [this]() {
        auto* label = new QLabel(QStringLiteral("0.000"), this);
        label->setObjectName(QStringLiteral("droValue"));
        return label;
    };
    m_posX = makeValue();
    m_posY = makeValue();
    m_posZ = makeValue();

    m_feed = new QLabel(QStringLiteral("0"), this);
    m_feed->setObjectName(QStringLiteral("droFeed"));

    m_zeroX = new QPushButton(tr("Zero X"), this);
    m_zeroY = new QPushButton(tr("Zero Y"), this);
    m_zeroZ = new QPushButton(tr("Zero Z"), this);
    m_zeroAll = new QPushButton(tr("Zero All"), this);
    m_home = new QPushButton(tr("Home Machine ($H)"), this);
    m_probe = new QPushButton(tr("Probe Z (find material)"), this);

    layout->addWidget(new QLabel(tr("Status:"), this), 0, 0);
    layout->addWidget(m_state, 0, 1, 1, 2);

    layout->addWidget(new QLabel(QStringLiteral("X:"), this), 1, 0);
    layout->addWidget(m_posX, 1, 1);
    layout->addWidget(m_zeroX, 1, 2);

    layout->addWidget(new QLabel(QStringLiteral("Y:"), this), 2, 0);
    layout->addWidget(m_posY, 2, 1);
    layout->addWidget(m_zeroY, 2, 2);

    layout->addWidget(new QLabel(QStringLiteral("Z:"), this), 3, 0);
    layout->addWidget(m_posZ, 3, 1);
    layout->addWidget(m_zeroZ, 3, 2);

    layout->addWidget(new QLabel(tr("Feed (F):"), this), 4, 0);
    layout->addWidget(m_feed, 4, 1);

    layout->addWidget(m_zeroAll, 5, 0, 1, 3);
    layout->addWidget(m_home, 6, 0, 1, 3);
    layout->addWidget(m_probe, 7, 0, 1, 3);

    connect(m_zeroX, &QPushButton::clicked, this, [this]() { emit zeroAxisRequested(QStringLiteral("X")); });
    connect(m_zeroY, &QPushButton::clicked, this, [this]() { emit zeroAxisRequested(QStringLiteral("Y")); });
    connect(m_zeroZ, &QPushButton::clicked, this, [this]() { emit zeroAxisRequested(QStringLiteral("Z")); });
    connect(m_zeroAll, &QPushButton::clicked, this, [this]() { emit zeroAxisRequested(QStringLiteral("ALL")); });
    connect(m_home, &QPushButton::clicked, this, &DroWidget::homeRequested);
    connect(m_probe, &QPushButton::clicked, this, &DroWidget::probeRequested);

    showDisconnected();
}

void DroWidget::updateStatus(const cnc::GrblStatus& status)
{
    setStateText(status.rawState, colorForState(status.state));

    if (status.hasPosition) {
        m_posX->setText(formatCoord(status.x));
        m_posY->setText(formatCoord(status.y));
        m_posZ->setText(formatCoord(status.z));
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
