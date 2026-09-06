#pragma once

#include "cad/ui/tools/CadTool.h"

class QUndoStack;

namespace cad {

class CadDocument;

// Places single points. Click to drop a point at the cursor, or type absolute
// X / Y and press Enter. Stays active to place several points.
class DrawPointTool : public CadTool
{
    Q_OBJECT

public:
    DrawPointTool(CadDocument* document, QUndoStack* undoStack, QObject* parent = nullptr);

    void onMouseMove(const QPointF& scenePos) override;
    void onMousePress(const QPointF& scenePos) override;

    QList<InputField> inputFields() const override;
    void applyInput(const QVector<double>& values) override;

private:
    void commitPoint(const QPointF& pos);

    CadDocument* m_document;
    QUndoStack* m_undoStack;
    QPointF m_cursor;
};

} // namespace cad
