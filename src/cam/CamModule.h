#pragma once

#include <QWidget>

// Entry-point widget for the CAM workspace (toolpath generation).
// Placeholder for now; contour/toolpath logic is added during migration.
class CamModule : public QWidget
{
    Q_OBJECT

public:
    explicit CamModule(QWidget* parent = nullptr);
};
