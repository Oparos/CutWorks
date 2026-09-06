#include "cad/ui/tools/DrawPointTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/entities/PointEntity.h"

#include <QUndoStack>

namespace cad {

DrawPointTool::DrawPointTool(CadDocument* document, QUndoStack* undoStack, QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
{
}

void DrawPointTool::onMouseMove(const QPointF& scenePos)
{
    m_cursor = scenePos;
    emit inputChanged();  // keep the X / Y fields showing the cursor position
}

void DrawPointTool::onMousePress(const QPointF& scenePos)
{
    emit inputChanged();  // keep the X / Y fields showing the cursor position
    emit requestInputFocus();  // let the user type X / Y immediately
    commitPoint(scenePos);
}

QList<CadTool::InputField> DrawPointTool::inputFields() const
{
    // Absolute coordinates — note this is a different field set than the line or
    // circle tools, which is exactly how the input bar knows what to show.
    return {{tr("X"), m_cursor.x()}, {tr("Y"), m_cursor.y()}};
}

void DrawPointTool::applyInput(const QVector<double>& values)
{
    if (values.size() >= 2) {
        commitPoint(QPointF(values[0], values[1]));
    }
}

void DrawPointTool::commitPoint(const QPointF& pos)
{
    m_undoStack->push(new AddEntityCommand(
        m_document, std::make_unique<PointEntity>(pos), tr("Add point")));
    emit inputChanged();  // keep the X / Y fields showing the cursor position
    emit requestInputFocus();  // keep typing for the next point in the chain
}

} // namespace cad
