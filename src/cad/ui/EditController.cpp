#include "cad/ui/EditController.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/commands/RemoveEntityCommand.h"
#include "cad/ui/render/EntityItem.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGraphicsScene>
#include <QSet>
#include <QSpinBox>
#include <QUndoStack>

namespace cad {

EditController::EditController(CadDocument* document, QUndoStack* undoStack,
                              QGraphicsScene* scene, QWidget* dialogParent, QObject* parent)
    : QObject(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
    , m_dialogParent(dialogParent)
{
}

QList<int> EditController::selectedEntityIds() const
{
    QList<int> ids;
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto* entityItem = dynamic_cast<EntityItem*>(item)) {
            ids.append(entityItem->entityId());
        }
    }
    return ids;
}

void EditController::deleteSelection()
{
    const QList<int> ids = selectedEntityIds();  // collect first: removal changes selection
    if (ids.isEmpty()) {
        return;
    }
    m_undoStack->beginMacro(tr("Delete"));
    for (int id : ids) {
        m_undoStack->push(new RemoveEntityCommand(m_document, id, tr("Delete entity")));
    }
    m_undoStack->endMacro();
}

void EditController::copySelection()
{
    m_clipboard.clear();
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto* entityItem = dynamic_cast<EntityItem*>(item)) {
            if (CadEntity* e = m_document->entity(entityItem->entityId())) {
                m_clipboard.push_back(e->clone());
            }
        }
    }
}

void EditController::paste()
{
    if (m_clipboard.empty()) {
        return;
    }

    const QPointF offset(10.0, 10.0);  // so pasted copies are visible and movable
    QSet<int> pastedIds;

    m_undoStack->beginMacro(tr("Paste"));
    for (const auto& source : m_clipboard) {
        std::unique_ptr<CadEntity> copy = source->clone();
        copy->translate(offset);
        auto* command = new AddEntityCommand(m_document, std::move(copy), tr("Paste entity"));
        m_undoStack->push(command);
        pastedIds.insert(command->entityId());
    }
    m_undoStack->endMacro();

    m_scene->clearSelection();
    for (QGraphicsItem* item : m_scene->items()) {
        if (auto* entityItem = dynamic_cast<EntityItem*>(item)) {
            if (pastedIds.contains(entityItem->entityId())) {
                item->setSelected(true);
            }
        }
    }
}

void EditController::arrayRectangular()
{
    const QList<int> ids = selectedEntityIds();
    if (ids.isEmpty()) {
        return;
    }

    QDialog dialog(m_dialogParent);
    dialog.setWindowTitle(tr("Rectangular Array"));
    auto* form = new QFormLayout(&dialog);
    auto* colsSpin = new QSpinBox(&dialog);
    colsSpin->setRange(1, 1000);
    colsSpin->setValue(3);
    auto* rowsSpin = new QSpinBox(&dialog);
    rowsSpin->setRange(1, 1000);
    rowsSpin->setValue(2);
    auto* dxSpin = new QDoubleSpinBox(&dialog);
    dxSpin->setRange(-100000, 100000);
    dxSpin->setValue(50);
    dxSpin->setSuffix(tr(" mm"));
    auto* dySpin = new QDoubleSpinBox(&dialog);
    dySpin->setRange(-100000, 100000);
    dySpin->setValue(50);
    dySpin->setSuffix(tr(" mm"));
    form->addRow(tr("Columns"), colsSpin);
    form->addRow(tr("Rows"), rowsSpin);
    form->addRow(tr("Column spacing"), dxSpin);
    form->addRow(tr("Row spacing"), dySpin);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const int cols = colsSpin->value();
    const int rows = rowsSpin->value();
    const double dx = dxSpin->value();
    const double dy = dySpin->value();

    m_undoStack->beginMacro(tr("Rectangular array"));
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (r == 0 && c == 0) {
                continue;  // the original stays in place
            }
            const QPointF offset(c * dx, r * dy);
            for (int id : ids) {
                if (CadEntity* e = m_document->entity(id)) {
                    std::unique_ptr<CadEntity> copy = e->clone();
                    copy->translate(offset);
                    m_undoStack->push(new AddEntityCommand(m_document, std::move(copy), tr("Array copy")));
                }
            }
        }
    }
    m_undoStack->endMacro();
}

void EditController::arrayPolar()
{
    const QList<int> ids = selectedEntityIds();
    if (ids.isEmpty()) {
        return;
    }

    QRectF bounds;
    for (int id : ids) {
        if (CadEntity* e = m_document->entity(id)) {
            bounds = bounds.isNull() ? e->bounds() : bounds.united(e->bounds());
        }
    }
    const QPointF defaultCenter = bounds.center();

    QDialog dialog(m_dialogParent);
    dialog.setWindowTitle(tr("Polar Array"));
    auto* form = new QFormLayout(&dialog);
    auto* countSpin = new QSpinBox(&dialog);
    countSpin->setRange(2, 1000);
    countSpin->setValue(6);
    auto* stepSpin = new QDoubleSpinBox(&dialog);
    stepSpin->setRange(-360, 360);
    stepSpin->setValue(60);
    stepSpin->setSuffix(tr(" deg"));
    auto* cxSpin = new QDoubleSpinBox(&dialog);
    cxSpin->setRange(-100000, 100000);
    cxSpin->setValue(defaultCenter.x());
    auto* cySpin = new QDoubleSpinBox(&dialog);
    cySpin->setRange(-100000, 100000);
    cySpin->setValue(defaultCenter.y());
    form->addRow(tr("Count (incl. original)"), countSpin);
    form->addRow(tr("Step angle"), stepSpin);
    form->addRow(tr("Center X"), cxSpin);
    form->addRow(tr("Center Y"), cySpin);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const int count = countSpin->value();
    const double step = stepSpin->value();
    const QPointF center(cxSpin->value(), cySpin->value());

    m_undoStack->beginMacro(tr("Polar array"));
    for (int i = 1; i < count; ++i) {
        const double angle = i * step;
        for (int id : ids) {
            if (CadEntity* e = m_document->entity(id)) {
                std::unique_ptr<CadEntity> copy = e->clone();
                copy->rotate(center, angle);
                m_undoStack->push(new AddEntityCommand(m_document, std::move(copy), tr("Array copy")));
            }
        }
    }
    m_undoStack->endMacro();
}

} // namespace cad
