#pragma once

#include <QPointF>

#include <optional>

namespace cad {

class CadDocument;

// The kind of point a snap landed on (drives the on-screen marker glyph).
enum class SnapType
{
    None,
    Endpoint,
    Midpoint,
    Center,
    Intersection,
    Perpendicular,
    Tangent,
    Grid
};

struct SnapResult
{
    bool hit = false;
    QPointF point;
    SnapType type = SnapType::None;
};

// Finds the significant point nearest the cursor so tools can lock onto exact
// geometry (line ends/mids, circle & arc centers, arc ends/mids, polyline
// vertices/segment mids, and crossings between entities). Pure domain logic:
// it reads the document and Qt value types only — no widgets, no scene. The view
// converts a pixel tolerance to scene units and draws the marker.
class SnapEngine
{
public:
    explicit SnapEngine(const CadDocument* document);

    // Nearest snap point to `query` within `tolerance` scene units (millimeters),
    // or a miss. `gridStep` > 0 enables a grid-point fallback (used only when no
    // object snap is found). `reference` is the active tool's anchor, needed by
    // the perpendicular / tangent snaps (empty = those are skipped). `ignoreId`
    // skips one entity (e.g. the one being edited), or -1.
    SnapResult snap(const QPointF& query, double tolerance, double gridStep = 0.0,
                    const std::optional<QPointF>& reference = std::nullopt,
                    int ignoreId = -1) const;

    // Which snap kinds to consider (wired to the UI toggles).
    bool endpoints = true;
    bool midpoints = true;
    bool centers = true;
    bool intersections = true;
    bool perpendicular = true;
    bool tangent = true;
    bool grid = true;

private:
    const CadDocument* m_document;
};

} // namespace cad
