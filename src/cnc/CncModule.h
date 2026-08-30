#pragma once

#include <QWidget>

// Entry-point widget for the CNC workspace (machine control).
// Placeholder for now; serial communication and controls are added during migration.
class CncModule : public QWidget
{
    Q_OBJECT

public:
    explicit CncModule(QWidget* parent = nullptr);
};
