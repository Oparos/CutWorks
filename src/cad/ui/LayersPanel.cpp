#include "cad/ui/LayersPanel.h"

#include "cad/core/CadDocument.h"
#include "cad/core/entities/CadEntity.h"
#include "cad/core/layer/LayerTable.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

LayersPanel::LayersPanel(cad::CadDocument* document, QWidget* parent)
    : QWidget(parent)
    , m_document(document)
    , m_list(new QListWidget(this))
{
    m_list->setFocusPolicy(Qt::NoFocus);

    auto* addButton = new QPushButton(tr("Add"), this);
    auto* deleteButton = new QPushButton(tr("Delete"), this);
    addButton->setFocusPolicy(Qt::NoFocus);
    deleteButton->setFocusPolicy(Qt::NoFocus);
    connect(addButton, &QPushButton::clicked, this, &LayersPanel::addLayer);
    connect(deleteButton, &QPushButton::clicked, this, &LayersPanel::deleteActiveLayer);

    auto* buttons = new QHBoxLayout();
    buttons->addWidget(addButton);
    buttons->addWidget(deleteButton);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(new QLabel(tr("Layers"), this));
    layout->addWidget(m_list, 1);
    layout->addLayout(buttons);
    setMaximumWidth(220);

    // Rebuild whenever the table changes.
    cad::LayerTable& layers = m_document->layers();
    connect(&layers, &cad::LayerTable::layersChanged, this, &LayersPanel::rebuild);
    connect(&layers, &cad::LayerTable::layerChanged, this, &LayersPanel::rebuild);
    connect(&layers, &cad::LayerTable::activeChanged, this, &LayersPanel::rebuild);

    rebuild();
}

void LayersPanel::rebuild()
{
    m_list->clear();
    cad::LayerTable& layers = m_document->layers();
    const QString active = layers.activeName();

    for (const cad::Layer& layer : layers.layers()) {
        const QString name = layer.name;

        auto* row = new QWidget();
        auto* hbox = new QHBoxLayout(row);
        hbox->setContentsMargins(2, 1, 2, 1);

        auto* visible = new QCheckBox(row);
        visible->setChecked(layer.visible);
        visible->setFocusPolicy(Qt::NoFocus);
        visible->setToolTip(tr("Visible"));
        connect(visible, &QCheckBox::toggled, this,
                [this, name](bool on) { m_document->layers().setVisible(name, on); });

        auto* swatch = new QPushButton(row);
        swatch->setFixedSize(18, 18);
        swatch->setFocusPolicy(Qt::NoFocus);
        swatch->setToolTip(tr("Color"));
        swatch->setStyleSheet(
            QStringLiteral("background:%1; border:1px solid #555;").arg(layer.color.name()));
        connect(swatch, &QPushButton::clicked, this, [this, name] {
            const QColor picked = QColorDialog::getColor(m_document->layers().colorOf(name), this,
                                                         tr("Layer color"));
            if (picked.isValid()) {
                m_document->layers().setColor(name, picked);
            }
        });

        auto* nameButton = new QPushButton(name, row);
        nameButton->setFlat(true);
        nameButton->setFocusPolicy(Qt::NoFocus);
        nameButton->setToolTip(tr("Set active"));
        nameButton->setStyleSheet(name == active ? QStringLiteral("text-align:left; font-weight:bold;")
                                                  : QStringLiteral("text-align:left;"));
        connect(nameButton, &QPushButton::clicked, this,
                [this, name] { m_document->layers().setActive(name); });

        hbox->addWidget(visible);
        hbox->addWidget(swatch);
        hbox->addWidget(nameButton, 1);

        auto* item = new QListWidgetItem(m_list);
        item->setSizeHint(row->sizeHint());
        m_list->setItemWidget(item, row);
    }
}

void LayersPanel::addLayer()
{
    cad::LayerTable& layers = m_document->layers();
    QString name;
    for (int i = 1;; ++i) {
        name = tr("Layer %1").arg(i);
        if (!layers.layer(name)) {
            break;
        }
    }
    layers.addLayer(name);
    layers.setActive(name);  // make the new layer active so drawing lands on it
}

void LayersPanel::deleteActiveLayer()
{
    cad::LayerTable& layers = m_document->layers();
    const QString name = layers.activeName();
    if (name == QStringLiteral("0")) {
        return;  // the default layer stays
    }
    // Move any entities off the layer before removing it, so nothing is orphaned.
    for (int id : m_document->entityIds()) {
        if (cad::CadEntity* e = m_document->entity(id); e && e->layer() == name) {
            e->setLayer(QStringLiteral("0"));
            m_document->notifyChanged(id);
        }
    }
    layers.removeLayer(name);
}
