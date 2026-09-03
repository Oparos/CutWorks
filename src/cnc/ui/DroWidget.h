#pragma once

#include "cnc/core/grbl/GrblTypes.h"

#include <QGroupBox>

#include <array>

class QLabel;
class QPushButton;

// Digital read-out: machine state, per-axis position and current feed, plus the
// zero / home / probe buttons. It shows exactly as many axis rows as the
// controller reports (X/Y/Z for a 3-axis machine, plus A/B/C when present), so
// no axis count is hard-coded. Displays what the backend reports; holds no
// machine logic.
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

    static constexpr int kMaxAxes = 6;  // X Y Z A B C

    QLabel* m_state = nullptr;
    std::array<QLabel*, kMaxAxes> m_axisName{};
    std::array<QLabel*, kMaxAxes> m_axisValue{};
    QLabel* m_feed = nullptr;
};
