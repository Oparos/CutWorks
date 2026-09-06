#pragma once

#include "cad/ui/tools/CadTool.h"

#include <QList>
#include <QVector>
#include <QWidget>

#include <array>

class QLabel;
class QLineEdit;

// A small parametric-input bar for the active tool. It shows one labeled field
// per value the tool exposes and keeps them updated live (without stealing what
// the user is currently typing). Pressing Enter commits all field values back.
class ToolInputBar : public QWidget
{
    Q_OBJECT

public:
    explicit ToolInputBar(QWidget* parent = nullptr);

    // Set the fields to show (labels + current values). Values of a field the
    // user is editing are left untouched.
    void setFields(const QList<cad::CadTool::InputField>& fields);

    // Move keyboard focus into the first field (used when the user presses Tab).
    void focusFirstField();

signals:
    void committed(const QVector<double>& values);

private:
    void onReturnPressed(int fieldIndex);
    void emitCommitted();

    static constexpr int kMaxFields = 4;
    std::array<QLabel*, kMaxFields> m_labels{};
    std::array<QLineEdit*, kMaxFields> m_edits{};
    int m_visibleCount = 0;
};
