#pragma once

#include "cnc/core/gcode/Toolpath.h"

#include <QStringList>

namespace cnc {

// The state needed to safely resume a job from the middle: where the tool was,
// the active feed, distance mode, and whether the torch was on. Reconstructed by
// scanning the program up to a line.
struct GCodeModalState
{
    Point2D pos;              // last commanded XY (absolute)
    bool hasXY = false;
    double z = 0.0;
    bool hasZ = false;
    double feed = 0.0;
    bool absolute = true;     // G90 / G91
    int torch = 5;            // 3 = M3, 4 = M4, 5 = M5 (off)
};

// Turns a G-code program into a 2D toolpath for preview and estimation.
//
// It tracks modal state properly (motion mode G0-G3, absolute/incremental
// G90/G91, feed) instead of treating each line independently. Any axis word is
// tolerated (X/Y/Z/A/...), but only X/Y build the drawn path and Z is followed
// to know the height; extra axes are accepted and ignored geometrically.
//
// Arcs use I/J center offsets (the form plasma post-processors emit); an arc
// given only R is currently drawn as its chord.
class GCodeParser
{
public:
    Toolpath parse(const QStringList& lines) const;

    // Modal state as it would be just before executing line `index`. Used to
    // build a catch-up block when resuming a job from the middle.
    GCodeModalState modalStateBefore(const QStringList& lines, int index) const;
};

} // namespace cnc
