#pragma once

#include "cnc/core/grbl/GrblTypes.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace cnc {

enum class JobState
{
    Idle,     // no job running
    Running,  // streaming lines
    Paused    // feed hold
};

// Streams a loaded G-code program to the controller using the simple
// send-one / wait-for-"ok" protocol. It owns only the job state machine; it
// does not know about the serial port or the UI. Wire its signals to a
// GrblController: `sendCommandRequested` -> controller.sendCommand, and
// controller.responseReceived -> `onResponse`.
class JobStreamer : public QObject
{
    Q_OBJECT

public:
    explicit JobStreamer(QObject* parent = nullptr);

    void setGcode(const QStringList& lines);       // only while Idle
    void updateLine(int index, const QString& line);
    void setDryRun(bool enabled);
    bool isDryRun() const { return m_dryRun; }
    JobState state() const { return m_state; }

public slots:
    void start();          // begin streaming from the top (only while Idle)
    void pauseOrResume();   // Running -> feed hold, Paused -> cycle start
    void stop();            // abort + soft reset
    void reset();           // silent state reset (e.g. connection lost)

    // Fed from GrblController::responseReceived.
    void onResponse(cnc::GrblResponseKind kind, int code);

signals:
    void sendCommandRequested(const QString& command);
    void currentLineChanged(int index);   // -1 = no active line
    void stateChanged(cnc::JobState state);
    void finished();

private:
    void setState(JobState state);
    void sendNextLine();
    static QString applyDryRun(QString line, bool dryRun);

    QStringList m_lines;
    int m_index = 0;
    JobState m_state = JobState::Idle;
    bool m_dryRun = false;
};

} // namespace cnc
