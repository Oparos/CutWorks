#include "cnc/ui/ConfigWidget.h"

#include "cnc/core/grbl/GrblSettings.h"
#include "cnc/core/grbl/GrblTypes.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
enum Column { CodeCol = 0, ValueCol = 1, DescCol = 2 };

// Dark-theme highlight for an edited-but-unsaved value cell.
const QColor kChangedBg(74, 58, 32);  // muted amber
}

ConfigWidget::ConfigWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({tr("Code ($)"), tr("Value"), tr("Description")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);

    m_readButton = new QPushButton(tr("Read settings ($$)"), this);
    m_saveButton = new QPushButton(tr("Save changes"), this);

    auto* buttons = new QHBoxLayout();
    buttons->addWidget(m_readButton);
    buttons->addWidget(m_saveButton);
    buttons->addStretch();

    layout->addWidget(m_table);
    layout->addLayout(buttons);

    connect(m_readButton, &QPushButton::clicked, this, &ConfigWidget::readRequested);
    connect(m_saveButton, &QPushButton::clicked, this, &ConfigWidget::onSave);
    connect(m_table, &QTableWidget::itemChanged, this, &ConfigWidget::onItemChanged);
}

void ConfigWidget::clearSettings()
{
    m_table->setRowCount(0);
}

void ConfigWidget::addSetting(const cnc::GrblSetting& setting)
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);

    auto* codeItem = new QTableWidgetItem(setting.code);
    codeItem->setFlags(codeItem->flags() & ~Qt::ItemIsEditable);
    m_table->setItem(row, CodeCol, codeItem);

    auto* valueItem = new QTableWidgetItem(setting.value);
    valueItem->setData(Qt::UserRole, setting.value);  // remember the original
    m_table->setItem(row, ValueCol, valueItem);

    // Machines usually report settings without a description, so fall back to
    // our built-in dictionary.
    const QString desc = setting.description.isEmpty()
                             ? cnc::grblSettingDescription(setting.code)
                             : setting.description;
    auto* descItem = new QTableWidgetItem(desc);
    descItem->setFlags(descItem->flags() & ~Qt::ItemIsEditable);
    descItem->setForeground(QColor(0x9a, 0x9a, 0x9a));
    m_table->setItem(row, DescCol, descItem);
}

void ConfigWidget::onItemChanged(QTableWidgetItem* item)
{
    if (item->column() != ValueCol) {
        return;
    }
    const bool changed = item->text() != item->data(Qt::UserRole).toString();
    item->setBackground(changed ? QBrush(kChangedBg) : QBrush());
}

void ConfigWidget::onSave()
{
    QStringList commands;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        QTableWidgetItem* valueItem = m_table->item(row, ValueCol);
        const QString value = valueItem->text();
        if (value != valueItem->data(Qt::UserRole).toString()) {
            commands << (m_table->item(row, CodeCol)->text() + "=" + value);
            valueItem->setData(Qt::UserRole, value);  // this value is now the baseline
            valueItem->setBackground(QBrush());
        }
    }

    if (!commands.isEmpty()) {
        emit saveRequested(commands);
    }
}
