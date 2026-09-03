#include "cnc/core/grbl/GrblSettings.h"

#include <QCoreApplication>
#include <QHash>

namespace cnc {

namespace {
constexpr char kCtx[] = "GrblSettings";

const QHash<QString, QString>& descriptions()
{
    // Built once on first use. Uses QCoreApplication::translate so the table is
    // translatable without pulling Qt Widgets into the backend.
    static const QHash<QString, QString> table = {
        {"$0",  QCoreApplication::translate(kCtx, "Step pulse time (microseconds)")},
        {"$1",  QCoreApplication::translate(kCtx, "Step idle delay (milliseconds)")},
        {"$2",  QCoreApplication::translate(kCtx, "Step pulse invert (axis mask)")},
        {"$3",  QCoreApplication::translate(kCtx, "Step direction invert (axis mask)")},
        {"$4",  QCoreApplication::translate(kCtx, "Invert step enable pin (0=no, 1=yes)")},
        {"$5",  QCoreApplication::translate(kCtx, "Invert limit pins (0=no, 1=yes)")},
        {"$6",  QCoreApplication::translate(kCtx, "Invert probe pin (0=no, 1=yes)")},
        {"$9",  QCoreApplication::translate(kCtx, "Spindle PWM (bit 0 enables the rest)")},
        {"$10", QCoreApplication::translate(kCtx, "Status report options (mask)")},
        {"$11", QCoreApplication::translate(kCtx, "Junction deviation (mm)")},
        {"$12", QCoreApplication::translate(kCtx, "Arc tolerance (mm)")},
        {"$13", QCoreApplication::translate(kCtx, "Report in inches (0=mm, 1=inch)")},
        {"$14", QCoreApplication::translate(kCtx, "Invert control inputs (mask)")},
        {"$15", QCoreApplication::translate(kCtx, "Invert coolant pins (mask)")},
        {"$16", QCoreApplication::translate(kCtx, "Invert spindle pins (mask)")},
        {"$17", QCoreApplication::translate(kCtx, "Disable pull-up resistors for control inputs (mask)")},
        {"$18", QCoreApplication::translate(kCtx, "Disable pull-up resistors for limit pins (mask)")},
        {"$19", QCoreApplication::translate(kCtx, "Disable pull-up resistor for probe (boolean)")},
        {"$20", QCoreApplication::translate(kCtx, "Soft limits enable (0=off, 1=on)")},
        {"$21", QCoreApplication::translate(kCtx, "Hard limits enable (0=off, 1=on)")},
        {"$22", QCoreApplication::translate(kCtx, "Homing cycle enable (axis mask in grblHAL)")},
        {"$23", QCoreApplication::translate(kCtx, "Homing direction invert (mask)")},
        {"$24", QCoreApplication::translate(kCtx, "Homing locate feed rate - slow (mm/min)")},
        {"$25", QCoreApplication::translate(kCtx, "Homing seek feed rate - fast (mm/min)")},
        {"$26", QCoreApplication::translate(kCtx, "Homing switch debounce delay (msec)")},
        {"$27", QCoreApplication::translate(kCtx, "Homing pull-off distance (mm)")},
        {"$28", QCoreApplication::translate(kCtx, "G73 retract distance (mm)")},
        {"$29", QCoreApplication::translate(kCtx, "Step pulse delay (ms)")},
        {"$30", QCoreApplication::translate(kCtx, "Maximum spindle speed / laser power")},
        {"$31", QCoreApplication::translate(kCtx, "Minimum spindle speed / laser power")},
        {"$32", QCoreApplication::translate(kCtx, "Laser mode enable (0=off, 1=on)")},
        {"$33", QCoreApplication::translate(kCtx, "Spindle PWM frequency (Hz)")},
        {"$34", QCoreApplication::translate(kCtx, "Spindle/laser off value")},
        {"$35", QCoreApplication::translate(kCtx, "Spindle/laser minimum value")},
        {"$36", QCoreApplication::translate(kCtx, "Spindle/laser maximum value")},
        {"$37", QCoreApplication::translate(kCtx, "Stepper de-energize mask")},
        {"$39", QCoreApplication::translate(kCtx, "Enable printable real-time command characters")},
        {"$40", QCoreApplication::translate(kCtx, "Apply soft limits for jog commands (0=no, 1=yes)")},
        {"$43", QCoreApplication::translate(kCtx, "Number of homing passes")},
        {"$44", QCoreApplication::translate(kCtx, "Homing cycle 1 (mask)")},
        {"$45", QCoreApplication::translate(kCtx, "Homing cycle 2 (mask)")},
        {"$46", QCoreApplication::translate(kCtx, "Homing cycle 3 (mask)")},
        {"$47", QCoreApplication::translate(kCtx, "Homing cycle 4 (mask)")},
        {"$48", QCoreApplication::translate(kCtx, "Homing cycle 5 (mask)")},
        {"$62", QCoreApplication::translate(kCtx, "Sleep enable")},
        {"$63", QCoreApplication::translate(kCtx, "Feed hold actions")},
        {"$64", QCoreApplication::translate(kCtx, "Force initial alarm on start")},
        {"$65", QCoreApplication::translate(kCtx, "Require homing on machine start")},
        {"$100", QCoreApplication::translate(kCtx, "X-axis: steps per millimeter")},
        {"$101", QCoreApplication::translate(kCtx, "Y-axis: steps per millimeter")},
        {"$102", QCoreApplication::translate(kCtx, "Z-axis: steps per millimeter")},
        {"$103", QCoreApplication::translate(kCtx, "A-axis: steps per degree")},
        {"$110", QCoreApplication::translate(kCtx, "X-axis: max rate (mm/min)")},
        {"$111", QCoreApplication::translate(kCtx, "Y-axis: max rate (mm/min)")},
        {"$112", QCoreApplication::translate(kCtx, "Z-axis: max rate (mm/min)")},
        {"$113", QCoreApplication::translate(kCtx, "A-axis: max rate (deg/min)")},
        {"$120", QCoreApplication::translate(kCtx, "X-axis: acceleration (mm/sec^2)")},
        {"$121", QCoreApplication::translate(kCtx, "Y-axis: acceleration (mm/sec^2)")},
        {"$122", QCoreApplication::translate(kCtx, "Z-axis: acceleration (mm/sec^2)")},
        {"$123", QCoreApplication::translate(kCtx, "A-axis: acceleration (deg/sec^2)")},
        {"$130", QCoreApplication::translate(kCtx, "X-axis: max travel (mm)")},
        {"$131", QCoreApplication::translate(kCtx, "Y-axis: max travel (mm)")},
        {"$132", QCoreApplication::translate(kCtx, "Z-axis: max travel (mm)")},
        {"$133", QCoreApplication::translate(kCtx, "A-axis: max travel (degrees)")},
        {"$350", QCoreApplication::translate(kCtx, "THC mode (0=Off, 1=Arc voltage, 2=Up/Down, 3=Arc OK only)")},
        {"$351", QCoreApplication::translate(kCtx, "THC: activation delay after Arc OK (seconds)")},
        {"$352", QCoreApplication::translate(kCtx, "THC: voltage tolerance threshold (V)")},
        {"$353", QCoreApplication::translate(kCtx, "THC: proportional gain (P)")},
        {"$354", QCoreApplication::translate(kCtx, "THC: integral gain (I)")},
        {"$355", QCoreApplication::translate(kCtx, "THC: derivative gain (D)")},
        {"$356", QCoreApplication::translate(kCtx, "THC: VAD threshold - anti-dive on slowdowns (%)")},
        {"$357", QCoreApplication::translate(kCtx, "THC: void override threshold (%)")},
        {"$358", QCoreApplication::translate(kCtx, "ARC: torch ignition fail timeout (seconds)")},
        {"$359", QCoreApplication::translate(kCtx, "ARC: ignition retry delay (seconds)")},
        {"$360", QCoreApplication::translate(kCtx, "ARC: maximum ignition retries")},
        {"$361", QCoreApplication::translate(kCtx, "ARC: arc voltage input scale")},
        {"$362", QCoreApplication::translate(kCtx, "ARC: arc voltage input offset")},
        {"$363", QCoreApplication::translate(kCtx, "ARC: height per volt (mm/V, manual)")},
        {"$364", QCoreApplication::translate(kCtx, "ARC: Ok high voltage threshold (V)")},
        {"$365", QCoreApplication::translate(kCtx, "ARC: Ok low voltage threshold (V)")},
        {"$366", QCoreApplication::translate(kCtx, "PORTS: arc voltage input port (analog)")},
        {"$367", QCoreApplication::translate(kCtx, "PORTS: Arc OK signal port (digital)")},
        {"$368", QCoreApplication::translate(kCtx, "PORTS: cutter down signal port (external THC)")},
        {"$369", QCoreApplication::translate(kCtx, "PORTS: cutter up signal port (external THC)")},
        {"$370", QCoreApplication::translate(kCtx, "Invert I/O input ports (mask)")},
        {"$372", QCoreApplication::translate(kCtx, "Invert I/O output ports (mask)")},
        {"$376", QCoreApplication::translate(kCtx, "Rotary axes (mask)")},
        {"$384", QCoreApplication::translate(kCtx, "Disable G92 persistence")},
        {"$394", QCoreApplication::translate(kCtx, "Spindle turn-on delay (s)")},
        {"$398", QCoreApplication::translate(kCtx, "Planner buffer blocks")},
        {"$481", QCoreApplication::translate(kCtx, "Status auto-refresh interval (ms)")},
        {"$484", QCoreApplication::translate(kCtx, "Require unlock ($X) after E-Stop")},
        {"$486", QCoreApplication::translate(kCtx, "Coordinate system lock (mask)")},
        {"$650", QCoreApplication::translate(kCtx, "File system options (mask)")},
        {"$673", QCoreApplication::translate(kCtx, "Coolant turn-on delay (s)")},
        {"$674", QCoreApplication::translate(kCtx, "PLASMA: plugin options (bit0=virtual ports, bit1=sync Z)")},
        {"$682", QCoreApplication::translate(kCtx, "THC: Z-axis speed during corrections (% of XY feed)")},
    };
    return table;
}

} // namespace

QString grblSettingDescription(const QString& code)
{
    return descriptions().value(code, QCoreApplication::translate(kCtx, "Advanced setting / grblHAL"));
}

} // namespace cnc
