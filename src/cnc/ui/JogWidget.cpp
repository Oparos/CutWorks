#include "cnc/ui/JogWidget.h"

#include <QComboBox>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>

namespace {
// Travel sent for a continuous jog; the move is stopped by a jog-cancel on
// release, so the exact value only needs to be "far enough".
constexpr double kContinuousDistanceMm = 500.0;
// Sentinel stored as the "Continuous" step item's data.
constexpr double kContinuousSentinel = -1.0;
constexpr int kArrowIconPx = 22;
} // namespace

JogWidget::JogWidget(QWidget* parent)
    : QGroupBox(tr("Manual Control (Jog)"), parent)
{
    auto* layout = new QGridLayout(this);

    // --- XY jog pad (row/col 0..2) + Z column (col 3) ---
    layout->addWidget(makeArrow(QStringLiteral("arrow-up-left"),    QStringLiteral("X- Y+"), -1,  1, 0), 0, 0);
    layout->addWidget(makeArrow(QStringLiteral("arrow-up"),         QStringLiteral("Y+"),     0,  1, 0), 0, 1);
    layout->addWidget(makeArrow(QStringLiteral("arrow-up-right"),   QStringLiteral("X+ Y+"),  1,  1, 0), 0, 2);
    layout->addWidget(makeArrow(QStringLiteral("arrow-left"),       QStringLiteral("X-"),    -1,  0, 0), 1, 0);
    layout->addWidget(makeArrow(QStringLiteral("arrow-right"),      QStringLiteral("X+"),     1,  0, 0), 1, 2);
    layout->addWidget(makeArrow(QStringLiteral("arrow-down-left"),  QStringLiteral("X- Y-"), -1, -1, 0), 2, 0);
    layout->addWidget(makeArrow(QStringLiteral("arrow-down"),       QStringLiteral("Y-"),     0, -1, 0), 2, 1);
    layout->addWidget(makeArrow(QStringLiteral("arrow-down-right"), QStringLiteral("X+ Y-"),  1, -1, 0), 2, 2);

    auto* zUp = new QPushButton(QStringLiteral("Z+"), this);
    auto* zDown = new QPushButton(QStringLiteral("Z-"), this);
    for (QPushButton* z : {zUp, zDown}) {
        z->setFocusPolicy(Qt::NoFocus);  // keep arrow keys for jogging, not focus
    }
    connect(zUp, &QPushButton::pressed, this, [this]() { jogInDirection(0, 0, 1); });
    connect(zUp, &QPushButton::released, this, &JogWidget::stopJog);
    connect(zDown, &QPushButton::pressed, this, [this]() { jogInDirection(0, 0, -1); });
    connect(zDown, &QPushButton::released, this, &JogWidget::stopJog);
    layout->addWidget(zUp, 0, 3);
    layout->addWidget(zDown, 2, 3);

    // --- step / feed selectors ---
    m_stepCombo = new QComboBox(this);
    m_stepCombo->addItem(tr("Continuous"), kContinuousSentinel);
    m_stepCombo->addItem(QStringLiteral("0.1 mm"), 0.1);
    m_stepCombo->addItem(QStringLiteral("1 mm"), 1.0);
    m_stepCombo->addItem(QStringLiteral("10 mm"), 10.0);
    m_stepCombo->addItem(QStringLiteral("50 mm"), 50.0);
    m_stepCombo->addItem(QStringLiteral("100 mm"), 100.0);
    m_stepCombo->setCurrentIndex(2);  // default: 1 mm

    m_feedCombo = new QComboBox(this);
    m_feedCombo->addItem(QStringLiteral("500 mm/min"), 500.0);
    m_feedCombo->addItem(QStringLiteral("1000 mm/min"), 1000.0);
    m_feedCombo->addItem(QStringLiteral("3000 mm/min"), 3000.0);
    m_feedCombo->addItem(QStringLiteral("5000 mm/min"), 5000.0);
    m_feedCombo->addItem(QStringLiteral("10000 mm/min"), 10000.0);
    m_feedCombo->setCurrentIndex(1);  // default: 1000 mm/min

    m_stepLabel = new QLabel(tr("Step:"), this);
    m_feedLabel = new QLabel(tr("Feed:"), this);

    layout->addWidget(m_stepLabel, 3, 0);
    layout->addWidget(m_stepCombo, 3, 1, 1, 3);
    layout->addWidget(m_feedLabel, 4, 0);
    layout->addWidget(m_feedCombo, 4, 1, 1, 3);
}

QPushButton* JogWidget::makeArrow(const QString& iconName, const QString& tooltip,
                                  double dx, double dy, double dz)
{
    auto* button = new QPushButton(this);
    button->setIcon(QIcon(QStringLiteral(":/icons/%1.svg").arg(iconName)));
    button->setIconSize(QSize(kArrowIconPx, kArrowIconPx));
    button->setToolTip(tooltip);
    button->setFocusPolicy(Qt::NoFocus);

    connect(button, &QPushButton::pressed, this, [this, dx, dy, dz]() { jogInDirection(dx, dy, dz); });
    connect(button, &QPushButton::released, this, &JogWidget::stopJog);
    return button;
}

void JogWidget::jogInDirection(double dx, double dy, double dz)
{
    emit jogRequested(dx, dy, dz, currentDistanceMm(), currentFeed());
}

void JogWidget::stopJog()
{
    // In step mode the move is finite and already done; only a continuous jog
    // needs an explicit cancel.
    if (isContinuous()) {
        emit jogCancelRequested();
    }
}

bool JogWidget::isContinuous() const
{
    return m_stepCombo->currentData().toDouble() < 0.0;
}

double JogWidget::currentDistanceMm() const
{
    return isContinuous() ? kContinuousDistanceMm : m_stepCombo->currentData().toDouble();
}

double JogWidget::currentFeed() const
{
    return m_feedCombo->currentData().toDouble();
}
