#include "cad/ui/tools/DrawLineTool.h"

#include "cad/core/CadDocument.h"
#include "cad/core/commands/AddEntityCommand.h"
#include "cad/core/entities/LineEntity.h"

#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QPen>
#include <QUndoStack>
#include <Qt>

#include <cmath>

namespace cad {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kRadToDeg = 180.0 / kPi;
constexpr double kDegToRad = kPi / 180.0;

QGraphicsLineItem* makePreview(QGraphicsScene* scene, const QPointF& at)
{
    QPen pen(QColor(0xff, 0x9c, 0x33));
    pen.setCosmetic(true);
    pen.setStyle(Qt::DashLine);
    return scene->addLine(QLineF(at, at), pen);
}
} // namespace

DrawLineTool::DrawLineTool(CadDocument* document, QUndoStack* undoStack,
                           QGraphicsScene* scene, QObject* parent)
    : CadTool(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_scene(scene)
{
}

void DrawLineTool::onMousePress(const QPointF& scenePos)
{
    if (!m_hasStart) {
        m_start = scenePos;
        m_cursor = scenePos;
        m_hasStart = true;
        m_preview = makePreview(m_scene, m_start);
        emit inputChanged();
        emit requestInputFocus();  // let the user type Length/Angle immediately
    }
    else {
        commitLine(scenePos);
    }
}

void DrawLineTool::onMouseMove(const QPointF& scenePos)
{
    if (!m_hasStart) {
        return;
    }
    m_cursor = scenePos;
    if (m_preview) {
        m_preview->setLine(QLineF(m_start, m_cursor));
    }
    emit inputChanged();
}

void DrawLineTool::onKeyPress(int key)
{
    if (key == Qt::Key_Escape) {
        cancelChain();
    }
}

void DrawLineTool::onCancel()
{
    cancelChain();
}

void DrawLineTool::cancelChain()
{
    clearPreview();
    m_hasStart = false;
    emit inputChanged();
}

void DrawLineTool::deactivate()
{
    clearPreview();
    m_hasStart = false;
}

QList<CadTool::InputField> DrawLineTool::inputFields() const
{
    if (!m_hasStart) {
        return {};
    }
    const double dx = m_cursor.x() - m_start.x();
    const double dy = m_cursor.y() - m_start.y();
    const double length = std::hypot(dx, dy);
    const double angle = std::atan2(dy, dx) * kRadToDeg;
    return {{tr("Length"), length}, {tr("Angle"), angle}};
}

void DrawLineTool::applyInput(const QVector<double>& values)
{
    if (!m_hasStart || values.size() < 2) {
        return;
    }
    const double length = values[0];
    const double angle = values[1] * kDegToRad;
    const QPointF end = m_start + QPointF(length * std::cos(angle), length * std::sin(angle));
    commitLine(end);
}

void DrawLineTool::commitLine(const QPointF& end)
{
    m_undoStack->push(new AddEntityCommand(
        m_document, std::make_unique<LineEntity>(m_start, end), tr("Draw line")));

    clearPreview();
    // Chain: the end becomes the start of the next segment.
    m_start = end;
    m_cursor = end;
    m_hasStart = true;
    m_preview = makePreview(m_scene, m_start);
    emit inputChanged();
    emit requestInputFocus();  // keep typing for the next segment in the chain
}

void DrawLineTool::clearPreview()
{
    if (m_preview) {
        m_scene->removeItem(m_preview);
        delete m_preview;
        m_preview = nullptr;
    }
}

} // namespace cad
