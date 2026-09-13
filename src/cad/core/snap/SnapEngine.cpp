#include "cad/core/snap/SnapEngine.h"

#include "cad/core/CadDocument.h"
#include "cad/core/entities/ArcEntity.h"
#include "cad/core/entities/CircleEntity.h"
#include "cad/core/entities/LineEntity.h"
#include "cad/core/entities/PolylineEntity.h"
#include "cad/core/geometry/Geometry.h"       // bulgeToArc
#include "cad/core/geometry/Intersections.h"  // intersect

#include <QLineF>

#include <cmath>
#include <functional>
#include <vector>

namespace cad {

namespace {

constexpr double kDegToRad = 0.017453292519943295;

QPointF pointOnCircle(const QPointF& center, double r, double angleDeg)
{
    return QPointF(center.x() + r * std::cos(angleDeg * kDegToRad),
                   center.y() + r * std::sin(angleDeg * kDegToRad));
}

} // namespace

SnapEngine::SnapEngine(const CadDocument* document)
    : m_document(document)
{
}

SnapResult SnapEngine::snap(const QPointF& query, double tolerance, double gridStep,
                            const std::optional<QPointF>& reference, int ignoreId) const
{
    SnapResult best;
    double bestDist = tolerance;

    const auto consider = [&](const QPointF& pt, SnapType type) {
        const double d = QLineF(query, pt).length();
        if (d < bestDist) {
            bestDist = d;
            best = {true, pt, type};
        }
    };

    const bool haveRef = reference.has_value();
    const QPointF ref = haveRef ? *reference : QPointF();

    // Foot of the perpendicular from the reference point onto a finite segment.
    const auto perpToSegment = [&](const QPointF& a, const QPointF& b) {
        if (!(perpendicular && haveRef)) {
            return;
        }
        const QPointF v = b - a;
        const QPointF w = ref - a;
        const double vv = v.x() * v.x() + v.y() * v.y();
        if (vv < 1e-9) {
            return;
        }
        const double t = (v.x() * w.x() + v.y() * w.y()) / vv;
        if (t >= 0.0 && t <= 1.0) {
            consider(QPointF(a.x() + t * v.x(), a.y() + t * v.y()), SnapType::Perpendicular);
        }
    };

    // Perpendicular (radial) and tangent points from the reference to a circle,
    // optionally restricted to an arc's sweep.
    const auto circlePerpTangent = [&](const QPointF& center, double r, bool arc, double start,
                                       double sweep) {
        const auto onArc = [&](const QPointF& p) {
            return !arc || geom::angleWithinSweep(geom::angleAtDeg(center, p), start, sweep);
        };
        const double dx = ref.x() - center.x();
        const double dy = ref.y() - center.y();
        const double d = std::hypot(dx, dy);
        if (perpendicular && haveRef && d > 1e-6) {
            const QPointF p(center.x() + r * dx / d, center.y() + r * dy / d);
            if (onArc(p)) {
                consider(p, SnapType::Perpendicular);
            }
        }
        if (tangent && haveRef && d > r + 1e-6) {
            const double ang = std::atan2(dy, dx);
            const double beta = std::acos(r / d);
            for (const double s : {ang + beta, ang - beta}) {
                const QPointF p(center.x() + r * std::cos(s), center.y() + r * std::sin(s));
                if (onArc(p)) {
                    consider(p, SnapType::Tangent);
                }
            }
        }
    };

    const auto addPoints = [&](const CadEntity& e) {
        switch (e.type()) {
        case EntityType::Line: {
            const auto& l = static_cast<const LineEntity&>(e);
            if (endpoints) {
                consider(l.p1(), SnapType::Endpoint);
                consider(l.p2(), SnapType::Endpoint);
            }
            if (midpoints) {
                consider((l.p1() + l.p2()) / 2.0, SnapType::Midpoint);
            }
            perpToSegment(l.p1(), l.p2());
            break;
        }
        case EntityType::Circle: {
            const auto& c = static_cast<const CircleEntity&>(e);
            if (centers) {
                consider(c.center(), SnapType::Center);
            }
            circlePerpTangent(c.center(), c.radius(), false, 0.0, 0.0);
            break;
        }
        case EntityType::Arc: {
            const auto& a = static_cast<const ArcEntity&>(e);
            if (centers) {
                consider(a.center(), SnapType::Center);
            }
            if (endpoints) {
                consider(pointOnCircle(a.center(), a.radius(), a.startAngle()), SnapType::Endpoint);
                consider(pointOnCircle(a.center(), a.radius(), a.startAngle() + a.sweepAngle()),
                         SnapType::Endpoint);
            }
            if (midpoints) {
                consider(pointOnCircle(a.center(), a.radius(), a.startAngle() + a.sweepAngle() / 2.0),
                         SnapType::Midpoint);
            }
            circlePerpTangent(a.center(), a.radius(), true, a.startAngle(), a.sweepAngle());
            break;
        }
        case EntityType::Polyline: {
            const auto& poly = static_cast<const PolylineEntity&>(e);
            const auto& vs = poly.vertices();
            const int n = vs.size();
            if (n == 0) {
                break;
            }
            if (endpoints) {
                for (const PolyVertex& v : vs) {
                    consider(v.pos, SnapType::Endpoint);
                }
            }
            const int segs = poly.isClosed() ? n : n - 1;
            for (int i = 0; i < segs; ++i) {
                const QPointF a = vs[i].pos;
                const QPointF b = vs[(i + 1) % n].pos;
                const BulgeArc arc = bulgeToArc(a, b, vs[i].bulge);
                if (arc.isArc) {
                    if (centers) {
                        consider(arc.center, SnapType::Center);
                    }
                    if (midpoints) {
                        consider(pointOnCircle(arc.center, arc.radius,
                                               arc.startAngleDeg + arc.sweepDeg / 2.0),
                                 SnapType::Midpoint);
                    }
                    circlePerpTangent(arc.center, arc.radius, true, arc.startAngleDeg, arc.sweepDeg);
                }
                else {
                    if (midpoints) {
                        consider((a + b) / 2.0, SnapType::Midpoint);
                    }
                    perpToSegment(a, b);
                }
            }
            break;
        }
        case EntityType::Point:
            if (endpoints) {
                consider(e.bounds().center(), SnapType::Endpoint);
            }
            break;
        }
    };

    // Only entities whose bounds reach the query matter — cheap spatial reject.
    // Perpendicular/tangent can land far from the query, so the reference point
    // must also be reachable for those to matter; the bounds check on the query
    // still bounds which entities we test, which is the intent (snap near cursor).
    std::vector<const CadEntity*> near;
    for (int id : m_document->entityIds()) {
        if (id == ignoreId) {
            continue;
        }
        const CadEntity* e = m_document->entity(id);
        if (!e) {
            continue;
        }
        if (!e->bounds().adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(query)) {
            continue;
        }
        near.push_back(e);
        addPoints(*e);
    }

    if (intersections) {
        for (std::size_t i = 0; i < near.size(); ++i) {
            for (std::size_t j = i + 1; j < near.size(); ++j) {
                for (const QPointF& pt : geom::intersect(*near[i], *near[j])) {
                    consider(pt, SnapType::Intersection);
                }
            }
        }
    }

    // Grid is a fallback: only when nothing on the geometry was close enough.
    if (grid && gridStep > 0.0 && !best.hit) {
        const QPointF gp(std::round(query.x() / gridStep) * gridStep,
                         std::round(query.y() / gridStep) * gridStep);
        if (QLineF(query, gp).length() < tolerance) {
            best = {true, gp, SnapType::Grid};
        }
    }

    return best;
}

} // namespace cad
