#include "cad/ui/CadModule.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/RemoveEntityCommand.h"
#include "cad/ui/ToolInputBar.h"
#include "cad/ui/render/CadScene.h"
#include "cad/ui/render/CadView.h"
#include "cad/ui/render/EntityItem.h"
#include "cad/ui/tools/SelectTool.h"
#include "cad/ui/tools/DrawCircleTool.h"
#include "cad/ui/tools/DrawLineTool.h"
#include "cad/ui/tools/DrawPointTool.h"
#include "cad/ui/tools/DrawArcTool.h"
#include "cad/ui/tools/DrawPolygonTool.h"
#include "cad/ui/tools/DrawPolylineTool.h"
#include "cad/ui/tools/DrawRectangleTool.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPushButton>
#include <QShortcut>
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

    for (QPushButton* b : {btnSelect, btnLine, btnPolyline, btnRectangle, btnCircle, btnArc, btnPolygon, btnPoint}) {
        b->setCheckable(true);
        b->setFocusPolicy(Qt::NoFocus);
        toolGroup->addButton(b);
        toolsLayout->addWidget(b);
    }
    btnSelect->setChecked(true);

    connect(btnSelect, &QPushButton::toggled, this, [this](bool on) {
        if (on) setActiveTool(m_selectTool);
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

    auto* topBar = new QHBoxLayout();
    topBar->addWidget(btnUndo);
    topBar->addWidget(btnRedo);
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

void CadModule::deleteSelection()
{
    // Collect ids first — removing entities changes the scene's selection.
    QList<int> ids;
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto* entityItem = dynamic_cast<EntityItem*>(item)) {
            ids.append(entityItem->entityId());
        }
    }
    if (ids.isEmpty()) {
        return;
    }

    m_undoStack->beginMacro(tr("Delete"));
    for (int id : ids) {
        m_undoStack->push(new cad::RemoveEntityCommand(m_document, id, tr("Delete entity")));
    }
    m_undoStack->endMacro();
}
