#pragma once

#include <QGroupBox>

class QComboBox;
class QPushButton;
class QLabel;

// Jog pad (8 XY directions + Z), with a step-size and feed selector. The step
// mode is stored as combo item data (a numeric mm value, or a sentinel for
// "continuous"), never as display text — so the continuous mode is detected
// independently of the UI language. Presentation only: it emits jog intents.
class JogWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit JogWidget(QWidget* parent = nullptr);

    // Reusable by keyboard jogging in the parent module.
    void jogInDirection(double dx, double dy, double dz);
    void stopJog();  // emits a cancel only in continuous mode

signals:
    // dx/dy/dz are unit directions; distance and feed come from the selectors.
    void jogRequested(double dx, double dy, double dz, double distanceMm, double feed);
    void jogCancelRequested();

private:
    QPushButton* makeArrow(const QString& iconName, const QString& tooltip,
                           double dx, double dy, double dz);

    bool isContinuous() const;
    double currentDistanceMm() const;
    double currentFeed() const;

    QComboBox* m_stepCombo = nullptr;
    QComboBox* m_feedCombo = nullptr;
    QLabel* m_stepLabel = nullptr;
    QLabel* m_feedLabel = nullptr;
};
