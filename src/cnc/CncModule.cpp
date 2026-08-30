#include "cnc/CncModule.h"

#include <QLabel>
#include <QVBoxLayout>

CncModule::CncModule(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    auto* label = new QLabel(tr("CNC — machine control workspace (placeholder)"), this);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
}
