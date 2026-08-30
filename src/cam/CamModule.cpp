#include "cam/CamModule.h"

#include <QLabel>
#include <QVBoxLayout>

CamModule::CamModule(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    auto* label = new QLabel(tr("CAM — toolpath workspace (placeholder)"), this);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
}
