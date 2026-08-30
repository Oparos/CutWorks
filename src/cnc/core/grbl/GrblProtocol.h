#pragma once

#include "cnc/core/grbl/GrblTypes.h"

#include <QByteArray>
#include <QString>

#include <optional>

// Pure, UI-free helpers that encode the GRBL protocol. No state, no Qt widgets.
namespace cnc {

// Classify one line received from the controller (already trimmed or not).
GrblResponse classifyGrblLine(const QString& line);

// Parse a "<...>" status line. Returns nullopt if the line is not a status report.
std::optional<GrblStatus> parseGrblStatus(const QString& line);

// Parse a "$N=value (description)" setting line. Returns nullopt if not a setting.
std::optional<GrblSetting> parseGrblSetting(const QString& line);

// Map a GRBL state token ("Idle", "Hold:0", ...) to the GrblState enum.
GrblState grblStateFromString(const QString& token);

// Human-readable text for a GRBL error / alarm code.
QString grblErrorText(int code);
QString grblAlarmText(int code);

// Encode a command into the exact bytes to write to the port.
// Real-time commands (?, !, ~, 0x18 soft-reset, 0x85 jog-cancel, feed/spindle
// overrides 0x90-0x9D) are single raw bytes with no newline; every other
// command is UTF-8 terminated with '\n'.
QByteArray encodeGrblCommand(const QString& command);

} // namespace cnc
