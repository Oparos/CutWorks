#include "cnc/ui/ConnectionWidget.h"

#include <QComboBox>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>

ConnectionWidget::ConnectionWidget(QWidget* parent)
    : QGroupBox(tr("GRBL Connection"), parent)
{
    auto* layout = new QGridLayout(this);

    m_portLabel = new QLabel(tr("Port:"), this);
    m_baudLabel = new QLabel(tr("Baud:"), this);

    m_portCombo = new QComboBox(this);
    m_portCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_baudCombo = new QComboBox(this);
    m_baudCombo->addItems({"9600", "115200", "250000"});
    m_baudCombo->setCurrentText("115200");

    m_refreshButton = new QPushButton(this);
    m_refreshButton->setIcon(QIcon(QStringLiteral(":/icons/refresh.svg")));
    m_refreshButton->setToolTip(tr("Refresh port list"));

    m_connectButton = new QPushButton(tr("Connect"), this);

    layout->addWidget(m_portLabel, 0, 0);
    layout->addWidget(m_portCombo, 0, 1);
    layout->addWidget(m_refreshButton, 0, 2);
    layout->addWidget(m_baudLabel, 1, 0);
    layout->addWidget(m_baudCombo, 1, 1);
    layout->addWidget(m_connectButton, 2, 0, 1, 3);

    connect(m_refreshButton, &QPushButton::clicked, this, &ConnectionWidget::refreshRequested);
    connect(m_connectButton, &QPushButton::clicked, this, &ConnectionWidget::onConnectButtonClicked);
}

void ConnectionWidget::onConnectButtonClicked()
{
    if (m_connected) {
        emit disconnectRequested();
        return;
    }

    const QString portName = m_portCombo->currentData().toString();
    if (portName.isEmpty()) {
        return;  // "no ports" placeholder is selected
    }
    emit connectRequested(portName, m_baudCombo->currentText().toInt());
}

void ConnectionWidget::setPorts(const QList<cnc::SerialPortInfo>& ports)
{
    if (m_connected) {
        return;  // do not disturb the list during an active connection
    }

    const QString previous = m_portCombo->currentData().toString();

    m_portCombo->clear();
    if (ports.isEmpty()) {
        m_portCombo->addItem(tr("No ports available"), QString());
        m_connectButton->setEnabled(false);
    }
    else {
        for (const cnc::SerialPortInfo& port : ports) {
            m_portCombo->addItem(port.displayLabel(), port.name);
        }
        m_connectButton->setEnabled(true);

        const int index = m_portCombo->findData(previous);
        if (index != -1) {
            m_portCombo->setCurrentIndex(index);
        }
    }
}

void ConnectionWidget::setConnected(bool connected)
{
    m_connected = connected;

    m_connectButton->setText(connected ? tr("Disconnect") : tr("Connect"));
    m_connectButton->setEnabled(true);
    m_portCombo->setEnabled(!connected);
    m_baudCombo->setEnabled(!connected);
    m_refreshButton->setEnabled(!connected);
}
