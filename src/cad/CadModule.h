#pragma once

#include <QWidget>

// Entry-point widget for the CAD workspace (geometry design).
// Placeholder for now; real design tools are added during migration.
class CadModule : public QWidget
{
    Q_OBJECT

public:
    explicit CadModule(QWidget* parent = nullptr);
};
