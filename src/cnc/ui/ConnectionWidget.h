#pragma once

#include "cnc/core/SerialPortInfo.h"

#include <QGroupBox>
#include <QList>

class QComboBox;
class QPushButton;
class QLabel;

// Port / baud selection and the connect-disconnect button. Purely presentation:
// it emits intents and reflects state, but never touches the serial port.
class ConnectionWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit ConnectionWidget(QWidget* parent = nullptr);

    // Refresh the port list, preserving the current selection. Ignored while
    // connected (the list is frozen so it does not flicker mid-session).
    void setPorts(const QList<cnc::SerialPortInfo>& ports);

    // Reflect the connection state (button label, enabled controls).
    void setConnected(bool connected);

signals:
    void connectRequested(const QString& portName, int baudRate);
    void disconnectRequested();
    void refreshRequested();

private:
    void onConnectButtonClicked();

    QLabel* m_portLabel = nullptr;
    QLabel* m_baudLabel = nullptr;
    QComboBox* m_portCombo = nullptr;
    QComboBox* m_baudCombo = nullptr;
    QPushButton* m_refreshButton = nullptr;
    QPushButton* m_connectButton = nullptr;

    bool m_connected = false;
};
