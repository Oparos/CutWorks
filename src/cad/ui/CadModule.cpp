#include "cad/ui/CadModule.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/commands/RemoveEntityCommand.h"
#include "cad/ui/ToolInputBar.h"
#include "cad/ui/render/CadScene.h"
#include "cad/ui/render/CadView.h"
#include "cad/ui/render/EntityItem.h"
#include "cad/ui/tools/MirrorTool.h"
#include "cad/ui/tools/MoveTool.h"
#include "cad/ui/tools/RotateTool.h"
#include "cad/ui/tools/SelectTool.h"
#include "cad/ui/tools/DrawCircleTool.h"
#include "cad/ui/tools/DrawLineTool.h"
#include "cad/ui/tools/DrawPointTool.h"
#include "cad/ui/tools/DrawArcTool.h"
#include "cad/ui/tools/DrawPolygonTool.h"
#include "cad/ui/tools/DrawPolylineTool.h"
#include "cad/ui/tools/DrawRectangleTool.h"

#include <QButtonGroup>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSet>
#include <QShortcut>
#include <QSpinBox>
#include <QUndoStack>
#include <QVBoxLayout>

CadModule::CadModule(QWidget* parent)
    : QWidget(parent)
    , m_document(new cad::CadDocument(this))
    , m_undoStack(new QUndoStack(this))
{
    m_scene = new CadScene(m_document, this);
    m_view = new CadView(this);
    m_view->setScene(m_scene);

    m_selectTool = new cad::SelectTool(m_scene, this);
    m_moveTool = new cad::MoveTool(m_document, m_undoStack, m_scene, this);
    m_rotateTool = new cad::RotateTool(m_document, m_undoStack, m_scene, this);
    m_mirrorTool = new cad::MirrorTool(m_document, m_undoStack, m_scene, this);
    m_lineTool = new cad::DrawLineTool(m_document, m_undoStack, m_scene, this);
    m_circleTool = new cad::DrawCircleTool(m_document, m_undoStack, m_scene, this);
    m_pointTool = new cad::DrawPointTool(m_document, m_undoStack, this);
    m_polylineTool = new cad::DrawPolylineTool(m_document, m_undoStack, m_scene, this);
    m_rectangleTool = new cad::DrawRectangleTool(m_document, m_undoStack, m_scene, this);
    m_arcTool = new cad::DrawArcTool(m_document, m_undoStack, m_scene, this);
    m_polygonTool = new cad::DrawPolygonTool(m_document, m_undoStack, m_scene, this);
    m_inputBar = new ToolInputBar(this);

    // --- Left: tool buttons ---
    auto* btnSelect = new QPushButton(tr("Select"), this);
    auto* btnMove = new QPushButton(tr("Move"), this);
    auto* btnRotate = new QPushButton(tr("Rotate"), this);
    auto* btnMirror = new QPushButton(tr("Mirror"), this);
    auto* btnLine = new QPushButton(tr("Line"), this);
    auto* btnPolyline = new QPushButton(tr("Polyline"), this);
    auto* btnRectangle = new QPushButton(tr("Rectangle"), this);
    auto* btnCircle = new QPushButton(tr("Circle"), this);
    auto* btnArc = new QPushButton(tr("Arc"), this);
    auto* btnPolygon = new QPushButton(tr("Polygon"), this);
    auto* btnPoint = new QPushButton(tr("Point"), this);

    auto* toolGroup = new QButtonGroup(this);
    toolGroup->setExclusive(true);

    auto* toolsLayout = new QVBoxLayout();
    toolsLayout->setAlignment(Qt::AlignTop);

    for (QPushButton* b : {btnSelect, btnMove, btnRotate, btnMirror, btnLine, btnPolyline, btnRectangle, btnCircle, btnArc, btnPolygon, btnPoint}) {
        b->setCheckable(true);
        b->setFocusPolicy(Qt::NoFocus);
        toolGroup->addButton(b);
        toolsLayout->addWidget(b);
    }
    btnSelect->setChecked(true);

    connect(btnSelect, &QPushButton::toggled, this, [this](bool on) {
        if (on) setActiveTool(m_selectTool);
    });
    connect(btnMove, &QPushButton::toggled, this, [this](bool on) {
        if (on) setActiveTool(m_moveTool);
    });
    connect(btnRotate, &QPushButton::toggled, this, [this](bool on) {
        if (on) setActiveTool(m_rotateTool);
    });
    connect(btnMirror, &QPushButton::toggled, this, [this](bool on) {
        if (on) setActiveTool(m_mirrorTool);
    });
    connect(btnLine, &QPushButton::toggled, this, [this](bool on) {
        if (on) setActiveTool(m_lineTool);
    });
    connect(btnPolyline, &QPushButton::toggled, this, [this](bool on) {
        if (on) setActiveTool(m_polylineTool);
    });
    connect(btnRectangle, &QPushButton::toggled, this, [this](bool on) {
        if (on) setActiveTool(m_rectangleTool);
    });
    connect(btnCircle, &QPushButton::toggled, this, [this](bool on) {
        if (on) setActiveTool(m_circleTool);
    });
    connect(btnArc, &QPushButton::toggled, this, [this](bool on) {
        if (on) setActiveTool(m_arcTool);
    });
    connect(btnPolygon, &QPushButton::toggled, this, [this](bool on) {
        if (on) setActiveTool(m_polygonTool);
    });
    connect(btnPoint, &QPushButton::toggled, this, [this](bool on) {
        if (on) setActiveTool(m_pointTool);
    });

    // --- Center: undo/redo bar, view, parametric input bar ---
    auto* btnUndo = new QPushButton(tr("Undo"), this);
    auto* btnRedo = new QPushButton(tr("Redo"), this);
    for (QPushButton* b : {btnUndo, btnRedo}) {
        b->setFocusPolicy(Qt::NoFocus);
    }
    connect(btnUndo, &QPushButton::clicked, m_undoStack, &QUndoStack::undo);
    connect(btnRedo, &QPushButton::clicked, m_undoStack, &QUndoStack::redo);

    // One-shot operations on the current selection (not modal tools).
    auto* btnArrayRect = new QPushButton(tr("Array ▦"), this);
    auto* btnArrayPolar = new QPushButton(tr("Array ⟳"), this);
    for (QPushButton* b : {btnArrayRect, btnArrayPolar}) {
        b->setFocusPolicy(Qt::NoFocus);
    }
    connect(btnArrayRect, &QPushButton::clicked, this, &CadModule::arrayRectangular);
    connect(btnArrayPolar, &QPushButton::clicked, this, &CadModule::arrayPolar);

    auto* topBar = new QHBoxLayout();
    topBar->addWidget(btnUndo);
    topBar->addWidget(btnRedo);
    topBar->addWidget(btnArrayRect);
    topBar->addWidget(btnArrayPolar);
    topBar->addStretch();

    auto* centerLayout = new QVBoxLayout();
    centerLayout->addLayout(topBar);
    centerLayout->addWidget(m_view, 1);
    centerLayout->addWidget(m_inputBar);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->addLayout(toolsLayout);
    mainLayout->addLayout(centerLayout, 1);

    // Tab in the view jumps focus into the parametric input bar.
    connect(m_view, &CadView::focusInputRequested, this, [this]() {
        m_inputBar->focusFirstField();
    });

    // Parametric input committed -> hand values to the active tool. Focus stays
    // in the bar so the next segment can be typed straight away; the tool
    // re-focuses the first field via requestInputFocus.
    connect(m_inputBar, &ToolInputBar::committed, this, [this](const QVector<double>& values) {
        if (m_activeTool) {
            m_activeTool->applyInput(values);
        }
    });

    // Esc in the bar cancels the current operation and returns focus to canvas.
    connect(m_inputBar, &ToolInputBar::cancelRequested, this, [this]() {
        if (m_activeTool) {
            m_activeTool->onCancel();
        }
        m_view->setFocus();
    });

    // Undo/redo shortcuts.
    auto* undoSc = new QShortcut(QKeySequence::Undo, this);
    connect(undoSc, &QShortcut::activated, m_undoStack, &QUndoStack::undo);
    auto* redoSc = new QShortcut(QKeySequence::Redo, this);
    connect(redoSc, &QShortcut::activated, m_undoStack, &QUndoStack::redo);

    auto* deleteSc = new QShortcut(QKeySequence::Delete, this);
    connect(deleteSc, &QShortcut::activated, this, &CadModule::deleteSelection);

    auto* copySc = new QShortcut(QKeySequence::Copy, this);
    connect(copySc, &QShortcut::activated, this, &CadModule::copySelection);
    auto* pasteSc = new QShortcut(QKeySequence::Paste, this);
    connect(pasteSc, &QShortcut::activated, this, &CadModule::pasteClipboard);

    setActiveTool(m_selectTool);  // start in Select mode
}

void CadModule::setActiveTool(cad::CadTool* tool)
{
    if (m_activeTool) {
        disconnect(m_activeTool, nullptr, this, nullptr);
        m_activeTool->deactivate();
    }

    m_activeTool = tool;
    m_view->setTool(tool);

    if (m_activeTool) {
        connect(m_activeTool, &cad::CadTool::inputChanged, this, &CadModule::refreshInputBar);
        connect(m_activeTool, &cad::CadTool::requestInputFocus, this,
                [this]() { m_inputBar->focusFirstField(); });
    }
    refreshInputBar();
}

void CadModule::refreshInputBar()
{
    m_inputBar->setFields(m_activeTool ? m_activeTool->inputFields()
                                       : QList<cad::CadTool::InputField>{});
}

QList<int> CadModule::selectedEntityIds() const
{
    QList<int> ids;
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto* entityItem = dynamic_cast<EntityItem*>(item)) {
            ids.append(entityItem->entityId());
        }
    }
    return ids;
}

void CadModule::deleteSelection()
{
    const QList<int> ids = selectedEntityIds();  // collect first: removal changes selection
    if (ids.isEmpty()) {
        return;
    }
    m_undoStack->beginMacro(tr("Delete"));
    for (int id : ids) {
        m_undoStack->push(new cad::RemoveEntityCommand(m_document, id, tr("Delete entity")));
    }
    m_undoStack->endMacro();
}

void CadModule::arrayRectangular()
{
    const QList<int> ids = selectedEntityIds();
    if (ids.isEmpty()) {
        return;
    }

    QDialog dialog(this);
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
                if (cad::CadEntity* e = m_document->entity(id)) {
                    std::unique_ptr<cad::CadEntity> copy = e->clone();
                    copy->translate(offset);
                    m_undoStack->push(new cad::AddEntityCommand(m_document, std::move(copy), tr("Array copy")));
                }
            }
        }
    }
    m_undoStack->endMacro();
}

void CadModule::arrayPolar()
{
    const QList<int> ids = selectedEntityIds();
    if (ids.isEmpty()) {
        return;
    }

    // Default center = bounding-box center of the selection.
    QRectF bounds;
    for (int id : ids) {
        if (cad::CadEntity* e = m_document->entity(id)) {
            bounds = bounds.isNull() ? e->bounds() : bounds.united(e->bounds());
        }
    }
    const QPointF defaultCenter = bounds.center();

    QDialog dialog(this);
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
            if (cad::CadEntity* e = m_document->entity(id)) {
                std::unique_ptr<cad::CadEntity> copy = e->clone();
                copy->rotate(center, angle);
                m_undoStack->push(new cad::AddEntityCommand(m_document, std::move(copy), tr("Array copy")));
            }
        }
    }
    m_undoStack->endMacro();
}

void CadModule::copySelection()
{
    m_clipboard.clear();
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto* entityItem = dynamic_cast<EntityItem*>(item)) {
            if (cad::CadEntity* e = m_document->entity(entityItem->entityId())) {
                m_clipboard.push_back(e->clone());
            }
        }
    }
}

void CadModule::pasteClipboard()
{
    if (m_clipboard.empty()) {
        return;
    }

    // Paste at a small offset so the copies are visible and ready to be moved.
    const QPointF offset(10.0, 10.0);
    QSet<int> pastedIds;

    m_undoStack->beginMacro(tr("Paste"));
    for (const auto& source : m_clipboard) {
        std::unique_ptr<cad::CadEntity> copy = source->clone();
        copy->translate(offset);
        auto* command = new cad::AddEntityCommand(m_document, std::move(copy), tr("Paste entity"));
        m_undoStack->push(command);
        pastedIds.insert(command->entityId());
    }
    m_undoStack->endMacro();

    // Select the freshly pasted entities.
    m_scene->clearSelection();
    for (QGraphicsItem* item : m_scene->items()) {
        if (auto* entityItem = dynamic_cast<EntityItem*>(item)) {
            if (pastedIds.contains(entityItem->entityId())) {
                item->setSelected(true);
            }
        }
    }
}
