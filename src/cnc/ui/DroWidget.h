#pragma once

#include "cnc/core/grbl/GrblTypes.h"

#include <QGroupBox>

class QLabel;
class QPushButton;

// Digital read-out: machine state, X/Y/Z position and current feed, plus the
// zero / home / probe buttons. Displays what the backend reports and emits the
// user's intents; it holds no machine logic.
class DroWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit DroWidget(QWidget* parent = nullptr);

    void updateStatus(const cnc::GrblStatus& status);
    void showDisconnected();

signals:
    void zeroAxisRequested(const QString& axis);  // "X" / "Y" / "Z" / "ALL"
    void homeRequested();
    void probeRequested();

private:
    void setStateText(const QString& text, const QString& color);

    QLabel* m_state = nullptr;
    QLabel* m_posX = nullptr;
    QLabel* m_posY = nullptr;
    QLabel* m_posZ = nullptr;
    QLabel* m_feed = nullptr;

    QPushButton* m_zeroX = nullptr;
    QPushButton* m_zeroY = nullptr;
    QPushButton* m_zeroZ = nullptr;
    QPushButton* m_zeroAll = nullptr;
    QPushButton* m_home = nullptr;
    QPushButton* m_probe = nullptr;
};
