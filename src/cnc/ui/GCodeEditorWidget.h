#pragma once

#include "cnc/core/JobStreamer.h"

#include <QGroupBox>
#include <QStringList>

class QListWidget;
class QListWidgetItem;
class QPushButton;

// Loads a G-code file, shows it as an editable list, highlights the line being
// streamed, and offers play / pause / stop / dry-run controls. Presentation
// only: it emits the user's intents and reflects the job state.
class GCodeEditorWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit GCodeEditorWidget(QWidget* parent = nullptr);

    bool loadGCodeFile(const QString& filePath);
    void highlightLine(int lineNumber);  // -1 clears the highlight
    void clearContent();

signals:
    void fileLoaded(const QStringList& gcodeLines, const QString& filePath);
    void lineEdited(int lineNumber, const QString& newLine);
    void playRequested();
    void pauseRequested();
    void stopRequested();
    void dryRunToggled(bool enabled);
    void runFromHereRequested(int line);

public slots:
    void onJobStateChanged(cnc::JobState state);

private:
    void onLoadClicked();
    void onItemChanged(QListWidgetItem* item);

    QListWidget* m_list = nullptr;
    QPushButton* m_loadButton = nullptr;
    QPushButton* m_playButton = nullptr;
    QPushButton* m_pauseButton = nullptr;
    QPushButton* m_stopButton = nullptr;
    QPushButton* m_dryRunButton = nullptr;
};
