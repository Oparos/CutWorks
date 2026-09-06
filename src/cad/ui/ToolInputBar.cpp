#include "cad/ui/ToolInputBar.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>

ToolInputBar::ToolInputBar(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);

    for (int i = 0; i < kMaxFields; ++i) {
        m_labels[i] = new QLabel(this);
        m_edits[i] = new QLineEdit(this);
        m_edits[i]->setMaximumWidth(90);
        layout->addWidget(m_labels[i]);
        layout->addWidget(m_edits[i]);
        m_labels[i]->hide();
        m_edits[i]->hide();
        m_edits[i]->installEventFilter(this);  // catch Esc to cancel
        connect(m_edits[i], &QLineEdit::returnPressed, this, [this, i]() { onReturnPressed(i); });
        // textEdited fires only on real user typing (not our setText) — once the
        // user edits a field we stop overwriting it with live values.
        connect(m_edits[i], &QLineEdit::textEdited, this, [this, i]() { m_dirty[i] = true; });
    }
    layout->addStretch();
}

bool ToolInputBar::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            emit cancelRequested();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ToolInputBar::focusFirstField()
{
    // A fresh operation: allow live values to flow again until the user types.
    m_dirty.fill(false);
    if (m_visibleCount > 0) {
        m_edits[0]->setFocus();
        m_edits[0]->selectAll();
    }
}

void ToolInputBar::onReturnPressed(int fieldIndex)
{
    // Enter advances to the next field; on the last field it commits. So the
    // user types Length, Enter -> Angle, types Angle, Enter -> draw.
    if (fieldIndex < m_visibleCount - 1) {
        m_edits[fieldIndex + 1]->setFocus();
        m_edits[fieldIndex + 1]->selectAll();
    }
    else {
        emitCommitted();
    }
}

void ToolInputBar::setFields(const QList<cad::CadTool::InputField>& fields)
{
    m_visibleCount = qMin(static_cast<int>(fields.size()), kMaxFields);

    for (int i = 0; i < kMaxFields; ++i) {
        const bool visible = i < m_visibleCount;
        m_labels[i]->setVisible(visible);
        m_edits[i]->setVisible(visible);
        if (!visible) {
            continue;
        }
        const QString label = fields[i].label + QStringLiteral(":");
        if (m_labels[i]->text() != label) {
            m_labels[i]->setText(label);
            m_dirty[i] = false;  // a different parameter now — accept live values
        }
        // Keep showing the live value (even while focused) until the user edits
        // this field themselves.
        if (!m_dirty[i]) {
            m_edits[i]->setText(QString::number(fields[i].value, 'f', 3));
        }
    }
}

void ToolInputBar::emitCommitted()
{
    QVector<double> values;
    values.reserve(m_visibleCount);
    for (int i = 0; i < m_visibleCount; ++i) {
        values.append(m_edits[i]->text().toDouble());
    }
    emit committed(values);
}
