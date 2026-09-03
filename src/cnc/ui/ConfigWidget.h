#pragma once

#include <QWidget>

class QTableWidget;
class QTableWidgetItem;
class QPushButton;

namespace cnc {
struct GrblSetting;
}

// Table editor for GRBL settings ($$). Reads settings from the machine (rows are
// added as they arrive), lets the user edit values, and sends back only the ones
// that changed. Presentation only: no protocol logic here.
class ConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigWidget(QWidget* parent = nullptr);

    void clearSettings();
    void addSetting(const cnc::GrblSetting& setting);

signals:
    void readRequested();
    void saveRequested(const QStringList& commands);  // e.g. {"$110=1000", ...}

private:
    void onSave();
    void onItemChanged(QTableWidgetItem* item);

    QTableWidget* m_table = nullptr;
    QPushButton* m_readButton = nullptr;
    QPushButton* m_saveButton = nullptr;
};
