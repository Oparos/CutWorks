#pragma once

#include <QString>

#include <optional>
#include <vector>

namespace cnc {

// Machine state reported inside a GRBL status line, e.g. <Idle|...> or <Hold:0|...>.
enum class GrblState
{
    Unknown,
    Idle,
    Run,
    Hold,
    Jog,
    Alarm,
    Door,
    Check,
    Home,
    Sleep
};

// Parsed contents of a GRBL status report ("<...>" line).
struct GrblStatus
{
    GrblState state = GrblState::Unknown;
    QString rawState;                 // original token, e.g. "Run" or "Hold:0"

    bool hasPosition = false;
    bool positionIsMachine = false;   // true = MPos, false = WPos
    // One value per axis the controller reports, in order X, Y, Z, A, B, C...
    // A 3-axis machine reports 3; a 4-axis machine reports 4. No axis count is
    // assumed anywhere.
    std::vector<double> position;

    std::optional<double> feed;       // current feed rate, if reported
};

// A single GRBL setting line, e.g. "$110=1000.0 (x max rate, mm/min)".
struct GrblSetting
{
    QString code;         // "$110"
    QString value;        // "1000.0"
    QString description;  // "x max rate, mm/min" (may be empty)
};

// How one interpreted GRBL line should be treated. Kept minimal: details for
// Status / Setting are parsed by dedicated functions when needed.
enum class GrblResponseKind
{
    Ok,       // "ok"
    Error,    // "error:N"
    Alarm,    // "ALARM:N"
    Status,   // "<...>"
    Setting,  // "$N=..."
    Welcome,  // startup banner
    Message,  // "[...]"
    Unknown   // anything else
};

struct GrblResponse
{
    GrblResponseKind kind = GrblResponseKind::Unknown;
    int code = 0;  // valid for Error / Alarm
};

// Feed-rate override actions (GRBL real-time commands 0x90-0x94).
enum class FeedOverride
{
    Reset,    // back to 100%
    Plus10,
    Minus10,
    Plus1,
    Minus1
};

// Category for a line shown in the console, so the UI can color it without the
// backend knowing anything about colors or HTML.
enum class LogCategory
{
    Sent,
    Received,
    Info,
    Warning,
    Error
};

} // namespace cnc
