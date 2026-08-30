#pragma once

#include "cnc/core/grbl/GrblTypes.h"

#include <QWidget>

class QTextEdit;
class QLineEdit;

// Scrolling log plus a command input line. The backend hands it (text, category)
// pairs; this widget decides how each category looks. It knows no GRBL details.
class ConsoleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConsoleWidget(QWidget* parent = nullptr);

    void appendMessage(const QString& text, cnc::LogCategory category);
    void clear();

signals:
    void commandEntered(const QString& command);

private:
    void onReturnPressed();

    QTextEdit* m_output = nullptr;
    QLineEdit* m_input = nullptr;
};
