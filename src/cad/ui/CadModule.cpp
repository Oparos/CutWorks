#include "cad/ui/CadModule.h"

#include "cad/core/CadDocument.h"
#include "cad/ui/ToolInputBar.h"
#include "cad/ui/render/CadScene.h"
#include "cad/ui/render/CadView.h"
#include "cad/ui/tools/DrawLineTool.h"

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

    m_lineTool = new cad::DrawLineTool(m_document, m_undoStack, m_scene, this);
    m_inputBar = new ToolInputBar(this);

    // --- Left: tool buttons ---
    auto* btnSelect = new QPushButton(tr("Select"), this);
    auto* btnLine = new QPushButton(tr("Line"), this);
    for (QPushButton* b : {btnSelect, btnLine}) {
        b->setCheckable(true);
        b->setFocusPolicy(Qt::NoFocus);
    }
    btnSelect->setChecked(true);

    auto* toolGroup = new QButtonGroup(this);
    toolGroup->setExclusive(true);
    toolGroup->addButton(btnSelect);
    toolGroup->addButton(btnLine);

    auto* toolsLayout = new QVBoxLayout();
    toolsLayout->setAlignment(Qt::AlignTop);
    toolsLayout->addWidget(btnSelect);
    toolsLayout->addWidget(btnLine);

    connect(btnSelect, &QPushButton::toggled, this, [this](bool on) {
        if (on) setActiveTool(nullptr);
    });
    connect(btnLine, &QPushButton::toggled, this, [this](bool on) {
        if (on) setActiveTool(m_lineTool);
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

    // Parametric input committed -> hand values to the active tool, then return
    // focus to the canvas so clicks / Esc / right-click work again.
    connect(m_inputBar, &ToolInputBar::committed, this, [this](const QVector<double>& values) {
        if (m_activeTool) {
            m_activeTool->applyInput(values);
        }
        m_view->setFocus();
    });

    // Undo/redo shortcuts.
    auto* undoSc = new QShortcut(QKeySequence::Undo, this);
    connect(undoSc, &QShortcut::activated, m_undoStack, &QUndoStack::undo);
    auto* redoSc = new QShortcut(QKeySequence::Redo, this);
    connect(redoSc, &QShortcut::activated, m_undoStack, &QUndoStack::redo);

    setActiveTool(nullptr);  // start in Select mode
}

void CadModule::setActiveTool(cad::CadTool* tool)
{
    if (m_activeTool) {
        disconnect(m_activeTool, &cad::CadTool::inputChanged, this, nullptr);
        m_activeTool->deactivate();
    }

    m_activeTool = tool;
    m_view->setTool(tool);

    if (m_activeTool) {
        connect(m_activeTool, &cad::CadTool::inputChanged, this, &CadModule::refreshInputBar);
    }
    refreshInputBar();
}

void CadModule::refreshInputBar()
{
    m_inputBar->setFields(m_activeTool ? m_activeTool->inputFields()
                                       : QList<cad::CadTool::InputField>{});
}
