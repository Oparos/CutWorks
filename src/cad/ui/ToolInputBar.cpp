#include "cad/ui/ToolInputBar.h"

#include <QHBoxLayout>
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
        connect(m_edits[i], &QLineEdit::returnPressed, this, [this, i]() { onReturnPressed(i); });
    }
    layout->addStretch();
}

void ToolInputBar::focusFirstField()
{
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
        m_labels[i]->setText(fields[i].label + QStringLiteral(":"));
        // Don't overwrite a value the user is in the middle of typing.
        if (!m_edits[i]->hasFocus()) {
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
