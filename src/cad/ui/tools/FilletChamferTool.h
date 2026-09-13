#pragma once

#include "cad/ui/tools/CadTool.h"

#include <QPointF>

class QGraphicsScene;
class QGraphicsPathItem;
class QPainterPath;
class QUndoStack;

namespace cad {

class CadDocument;

// Rounds (fillet) or bevels (chamfer) a corner. Two cases, matching how CAD
// users expect it to work:
//   * two adjacent segments of the same polyline (e.g. a rectangle corner) — the
//     round/bevel is inserted into that polyline, which stays one entity;
//   * two separate straight lines — both are trimmed to the corner and joined by
//     an arc (fillet) or a short line (chamfer).
// The radius / setback is a sticky value shown in the input bar (type it any
// time). Value 0 just cleans the corner to a sharp meet. One class, two modes
// (two toolbar buttons). Esc / right-click starts over.
class FilletChamferTool : public CadTool
{
    Q_OBJECT

public:
    enum class Mode
    {
        Fillet,
        Chamfer
    };

    // A picked corner arm: an entity and, for a polyline, which segment; plus the
    // segment's endpoints and where the user clicked (the side to keep).
    struct Pick
    {
        int id = -1;
        int segment = -1;  // polyline segment index, or -1 for a plain line
        bool isPolyline = false;
        QPointF a;
        QPointF b;
        QPointF click;
    };

    FilletChamferTool(Mode mode, CadDocument* document, QUndoStack* undoStack,
                      QGraphicsScene* scene, QObject* parent = nullptr);

    void onMouseMove(const QPointF& scenePos) override;
    void onMousePress(const QPointF& scenePos) override;
    void onKeyPress(int key) override;
    void onCancel() override;
    void deactivate() override;

    QList<InputField> inputFields() const override;
    void applyInput(const QVector<double>& values) override;

private:
    Pick pickArm(const QPointF& scenePos) const;
    void showPreview(const QPainterPath& path);
    void clearPreview();
    void reset();

    Mode m_mode;
    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QGraphicsScene* m_scene;
    double m_value = 5.0;  // radius (fillet) or setback distance (chamfer)
    Pick m_first;
    QGraphicsPathItem* m_preview = nullptr;
};

} // namespace cad
