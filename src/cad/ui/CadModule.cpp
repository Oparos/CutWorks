#include "cad/ui/CadModule.h"

#include "cad/core/CadDocument.h"
#include "cad/ui/EditController.h"
#include "cad/ui/ToolInputBar.h"
#include "cad/ui/render/CadScene.h"
#include "cad/ui/render/CadView.h"
#include "cad/ui/tools/DrawArcTool.h"
#include "cad/ui/tools/DrawCircleTool.h"
#include "cad/ui/tools/DrawLineTool.h"
#include "cad/ui/tools/DrawPointTool.h"
#include "cad/ui/tools/DrawPolygonTool.h"
#include "cad/ui/tools/DrawPolylineTool.h"
#include "cad/ui/tools/DrawSlotTool.h"
#include "cad/ui/tools/DrawTangentTool.h"
#include "cad/ui/tools/DrawTeardropTool.h"
#include "cad/ui/tools/DrawRectangleTool.h"
#include "cad/ui/tools/ExtendTool.h"
#include "cad/ui/tools/FilletChamferTool.h"
#include "cad/ui/tools/MirrorTool.h"
#include "cad/ui/tools/MoveTool.h"
#include "cad/ui/tools/RotateTool.h"
#include "cad/ui/tools/SelectTool.h"
#include "cad/ui/tools/TrimTool.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QUndoStack>
#include <QVBoxLayout>

CadModule::CadModule(QWidget* parent)
    : QWidget(parent)
    , m_document(new cad::CadDocument(this))
    , m_snap(m_document)
    , m_undoStack(new QUndoStack(this))
{
    m_scene = new CadScene(m_document, this);
    m_view = new CadView(this);
    m_view->setScene(m_scene);
    m_view->setSnapEngine(&m_snap);

    m_selectTool = new cad::SelectTool(m_scene, this);
    m_moveTool = new cad::MoveTool(m_document, m_undoStack, m_scene, this);
    m_rotateTool = new cad::RotateTool(m_document, m_undoStack, m_scene, this);
    m_mirrorTool = new cad::MirrorTool(m_document, m_undoStack, m_scene, this);
    m_trimTool = new cad::TrimTool(m_document, m_undoStack, m_scene, this);
    m_extendTool = new cad::ExtendTool(m_document, m_undoStack, m_scene, this);
    m_filletTool = new cad::FilletChamferTool(cad::FilletChamferTool::Mode::Fillet, m_document,
                                              m_undoStack, m_scene, this);
    m_chamferTool = new cad::FilletChamferTool(cad::FilletChamferTool::Mode::Chamfer, m_document,
                                               m_undoStack, m_scene, this);
    m_lineTool = new cad::DrawLineTool(m_document, m_undoStack, m_scene, this);
    m_polylineTool = new cad::DrawPolylineTool(m_document, m_undoStack, m_scene, this);
    m_rectangleTool = new cad::DrawRectangleTool(m_document, m_undoStack, m_scene, this);
    m_circleTool = new cad::DrawCircleTool(m_document, m_undoStack, m_scene, this);
    m_arcTool = new cad::DrawArcTool(m_document, m_undoStack, m_scene, this);
    m_tangentTool = new cad::DrawTangentTool(m_document, m_undoStack, m_scene, this);
    m_slotTool = new cad::DrawSlotTool(m_document, m_undoStack, m_scene, this);
    m_teardropTool = new cad::DrawTeardropTool(m_document, m_undoStack, m_scene, this);
    m_polygonTool = new cad::DrawPolygonTool(m_document, m_undoStack, m_scene, this);
    m_pointTool = new cad::DrawPointTool(m_document, m_undoStack, this);

    m_edit = new cad::EditController(m_document, m_undoStack, m_scene, this, this);
    m_inputBar = new ToolInputBar(this);

    // --- Left: tool buttons (data-driven) ---
    struct ToolEntry
    {
        QString label;
        cad::CadTool* tool;
    };
    const ToolEntry toolEntries[] = {
        {tr("Select"), m_selectTool},
        {tr("Move"), m_moveTool},
        {tr("Rotate"), m_rotateTool},
        {tr("Mirror"), m_mirrorTool},
        {tr("Trim"), m_trimTool},
        {tr("Extend"), m_extendTool},
        {tr("Fillet"), m_filletTool},
        {tr("Chamfer"), m_chamferTool},
        {tr("Line"), m_lineTool},
        {tr("Polyline"), m_polylineTool},
        {tr("Rectangle"), m_rectangleTool},
        {tr("Circle"), m_circleTool},
        {tr("Arc"), m_arcTool},
        {tr("Tangent"), m_tangentTool},
        {tr("Slot"), m_slotTool},
        {tr("Teardrop"), m_teardropTool},
        {tr("Polygon"), m_polygonTool},
        {tr("Point"), m_pointTool},
    };

    auto* toolGroup = new QButtonGroup(this);
    toolGroup->setExclusive(true);
    auto* toolsLayout = new QVBoxLayout();
    toolsLayout->setAlignment(Qt::AlignTop);

    bool firstTool = true;
    for (const ToolEntry& entry : toolEntries) {
        auto* button = new QPushButton(entry.label, this);
        button->setCheckable(true);
        button->setFocusPolicy(Qt::NoFocus);
        toolGroup->addButton(button);
        toolsLayout->addWidget(button);
        cad::CadTool* tool = entry.tool;
        connect(button, &QPushButton::toggled, this, [this, tool](bool on) {
            if (on) {
                setActiveTool(tool);
            }
        });
        if (firstTool) {
            button->setChecked(true);
            firstTool = false;
        }
    }

    // --- Top bar: undo/redo + selection operations ---
    auto* btnUndo = new QPushButton(tr("Undo"), this);
    auto* btnRedo = new QPushButton(tr("Redo"), this);
    auto* btnArrayRect = new QPushButton(tr("Array ▦"), this);
    auto* btnArrayPolar = new QPushButton(tr("Array ⟳"), this);
    for (QPushButton* b : {btnUndo, btnRedo, btnArrayRect, btnArrayPolar}) {
        b->setFocusPolicy(Qt::NoFocus);
    }
    connect(btnUndo, &QPushButton::clicked, m_undoStack, &QUndoStack::undo);
    connect(btnRedo, &QPushButton::clicked, m_undoStack, &QUndoStack::redo);
    connect(btnArrayRect, &QPushButton::clicked, this, [this]() { m_edit->arrayRectangular(); });
    connect(btnArrayPolar, &QPushButton::clicked, this, [this]() { m_edit->arrayPolar(); });

    auto* topBar = new QHBoxLayout();
    topBar->addWidget(btnUndo);
    topBar->addWidget(btnRedo);
    topBar->addWidget(btnArrayRect);
    topBar->addWidget(btnArrayPolar);
    topBar->addStretch();

    // --- Snap toggles: enable/disable each snap mode (all on by default) ---
    struct SnapToggle
    {
        const char* label;
        bool cad::SnapEngine::*flag;
    };
    const SnapToggle snapToggles[] = {
        {QT_TR_NOOP("End"), &cad::SnapEngine::endpoints},
        {QT_TR_NOOP("Mid"), &cad::SnapEngine::midpoints},
        {QT_TR_NOOP("Center"), &cad::SnapEngine::centers},
        {QT_TR_NOOP("Intersect"), &cad::SnapEngine::intersections},
        {QT_TR_NOOP("Perp"), &cad::SnapEngine::perpendicular},
        {QT_TR_NOOP("Tangent"), &cad::SnapEngine::tangent},
        {QT_TR_NOOP("Grid"), &cad::SnapEngine::grid},
    };
    auto* snapBar = new QHBoxLayout();
    snapBar->addWidget(new QLabel(tr("Snap:"), this));
    for (const SnapToggle& toggle : snapToggles) {
        auto* checkBox = new QCheckBox(tr(toggle.label), this);
        checkBox->setChecked(true);
        checkBox->setFocusPolicy(Qt::NoFocus);
        bool cad::SnapEngine::*flag = toggle.flag;
        connect(checkBox, &QCheckBox::toggled, this, [this, flag](bool on) { m_snap.*flag = on; });
        snapBar->addWidget(checkBox);
    }
    snapBar->addStretch();

    auto* centerLayout = new QVBoxLayout();
    centerLayout->addLayout(topBar);
    centerLayout->addLayout(snapBar);
    centerLayout->addWidget(m_view, 1);
    centerLayout->addWidget(m_inputBar);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->addLayout(toolsLayout);
    mainLayout->addLayout(centerLayout, 1);

    // --- Parametric input bar wiring ---
    connect(m_view, &CadView::focusInputRequested, this, [this]() {
        m_inputBar->focusFirstField();
    });
    connect(m_inputBar, &ToolInputBar::committed, this, [this](const QVector<double>& values) {
        if (m_activeTool) {
            m_activeTool->applyInput(values);
        }
    });
    connect(m_inputBar, &ToolInputBar::cancelRequested, this, [this]() {
        if (m_activeTool) {
            m_activeTool->onCancel();
        }
        m_view->setFocus();
    });

    // --- Shortcuts ---
    auto* undoSc = new QShortcut(QKeySequence::Undo, this);
    connect(undoSc, &QShortcut::activated, m_undoStack, &QUndoStack::undo);
    auto* redoSc = new QShortcut(QKeySequence::Redo, this);
    connect(redoSc, &QShortcut::activated, m_undoStack, &QUndoStack::redo);
    auto* deleteSc = new QShortcut(QKeySequence::Delete, this);
    connect(deleteSc, &QShortcut::activated, this, [this]() { m_edit->deleteSelection(); });
    auto* copySc = new QShortcut(QKeySequence::Copy, this);
    connect(copySc, &QShortcut::activated, this, [this]() { m_edit->copySelection(); });
    auto* pasteSc = new QShortcut(QKeySequence::Paste, this);
    connect(pasteSc, &QShortcut::activated, this, [this]() { m_edit->paste(); });

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
