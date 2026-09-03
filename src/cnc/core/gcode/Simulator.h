#pragma once

#include "cnc/core/gcode/Toolpath.h"

#include <QRectF>

namespace cnc {

struct JobEstimate
{
    double seconds = 0.0;  // estimated run time
    QRectF bounds;         // XY bounding box of the path (mm)
};

// Estimate run time and bounding box for a toolpath. Rapids use rapidRateMmMin;
// cuts use each segment's feed (falling back to fallbackFeedMmMin when a program
// never set F).
JobEstimate estimateJob(const Toolpath& path,
                        double rapidRateMmMin,
                        double fallbackFeedMmMin = 1000.0);

} // namespace cnc
