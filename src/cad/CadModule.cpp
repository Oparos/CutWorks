#include "cad/CadModule.h"

#include <QLabel>
#include <QVBoxLayout>

CadModule::CadModule(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    auto* label = new QLabel(tr("CAD — design workspace (placeholder)"), this);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
}
