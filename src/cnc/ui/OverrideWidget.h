#pragma once

#include "cnc/core/grbl/GrblTypes.h"

#include <QGroupBox>

// Feed-rate override buttons (-10 / -1 / 100% / +1 / +10). Emits a semantic
// action; the backend turns it into the GRBL real-time byte.
class OverrideWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit OverrideWidget(QWidget* parent = nullptr);

signals:
    void overrideRequested(cnc::FeedOverride action);
};
