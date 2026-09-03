#include "cnc/core/gcode/GCodeParser.h"

#include <QRegularExpression>

#include <algorithm>
#include <optional>

namespace cnc {

namespace {

// Modal state carried across lines while parsing a program.
struct ModalState
{
    int motionMode = 0;   // 0=G0, 1=G1, 2=G2, 3=G3
    bool absolute = true;  // G90 / G91
    double feed = 0.0;     // mm/min
    Point2D pos;
    double z = 0.0;
};

QString stripComments(QString line)
{
    const int semicolon = line.indexOf(';');
    if (semicolon != -1) {
        line = line.left(semicolon);
    }
    static const QRegularExpression parens(QStringLiteral("\\([^)]*\\)"));
    line.remove(parens);
    return line;
}

} // namespace

Toolpath GCodeParser::parse(const QStringList& lines) const
{
    Toolpath path;
    ModalState state;

    static const QRegularExpression wordRe(QStringLiteral("([A-Z])\\s*([+-]?\\d*\\.?\\d+)"));

    for (const QString& raw : lines) {
        const QString line = stripComments(raw.toUpper());
        if (line.trimmed().isEmpty()) {
            continue;
        }

        std::optional<double> wordX, wordY, wordZ, wordI, wordJ;
        bool sawAxis = false;

        auto it = wordRe.globalMatch(line);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            const char letter = match.captured(1).at(0).toLatin1();
            const double value = match.captured(2).toDouble();

            switch (letter) {
            case 'G': {
                const int g = static_cast<int>(value);
                if (g >= 0 && g <= 3) {
                    state.motionMode = g;
                }
                else if (g == 90) {
                    state.absolute = true;
                }
                else if (g == 91) {
                    state.absolute = false;
                }
                break;
            }
            case 'X': wordX = value; sawAxis = true; break;
            case 'Y': wordY = value; sawAxis = true; break;
            case 'Z': wordZ = value; sawAxis = true; break;
            case 'I': wordI = value; break;
            case 'J': wordJ = value; break;
            case 'F': state.feed = value; break;
            default:  break;  // A/B/C and others: accepted, not used for the 2D path
            }
        }

        if (!sawAxis) {
            continue;  // no motion on this line (e.g. a bare M-code)
        }

        // Resolve the target, honoring absolute/incremental mode.
        Point2D target = state.pos;
        double targetZ = state.z;
        if (wordX) target.x = state.absolute ? *wordX : state.pos.x + *wordX;
        if (wordY) target.y = state.absolute ? *wordY : state.pos.y + *wordY;
        if (wordZ) targetZ = state.absolute ? *wordZ : state.z + *wordZ;

        PathSegment seg;
        seg.start = state.pos;
        seg.end = target;
        seg.feed = state.feed;
        seg.move = (state.motionMode == 0) ? MoveType::Rapid : MoveType::Cut;

        if (state.motionMode == 2 || state.motionMode == 3) {
            if (wordI || wordJ) {
                seg.shape = SegmentShape::Arc;
                seg.clockwise = (state.motionMode == 2);
                seg.center = {state.pos.x + (wordI ? *wordI : 0.0),
                              state.pos.y + (wordJ ? *wordJ : 0.0)};
            }
            else {
                seg.shape = SegmentShape::Line;  // R-form arcs drawn as chord
            }
        }
        else {
            seg.shape = SegmentShape::Line;
        }

        const bool movedInPlane = (target.x != state.pos.x) || (target.y != state.pos.y);
        if (seg.shape == SegmentShape::Line && !movedInPlane) {
            // Pure Z move (torch up/down) — advance position but draw nothing.
            state.pos = target;
            state.z = targetZ;
            continue;
        }

        path.push_back(seg);
        state.pos = target;
        state.z = targetZ;
    }

    return path;
}

GCodeModalState GCodeParser::modalStateBefore(const QStringList& lines, int index) const
{
    GCodeModalState state;
    static const QRegularExpression wordRe(QStringLiteral("([A-Z])\\s*([+-]?\\d*\\.?\\d+)"));

    const int end = std::min(index, static_cast<int>(lines.size()));
    for (int li = 0; li < end; ++li) {
        const QString line = stripComments(lines.at(li).toUpper());
        if (line.trimmed().isEmpty()) {
            continue;
        }

        std::optional<double> wordX, wordY, wordZ;

        auto it = wordRe.globalMatch(line);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            const char letter = match.captured(1).at(0).toLatin1();
            const double value = match.captured(2).toDouble();

            switch (letter) {
            case 'G': {
                const int g = static_cast<int>(value);
                if (g == 90) state.absolute = true;
                else if (g == 91) state.absolute = false;
                break;
            }
            case 'M': {
                const int m = static_cast<int>(value);
                if (m == 3 || m == 4 || m == 5) {
                    state.torch = m;
                }
                break;
            }
            case 'X': wordX = value; break;
            case 'Y': wordY = value; break;
            case 'Z': wordZ = value; break;
            case 'F': state.feed = value; break;
            default:  break;
            }
        }

        if (wordX) { state.pos.x = state.absolute ? *wordX : state.pos.x + *wordX; state.hasXY = true; }
        if (wordY) { state.pos.y = state.absolute ? *wordY : state.pos.y + *wordY; state.hasXY = true; }
        if (wordZ) { state.z = state.absolute ? *wordZ : state.z + *wordZ; state.hasZ = true; }
    }

    return state;
}

} // namespace cnc
