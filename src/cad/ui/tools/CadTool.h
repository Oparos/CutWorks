#pragma once

#include <QList>
#include <QObject>
#include <QPointF>
#include <QString>
#include <QVector>

#include <optional>

namespace cad {

// Base class for interactive drawing/editing tools. A tool reacts to mouse and
// keyboard events (in scene coordinates: millimeters, Y up) and, when it has
// enough information, pushes an undo command that changes the document.
//
// Parametric input: a tool can expose named numeric fields (e.g. Length, Angle)
// that the UI shows in an input bar. It updates them live via inputChanged(),
// and the UI calls applyInput() when the user types values and presses Enter.
class CadTool : public QObject
{
    Q_OBJECT

public:
    struct InputField
    {
        QString label;
        double value = 0.0;
    };

    using QObject::QObject;

    virtual void onMousePress(const QPointF& scenePos) {}
    virtual void onMouseMove(const QPointF& scenePos) {}
    virtual void onMouseRelease(const QPointF& scenePos) {}
    virtual void onKeyPress(int key) {}

    // End the current in-progress operation (e.g. right-click), keeping the tool
    // active for a fresh start.
    virtual void onCancel() {}

    // Called when the tool is deactivated, so it can clear any preview.
    virtual void deactivate() {}

    // Empty list = the tool has no parametric input right now (hide the bar).
    virtual QList<InputField> inputFields() const { return {}; }

    // Commit typed values (same order as inputFields()).
    virtual void applyInput(const QVector<double>& values) {}

    // The anchor that the perpendicular / tangent snaps measure from (e.g. a
    // line's start point while its end is being placed). Empty = no such point.
    virtual std::optional<QPointF> referencePoint() const { return std::nullopt; }

signals:
    void inputChanged();       // parametric fields changed (e.g. mouse moved)
    void requestInputFocus();  // ask the UI to move focus into the input bar now
};

} // namespace cad
