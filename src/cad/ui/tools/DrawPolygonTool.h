#pragma once

#include "cad/ui/tools/CadTool.h"

class QGraphicsScene;
class QGraphicsPathItem;
class QUndoStack;

namespace cad {

class CadDocument;

// Draws a regular polygon (n-gon): click the center, then click for the
// circumscribed radius (or type Sides + Radius). Built as a closed polyline.
class DrawPolygonTool : public CadTool
{
    Q_OBJECT

public:
    DrawPolygonTool(CadDocument* document, QUndoStack* undoStack,
                    QGraphicsScene* scene, QObject* parent = nullptr);

    void onMousePress(const QPointF& scenePos) override;
    void onMouseMove(const QPointF& scenePos) override;
    void onKeyPress(int key) override;
    void onCancel() override;
    void deactivate() override;

    QList<InputField> inputFields() const override;
    void applyInput(const QVector<double>& values) override;

private:
    QVector<QPointF> polygonPoints(double radius) const;
    void commitPolygon(double radius);
    void reset();
    void clearPreview();

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;

    bool m_hasCenter = false;
    int m_sides = 6;
    QPointF m_center;
    QPointF m_cursor;
    QGraphicsPathItem* m_preview = nullptr;
};

} // namespace cad
