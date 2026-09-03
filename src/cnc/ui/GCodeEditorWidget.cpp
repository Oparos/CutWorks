#include "cnc/ui/GCodeEditorWidget.h"

#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>

GCodeEditorWidget::GCodeEditorWidget(QWidget* parent)
    : QGroupBox(tr("G-Code"), parent)
{
    m_loadButton = new QPushButton(tr("Load G-Code File"), this);

    m_playButton = new QPushButton(QIcon(QStringLiteral(":/icons/play.svg")), tr("Play"), this);
    m_playButton->setObjectName(QStringLiteral("jobPlay"));

    m_pauseButton = new QPushButton(QIcon(QStringLiteral(":/icons/pause.svg")), tr("Pause"), this);
    m_pauseButton->setObjectName(QStringLiteral("jobPause"));

    m_stopButton = new QPushButton(QIcon(QStringLiteral(":/icons/stop.svg")), tr("Stop"), this);
    m_stopButton->setObjectName(QStringLiteral("jobStop"));

    m_dryRunButton = new QPushButton(tr("Dry Run"), this);
    m_dryRunButton->setObjectName(QStringLiteral("jobDryRun"));
    m_dryRunButton->setCheckable(true);

    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("gcodeList"));
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);

    auto* controls = new QHBoxLayout();
    controls->addWidget(m_playButton);
    controls->addWidget(m_pauseButton);
    controls->addWidget(m_stopButton);
    controls->addWidget(m_dryRunButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_loadButton);
    layout->addLayout(controls);
    layout->addWidget(m_list);

    connect(m_loadButton, &QPushButton::clicked, this, &GCodeEditorWidget::onLoadClicked);
    connect(m_playButton, &QPushButton::clicked, this, &GCodeEditorWidget::playRequested);
    connect(m_pauseButton, &QPushButton::clicked, this, &GCodeEditorWidget::pauseRequested);
    connect(m_stopButton, &QPushButton::clicked, this, &GCodeEditorWidget::stopRequested);
    connect(m_dryRunButton, &QPushButton::toggled, this, &GCodeEditorWidget::dryRunToggled);
    connect(m_list, &QListWidget::itemChanged, this, &GCodeEditorWidget::onItemChanged);

    connect(m_list, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QListWidgetItem* item = m_list->itemAt(pos);
        if (!item) {
            return;
        }
        const int line = m_list->row(item);
        QMenu menu(this);
        QAction* runFromHere = menu.addAction(tr("Run from this line"));
        connect(runFromHere, &QAction::triggered, this,
                [this, line]() { emit runFromHereRequested(line); });
        menu.exec(m_list->viewport()->mapToGlobal(pos));
    });
}

bool GCodeEditorWidget::loadGCodeFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open the G-Code file."));
        return false;
    }

    QStringList lines;
    m_list->blockSignals(true);  // avoid firing itemEdited while populating
    m_list->clear();

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        lines << line;
        auto* item = new QListWidgetItem(line, m_list);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
    m_list->blockSignals(false);

    emit fileLoaded(lines, filePath);
    return true;
}

void GCodeEditorWidget::highlightLine(int lineNumber)
{
    if (lineNumber >= 0 && lineNumber < m_list->count()) {
        m_list->setCurrentRow(lineNumber);
    }
    else {
        m_list->clearSelection();
        m_list->setCurrentRow(-1);
    }
}

void GCodeEditorWidget::clearContent()
{
    m_list->clear();
}

void GCodeEditorWidget::onLoadClicked()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, tr("Select G-Code file"), QString(),
        tr("G-Code Files (*.nc *.gcode *.txt);;All files (*.*)"));
    if (!filePath.isEmpty()) {
        loadGCodeFile(filePath);
    }
}

void GCodeEditorWidget::onItemChanged(QListWidgetItem* item)
{
    if (item) {
        emit lineEdited(m_list->row(item), item->text());
    }
}

void GCodeEditorWidget::onJobStateChanged(cnc::JobState state)
{
    const bool paused = (state == cnc::JobState::Paused);
    m_pauseButton->setText(paused ? tr("Resume") : tr("Pause"));
    m_pauseButton->setIcon(QIcon(paused ? QStringLiteral(":/icons/play.svg")
                                        : QStringLiteral(":/icons/pause.svg")));
}
