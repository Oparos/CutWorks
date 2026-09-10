#include "cad/core/geometry/Tangents.h"

#include <algorithm>
#include <cmath>

namespace cad {
namespace geom {

namespace {

constexpr double kEps = 1e-9;

// Both families share the same construction: a normal direction on circle A at
// angle `base ± acos(ratio)`, where `ratio` is (rA - rB)/dist for external and
// (rA + rB)/dist for internal tangents. `sameSide` places circle B's touch
// point along the same normal (external) or the opposite one (internal).
QVector<QLineF> tangents(const QPointF& cA, double rA, const QPointF& cB, double rB,
                         double ratio, bool sameSide)
{
    QVector<QLineF> out;
    const double dx = cB.x() - cA.x();
    const double dy = cB.y() - cA.y();
    const double dist = std::hypot(dx, dy);
    if (dist < kEps || std::abs(ratio) > 1.0) {
        return out;
    }

    const double base = std::atan2(dy, dx);
    const double spread = std::acos(std::clamp(ratio, -1.0, 1.0));
    const double sideB = sameSide ? 1.0 : -1.0;

    for (const double sign : {1.0, -1.0}) {
        const double angle = base + sign * spread;
        const QPointF n(std::cos(angle), std::sin(angle));
        const QPointF pA(cA.x() + rA * n.x(), cA.y() + rA * n.y());
        const QPointF pB(cB.x() + sideB * rB * n.x(), cB.y() + sideB * rB * n.y());
        out.append(QLineF(pA, pB));
    }
    return out;
}

} // namespace

QVector<QLineF> externalTangents(const QPointF& cA, double rA, const QPointF& cB, double rB)
{
    const double dist = std::hypot(cB.x() - cA.x(), cB.y() - cA.y());
    if (dist < kEps) {
        return {};
    }
    return tangents(cA, rA, cB, rB, (rA - rB) / dist, /*sameSide=*/true);
}

QVector<QLineF> internalTangents(const QPointF& cA, double rA, const QPointF& cB, double rB)
{
    const double dist = std::hypot(cB.x() - cA.x(), cB.y() - cA.y());
    if (dist < kEps) {
        return {};
    }
    return tangents(cA, rA, cB, rB, (rA + rB) / dist, /*sameSide=*/false);
}

} // namespace geom
} // namespace cad
