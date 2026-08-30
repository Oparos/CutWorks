#include "cnc/ui/ThcWidget.h"

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

ThcWidget::ThcWidget(QWidget* parent)
    : QGroupBox(tr("Torch Height Control (THC)"), parent)
{
    auto* layout = new QVBoxLayout(this);

    // Modes follow the GRBL-HAL plasma plugin ($350).
    m_mode = new QComboBox(this);
    m_mode->addItem(tr("0 - Disabled"), 0);
    m_mode->addItem(tr("1 - Arc voltage (THC)"), 1);
    m_mode->addItem(tr("2 - Up/down signals (THC)"), 2);
    m_mode->addItem(tr("3 - Arc OK only"), 3);

    layout->addWidget(new QLabel(tr("THC Mode:"), this));
    layout->addWidget(m_mode);

    m_toggle = new QPushButton(tr("Enable THC (Auto)"), this);
    m_toggle->setObjectName(QStringLiteral("thcToggle"));
    m_toggle->setCheckable(true);
    m_toggle->setToolTip(tr("Enable/disable THC via virtual port (M63/M62 P2). "
                            "Requires virtual ports enabled ($674)."));
    layout->addWidget(m_toggle);

    connect(m_mode, &QComboBox::currentIndexChanged, this, &ThcWidget::onModeChanged);
    connect(m_toggle, &QPushButton::toggled, this, &ThcWidget::onToggled);
}

void ThcWidget::onModeChanged()
{
    emit modeChanged(m_mode->currentData().toInt());
}

void ThcWidget::onToggled(bool enabled)
{
    m_toggle->setText(enabled ? tr("THC Active") : tr("Enable THC (Auto)"));
    emit enabledChanged(enabled);
}
