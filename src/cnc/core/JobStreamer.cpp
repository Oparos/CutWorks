#include "cnc/core/JobStreamer.h"

#include <QRegularExpression>

namespace cnc {

JobStreamer::JobStreamer(QObject* parent)
    : QObject(parent)
{
}

void JobStreamer::setGcode(const QStringList& lines)
{
    if (m_state != JobState::Idle) {
        return;  // never swap the program mid-job
    }
    m_lines = lines;
    m_index = 0;
}

void JobStreamer::updateLine(int index, const QString& line)
{
    if (index >= 0 && index < m_lines.size()) {
        m_lines[index] = line;
    }
}

void JobStreamer::setDryRun(bool enabled)
{
    m_dryRun = enabled;
}

void JobStreamer::start()
{
    if (m_state != JobState::Idle || m_lines.isEmpty()) {
        return;
    }
    setState(JobState::Running);
    sendNextLine();
}

void JobStreamer::pauseOrResume()
{
    if (m_state == JobState::Running) {
        setState(JobState::Paused);
        emit sendCommandRequested(QStringLiteral("!"));  // feed hold
    }
    else if (m_state == JobState::Paused) {
        setState(JobState::Running);
        emit sendCommandRequested(QStringLiteral("~"));  // cycle start
    }
}

void JobStreamer::stop()
{
    m_index = 0;
    setState(JobState::Idle);
    emit currentLineChanged(-1);
    emit sendCommandRequested(QStringLiteral("\x18"));  // soft reset
}

void JobStreamer::reset()
{
    m_index = 0;
    setState(JobState::Idle);
    emit currentLineChanged(-1);
}

void JobStreamer::onResponse(GrblResponseKind kind, int /*code*/)
{
    if (m_state != JobState::Running) {
        return;
    }

    if (kind == GrblResponseKind::Ok) {
        sendNextLine();
    }
    else if (kind == GrblResponseKind::Error || kind == GrblResponseKind::Alarm) {
        // A syntax error or alarm mid-cut is fatal for safety: abort the job.
        stop();
    }
}

void JobStreamer::setState(JobState state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(m_state);
    }
}

void JobStreamer::sendNextLine()
{
    while (true) {
        if (m_index >= m_lines.size()) {
            m_index = 0;
            setState(JobState::Idle);
            emit currentLineChanged(-1);
            emit finished();
            return;
        }

        const int lineIndex = m_index;
        ++m_index;
        emit currentLineChanged(lineIndex);

        const QString out = applyDryRun(m_lines.at(lineIndex), m_dryRun).trimmed();

        // Skip empty lines and comments locally so we don't wait for an "ok".
        if (out.isEmpty() || out.startsWith(';')) {
            continue;
        }

        emit sendCommandRequested(out);
        return;
    }
}

QString JobStreamer::applyDryRun(QString line, bool dryRun)
{
    if (!dryRun) {
        return line;
    }

    // Strip torch on/off (M3/M4/M5) and THC on/off (M62/M63 P2) so a job can be
    // rehearsed without firing the torch. Whole-token matching avoids corrupting
    // e.g. M30 (which the legacy naive string removal broke).
    static const QRegularExpression torch(
        QStringLiteral("\\bM0?[345]\\b"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression thc(
        QStringLiteral("\\bM6[23]\\s*P2\\b"), QRegularExpression::CaseInsensitiveOption);

    line.remove(torch);
    line.remove(thc);
    return line;
}

} // namespace cnc
