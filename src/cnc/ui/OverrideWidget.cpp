#include "cnc/ui/OverrideWidget.h"

#include <QHBoxLayout>
#include <QPushButton>

OverrideWidget::OverrideWidget(QWidget* parent)
    : QGroupBox(tr("Feed Override (on the fly)"), parent)
{
    auto* layout = new QHBoxLayout(this);

    auto addButton = [this, layout](const QString& text, cnc::FeedOverride action,
                                    const QString& objectName = QString()) {
        auto* button = new QPushButton(text, this);
        button->setFocusPolicy(Qt::NoFocus);  // keep arrow keys for jogging
        if (!objectName.isEmpty()) {
            button->setObjectName(objectName);
        }
        connect(button, &QPushButton::clicked, this,
                [this, action]() { emit overrideRequested(action); });
        layout->addWidget(button);
    };

    addButton(QStringLiteral("-10%"), cnc::FeedOverride::Minus10);
    addButton(QStringLiteral("-1%"), cnc::FeedOverride::Minus1);
    addButton(QStringLiteral("100%"), cnc::FeedOverride::Reset, QStringLiteral("overrideReset"));
    addButton(QStringLiteral("+1%"), cnc::FeedOverride::Plus1);
    addButton(QStringLiteral("+10%"), cnc::FeedOverride::Plus10);
}
