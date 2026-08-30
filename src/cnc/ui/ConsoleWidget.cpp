#include "cnc/ui/ConsoleWidget.h"

#include <QLineEdit>
#include <QScrollBar>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

// Color for each log category. Kept here in the UI, not in the backend.
QString colorFor(cnc::LogCategory category)
{
    switch (category) {
    case cnc::LogCategory::Sent:     return QStringLiteral("#6c9bff");
    case cnc::LogCategory::Received: return QStringLiteral("#d4d4d4");
    case cnc::LogCategory::Info:     return QStringLiteral("#4ec9b0");
    case cnc::LogCategory::Warning:  return QStringLiteral("#ff9800");
    case cnc::LogCategory::Error:    return QStringLiteral("#ff5555");
    }
    return QStringLiteral("#d4d4d4");
}

} // namespace

ConsoleWidget::ConsoleWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_output = new QTextEdit(this);
    m_output->setObjectName(QStringLiteral("console"));
    m_output->setReadOnly(true);

    m_input = new QLineEdit(this);
    m_input->setObjectName(QStringLiteral("consoleInput"));
    m_input->setPlaceholderText(tr("Enter command (e.g. G0 X0 Y0 or $$) and press Enter..."));

    layout->addWidget(m_output);
    layout->addWidget(m_input);

    connect(m_input, &QLineEdit::returnPressed, this, &ConsoleWidget::onReturnPressed);
}

void ConsoleWidget::onReturnPressed()
{
    const QString command = m_input->text().trimmed();
    if (command.isEmpty()) {
        return;
    }
    emit commandEntered(command);
    m_input->clear();
}

void ConsoleWidget::appendMessage(const QString& text, cnc::LogCategory category)
{
    const QString prefix = (category == cnc::LogCategory::Sent) ? QStringLiteral("&gt; ") : QString();
    const QString html = QStringLiteral("<span style=\"color:%1;\">%2%3</span>")
                             .arg(colorFor(category), prefix, text.toHtmlEscaped());

    m_output->append(html);

    QScrollBar* bar = m_output->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void ConsoleWidget::clear()
{
    m_output->clear();
}
