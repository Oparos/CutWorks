#pragma once

#include <QColor>
#include <QObject>
#include <QString>

#include <vector>

namespace cad {

// One drawing layer: a name, a display color, and whether it is shown. This maps
// directly onto the DXF LAYER table.
struct Layer
{
    QString name;
    QColor color = QColor(0xe6, 0xe6, 0xe6);
    bool visible = true;
};

// The drawing's set of layers. Every entity names the layer it belongs to; the
// layer supplies its color and visibility. There is always a default layer "0"
// and always exactly one active layer (new entities inherit it). Domain object:
// a QObject for change signals, but with no widget dependency. Owned by
// CadDocument, so it travels with the drawing (and with a DXF load in CAM).
class LayerTable : public QObject
{
    Q_OBJECT

public:
    explicit LayerTable(QObject* parent = nullptr);

    // Create the layer if new; no-op if it already exists. Empty names ignored.
    void addLayer(const QString& name);
    // Remove a layer. Refuses to remove "0" or the last remaining layer. Callers
    // must first move any entities off it (the document does this). If it was the
    // active layer, "0" becomes active.
    void removeLayer(const QString& name);
    void setColor(const QString& name, const QColor& color);
    void setVisible(const QString& name, bool visible);

    const Layer* layer(const QString& name) const;  // nullptr if unknown
    QColor colorOf(const QString& name) const;       // layer color, or a safe default
    bool isVisible(const QString& name) const;       // true if unknown (never hide by mistake)
    const std::vector<Layer>& layers() const { return m_layers; }

    QString activeName() const { return m_active; }
    void setActive(const QString& name);

signals:
    void layersChanged();                     // a layer was added/removed
    void layerChanged(const QString& name);   // a layer's color or visibility changed
    void activeChanged(const QString& name);  // the active layer changed

private:
    Layer* find(const QString& name);
    const Layer* find(const QString& name) const;

    std::vector<Layer> m_layers;
    QString m_active;
};

} // namespace cad
