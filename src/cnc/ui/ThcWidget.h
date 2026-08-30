#pragma once

#include <QGroupBox>

class QComboBox;
class QPushButton;

// Torch height control panel: operating mode ($350) and an enable/disable toggle
// (M63/M62 P2). Detailed THC tuning ($351-$369, ...) lives in the settings
// table, not here. Presentation only.
class ThcWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit ThcWidget(QWidget* parent = nullptr);

signals:
    void modeChanged(int mode);
    void enabledChanged(bool enabled);

private:
    void onModeChanged();
    void onToggled(bool enabled);

    QComboBox* m_mode = nullptr;
    QPushButton* m_toggle = nullptr;
};
