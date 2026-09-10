#include "cad/core/geometry/Intersections.h"

#include "cad/core/entities/ArcEntity.h"
#include "cad/core/entities/CircleEntity.h"
#include "cad/core/entities/LineEntity.h"
#include "cad/core/entities/PolylineEntity.h"

#include <QLineF>

#include <algorithm>
#include <cmath>
#include <variant>
#include <vector>

namespace cad {
namespace geom {

// File-local constants (constexpr at namespace scope has internal linkage).
constexpr double kPi = 3.14159265358979323846;
constexpr double kRadToDeg = 180.0 / kPi;
constexpr double kDenEps = 1e-9;      // parallel-line determinant threshold
constexpr double kParamEps = 1e-6;    // tolerance on a segment parameter (0..1)
constexpr double kMergeEps = 1e-6;    // near-duplicate point distance (mm)
constexpr double kAngleEps = 1e-6;    // tolerance on an angle (deg)

double normalizeDeg(double deg)
{
    double d = std::fmod(deg, 360.0);
    if (d < 0.0) {
        d += 360.0;
    }
    return d;
}

double angleAtDeg(const QPointF& center, const QPointF& p)
{
    return normalizeDeg(std::atan2(p.y() - center.y(), p.x() - center.x()) * kRadToDeg);
}

bool angleWithinSweep(double angleDeg, double startDeg, double sweepDeg)
{
    const double span = std::abs(sweepDeg);
    if (span >= 360.0 - kAngleEps) {
        return true;  // full circle
    }
    const double sign = (sweepDeg >= 0.0) ? 1.0 : -1.0;
    const double theta = normalizeDeg(sign * (angleDeg - startDeg));
    return theta <= span + kAngleEps;
}

namespace {

// A straight edge. If !bounded it stands for the infinite line through a/b.
struct Seg
{
    QPointF a;
    QPointF b;
    bool bounded;
};

// A circular edge: an arc of `sweep` degrees from `start` (CCW +). A full circle
// is represented with sweep == 360.
struct Circ
{
    QPointF c;
    double r;
    double start;
    double sweep;
};

using Primitive = std::variant<Seg, Circ>;

QVector<QPointF> segSeg(const Seg& s1, const Seg& s2)
{
    const double x1 = s1.a.x(), y1 = s1.a.y(), x2 = s1.b.x(), y2 = s1.b.y();
    const double x3 = s2.a.x(), y3 = s2.a.y(), x4 = s2.b.x(), y4 = s2.b.y();

    const double den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (std::abs(den) < kDenEps) {
        return {};  // parallel or collinear: no single crossing point
    }

    const double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / den;
    const double u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / den;
    if (s1.bounded && (t < -kParamEps || t > 1.0 + kParamEps)) {
        return {};
    }
    if (s2.bounded && (u < -kParamEps || u > 1.0 + kParamEps)) {
        return {};
    }
    return {QPointF(x1 + t * (x2 - x1), y1 + t * (y2 - y1))};
}

QVector<QPointF> segCirc(const Seg& s, const Circ& c)
{
    QVector<QPointF> out;
    const double dx = s.b.x() - s.a.x();
    const double dy = s.b.y() - s.a.y();
    const double A = dx * dx + dy * dy;
    if (A < kDenEps) {
        return out;  // degenerate segment
    }

    const double fx = s.a.x() - c.c.x();
    const double fy = s.a.y() - c.c.y();
    const double B = 2.0 * (dx * fx + dy * fy);
    const double C = fx * fx + fy * fy - c.r * c.r;

    double disc = B * B - 4.0 * A * C;
    if (disc < -kDenEps) {
        return out;  // line misses the circle
    }
    disc = std::max(disc, 0.0);
    const double sq = std::sqrt(disc);
    const double ts[2] = {(-B - sq) / (2.0 * A), (-B + sq) / (2.0 * A)};
    const int count = (sq < kDenEps) ? 1 : 2;

    for (int i = 0; i < count; ++i) {
        const double t = ts[i];
        if (s.bounded && (t < -kParamEps || t > 1.0 + kParamEps)) {
            continue;
        }
        const QPointF p(s.a.x() + t * dx, s.a.y() + t * dy);
        if (angleWithinSweep(angleAtDeg(c.c, p), c.start, c.sweep)) {
            out.append(p);
        }
    }
    return out;
}

QVector<QPointF> circCirc(const Circ& c1, const Circ& c2)
{
    QVector<QPointF> out;
    const double dx = c2.c.x() - c1.c.x();
    const double dy = c2.c.y() - c1.c.y();
    const double d = std::hypot(dx, dy);
    if (d < kDenEps) {
        return out;  // concentric (or identical): no isolated points
    }
    if (d > c1.r + c2.r + kMergeEps) {
        return out;  // too far apart
    }
    if (d < std::abs(c1.r - c2.r) - kMergeEps) {
        return out;  // one circle strictly inside the other
    }

    const double a = (c1.r * c1.r - c2.r * c2.r + d * d) / (2.0 * d);
    double h2 = c1.r * c1.r - a * a;
    h2 = std::max(h2, 0.0);
    const double h = std::sqrt(h2);

    const double xm = c1.c.x() + a * dx / d;
    const double ym = c1.c.y() + a * dy / d;
    const QPointF candidates[2] = {QPointF(xm + h * dy / d, ym - h * dx / d),
                                   QPointF(xm - h * dy / d, ym + h * dx / d)};
    const int count = (h < kDenEps) ? 1 : 2;

    for (int i = 0; i < count; ++i) {
        const QPointF& p = candidates[i];
        if (angleWithinSweep(angleAtDeg(c1.c, p), c1.start, c1.sweep)
            && angleWithinSweep(angleAtDeg(c2.c, p), c2.start, c2.sweep)) {
            out.append(p);
        }
    }
    return out;
}

QVector<QPointF> intersectPrim(const Primitive& p, const Primitive& q)
{
    if (const auto* s1 = std::get_if<Seg>(&p)) {
        if (const auto* s2 = std::get_if<Seg>(&q)) {
            return segSeg(*s1, *s2);
        }
        return segCirc(*s1, std::get<Circ>(q));
    }
    const Circ& c1 = std::get<Circ>(p);
    if (const auto* s2 = std::get_if<Seg>(&q)) {
        return segCirc(*s2, c1);
    }
    return circCirc(c1, std::get<Circ>(q));
}

// Break an entity into the primitive edges it is drawn from.
std::vector<Primitive> decompose(const CadEntity& e)
{
    std::vector<Primitive> parts;
    switch (e.type()) {
    case EntityType::Line: {
        const auto& l = static_cast<const LineEntity&>(e);
        parts.push_back(Seg{l.p1(), l.p2(), true});
        break;
    }
    case EntityType::Circle: {
        const auto& c = static_cast<const CircleEntity&>(e);
        parts.push_back(Circ{c.center(), c.radius(), 0.0, 360.0});
        break;
    }
    case EntityType::Arc: {
        const auto& a = static_cast<const ArcEntity&>(e);
        parts.push_back(Circ{a.center(), a.radius(), a.startAngle(), a.sweepAngle()});
        break;
    }
    case EntityType::Polyline: {
        const auto& poly = static_cast<const PolylineEntity&>(e);
        const auto& vs = poly.vertices();
        const int n = vs.size();
        const int segs = poly.isClosed() ? n : n - 1;
        for (int i = 0; i < segs; ++i) {
            parts.push_back(Seg{vs[i].pos, vs[(i + 1) % n].pos, true});
        }
        break;
    }
    case EntityType::Point:
        break;  // no crossable geometry
    }
    return parts;
}

void appendMerged(QVector<QPointF>& out, const QVector<QPointF>& pts)
{
    for (const QPointF& p : pts) {
        const bool dup = std::any_of(out.begin(), out.end(), [&](const QPointF& q) {
            return QLineF(p, q).length() < kMergeEps;
        });
        if (!dup) {
            out.append(p);
        }
    }
}

QVector<QPointF> intersectSupport(const Primitive& support, const CadEntity& e)
{
    QVector<QPointF> out;
    for (const Primitive& pr : decompose(e)) {
        appendMerged(out, intersectPrim(support, pr));
    }
    return out;
}

} // namespace

QVector<QPointF> intersect(const CadEntity& a, const CadEntity& b)
{
    QVector<QPointF> out;
    for (const Primitive& pa : decompose(a)) {
        for (const Primitive& pb : decompose(b)) {
            appendMerged(out, intersectPrim(pa, pb));
        }
    }
    return out;
}

QVector<QPointF> supportLineVsEntity(const QPointF& p1, const QPointF& p2, const CadEntity& e)
{
    return intersectSupport(Seg{p1, p2, false}, e);
}

QVector<QPointF> supportCircleVsEntity(const QPointF& center, double radius, const CadEntity& e)
{
    return intersectSupport(Circ{center, radius, 0.0, 360.0}, e);
}

} // namespace geom
} // namespace cad
