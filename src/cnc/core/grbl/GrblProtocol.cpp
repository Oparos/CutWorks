#include "cnc/core/grbl/GrblProtocol.h"

#include <QCoreApplication>
#include <QStringList>

namespace cnc {

namespace {

// Translation context for GRBL message tables.
constexpr char kCtx[] = "Grbl";

} // namespace

GrblResponse classifyGrblLine(const QString& line)
{
    const QString t = line.trimmed();
    if (t.isEmpty()) {
        return {GrblResponseKind::Unknown, 0};
    }
    if (t.startsWith('<') && t.endsWith('>')) {
        return {GrblResponseKind::Status, 0};
    }
    if (t.compare(QStringLiteral("ok"), Qt::CaseInsensitive) == 0) {
        return {GrblResponseKind::Ok, 0};
    }
    if (t.startsWith(QStringLiteral("error:"), Qt::CaseInsensitive)) {
        return {GrblResponseKind::Error, t.mid(6).trimmed().toInt()};
    }
    if (t.startsWith(QStringLiteral("ALARM:"), Qt::CaseInsensitive)) {
        return {GrblResponseKind::Alarm, t.mid(6).trimmed().toInt()};
    }
    if (t.startsWith('$') && t.contains('=')) {
        return {GrblResponseKind::Setting, 0};
    }
    if (t.startsWith(QStringLiteral("Grbl"), Qt::CaseInsensitive)) {
        return {GrblResponseKind::Welcome, 0};
    }
    if (t.startsWith('[') && t.endsWith(']')) {
        return {GrblResponseKind::Message, 0};
    }
    return {GrblResponseKind::Unknown, 0};
}

QString grblAxisLetter(int index)
{
    static const char* const letters[] = {"X", "Y", "Z", "A", "B", "C"};
    constexpr int count = sizeof(letters) / sizeof(letters[0]);
    if (index >= 0 && index < count) {
        return QString::fromLatin1(letters[index]);
    }
    return QStringLiteral("?");
}

GrblState grblStateFromString(const QString& token)
{
    // States can carry a sub-code, e.g. "Hold:0" or "Door:1".
    const QString name = token.section(':', 0, 0).trimmed();

    if (name == QLatin1String("Idle"))  return GrblState::Idle;
    if (name == QLatin1String("Run"))   return GrblState::Run;
    if (name == QLatin1String("Hold"))  return GrblState::Hold;
    if (name == QLatin1String("Jog"))   return GrblState::Jog;
    if (name == QLatin1String("Alarm")) return GrblState::Alarm;
    if (name == QLatin1String("Door"))  return GrblState::Door;
    if (name == QLatin1String("Check")) return GrblState::Check;
    if (name == QLatin1String("Home"))  return GrblState::Home;
    if (name == QLatin1String("Sleep")) return GrblState::Sleep;
    return GrblState::Unknown;
}

std::optional<GrblStatus> parseGrblStatus(const QString& line)
{
    const QString t = line.trimmed();
    if (!(t.startsWith('<') && t.endsWith('>'))) {
        return std::nullopt;
    }

    const QString content = t.mid(1, t.length() - 2);
    const QStringList parts = content.split('|');
    if (parts.isEmpty()) {
        return std::nullopt;
    }

    GrblStatus status;
    status.rawState = parts.first();
    status.state = grblStateFromString(parts.first());

    for (int i = 1; i < parts.size(); ++i) {
        const QString& field = parts.at(i);

        if (field.startsWith(QLatin1String("MPos:")) || field.startsWith(QLatin1String("WPos:"))) {
            status.positionIsMachine = field.startsWith(QLatin1String("MPos:"));
            const QStringList coords = field.mid(5).split(',');
            if (!coords.isEmpty()) {
                status.hasPosition = true;
                status.position.clear();
                status.position.reserve(coords.size());
                for (const QString& coord : coords) {
                    status.position.push_back(coord.toDouble());
                }
            }
        }
        else if (field.startsWith(QLatin1String("FS:"))) {
            // "FS:feed,spindle" — we keep the feed value.
            const QStringList fs = field.mid(3).split(',');
            if (!fs.isEmpty()) {
                status.feed = fs.first().toDouble();
            }
        }
        else if (field.startsWith(QLatin1String("F:"))) {
            status.feed = field.mid(2).toDouble();
        }
    }

    return status;
}

std::optional<GrblSetting> parseGrblSetting(const QString& line)
{
    const QString t = line.trimmed();
    const int equalIdx = t.indexOf('=');
    if (!t.startsWith('$') || equalIdx == -1) {
        return std::nullopt;
    }

    GrblSetting setting;
    setting.code = t.left(equalIdx).trimmed();

    const QString remainder = t.mid(equalIdx + 1).trimmed();
    const int open = remainder.indexOf('(');
    const int close = remainder.lastIndexOf(')');
    if (open != -1 && close != -1 && close > open) {
        setting.value = remainder.left(open).trimmed();
        setting.description = remainder.mid(open + 1, close - open - 1).trimmed();
    }
    else {
        setting.value = remainder;
    }

    return setting;
}

QByteArray encodeGrblCommand(const QString& command)
{
    // Single-character real-time commands are sent as one raw byte, no newline.
    if (command.size() == 1) {
        const char16_t c = command.at(0).unicode();
        const bool isRealtime =
            c == u'?' || c == u'!' || c == u'~' ||
            c == 0x18 ||                 // Ctrl-X soft reset
            c == 0x84 ||                 // safety door
            c == 0x85 ||                 // jog cancel
            (c >= 0x90 && c <= 0x9D);    // feed / rapid / spindle overrides
        if (isRealtime) {
            return QByteArray(1, static_cast<char>(c & 0xFF));
        }
    }
    return command.toUtf8() + '\n';
}

QString grblErrorText(int code)
{
    switch (code) {
    case 1:  return QCoreApplication::translate(kCtx, "G-code words consist of a letter and a value. Letter was not found.");
    case 2:  return QCoreApplication::translate(kCtx, "Missing the expected G-code word value or numeric value format is not valid.");
    case 3:  return QCoreApplication::translate(kCtx, "'$' system command was not recognized or supported.");
    case 4:  return QCoreApplication::translate(kCtx, "Negative value received for an expected positive value.");
    case 5:  return QCoreApplication::translate(kCtx, "Homing cycle failure. Homing is not configured via settings.");
    case 6:  return QCoreApplication::translate(kCtx, "Step pulse time must be greater or equal to 2 microseconds.");
    case 7:  return QCoreApplication::translate(kCtx, "A settings read failed. Auto-restoring affected settings to default values.");
    case 8:  return QCoreApplication::translate(kCtx, "'$' command cannot be used unless controller state is IDLE.");
    case 9:  return QCoreApplication::translate(kCtx, "G-code commands are locked out during alarm or jog state.");
    case 10: return QCoreApplication::translate(kCtx, "Soft limits cannot be enabled without homing also enabled.");
    case 11: return QCoreApplication::translate(kCtx, "Max characters per line exceeded. Received command line was not executed.");
    case 12: return QCoreApplication::translate(kCtx, "'$' setting value causes the step rate to exceed the maximum supported.");
    case 13: return QCoreApplication::translate(kCtx, "Safety door detected as opened and door state initiated.");
    case 14: return QCoreApplication::translate(kCtx, "Build info or startup line exceeded line length limit. Line not stored.");
    case 15: return QCoreApplication::translate(kCtx, "Jog target exceeds machine travel. Jog command has been ignored.");
    case 16: return QCoreApplication::translate(kCtx, "Jog command has no '=' or contains prohibited g-code.");
    case 17: return QCoreApplication::translate(kCtx, "Laser mode requires PWM output.");
    case 20: return QCoreApplication::translate(kCtx, "Unsupported or invalid g-code command found in block.");
    case 21: return QCoreApplication::translate(kCtx, "More than one g-code command from same modal group found in block.");
    case 22: return QCoreApplication::translate(kCtx, "Feed rate has not yet been set or is undefined.");
    case 23: return QCoreApplication::translate(kCtx, "G-code command in block requires an integer value.");
    case 24: return QCoreApplication::translate(kCtx, "More than one g-code command that requires axis words found in block.");
    case 25: return QCoreApplication::translate(kCtx, "Repeated g-code word found in block.");
    case 26: return QCoreApplication::translate(kCtx, "No axis words found in block for g-code command or current modal state which requires them.");
    case 27: return QCoreApplication::translate(kCtx, "Line number value is invalid.");
    case 28: return QCoreApplication::translate(kCtx, "G-code command is missing a required value word.");
    case 29: return QCoreApplication::translate(kCtx, "G59.x work coordinate systems are not supported.");
    case 30: return QCoreApplication::translate(kCtx, "G53 only allowed with G0 and G1 motion modes.");
    case 31: return QCoreApplication::translate(kCtx, "Axis words found in block when no command or current modal state uses them.");
    case 32: return QCoreApplication::translate(kCtx, "G2 and G3 arcs require at least one in-plane axis word.");
    case 33: return QCoreApplication::translate(kCtx, "Motion command target is invalid.");
    case 34: return QCoreApplication::translate(kCtx, "Arc radius value is invalid.");
    case 35: return QCoreApplication::translate(kCtx, "G2 and G3 arcs require at least one in-plane offset word.");
    case 36: return QCoreApplication::translate(kCtx, "Unused value words found in block.");
    case 37: return QCoreApplication::translate(kCtx, "G43.1 dynamic tool length offset is not assigned to configured tool length axis.");
    case 38: return QCoreApplication::translate(kCtx, "Tool number greater than max supported value or undefined tool selected.");
    case 39: return QCoreApplication::translate(kCtx, "Value out of range.");
    case 40: return QCoreApplication::translate(kCtx, "G-code command not allowed when tool change is pending.");
    case 43: return QCoreApplication::translate(kCtx, "Max. feed rate exceeded.");
    case 44: return QCoreApplication::translate(kCtx, "RPM out of range.");
    case 50: return QCoreApplication::translate(kCtx, "Emergency stop active.");
    case 51: return QCoreApplication::translate(kCtx, "Motor fault.");
    case 52: return QCoreApplication::translate(kCtx, "Setting value is out of range.");
    case 53: return QCoreApplication::translate(kCtx, "Setting is not available, possibly due to limited driver support.");
    case 56: return QCoreApplication::translate(kCtx, "Coordinate system is locked.");
    default: return QCoreApplication::translate(kCtx, "Unknown or unlisted error code.");
    }
}

QString grblAlarmText(int code)
{
    switch (code) {
    case 1:  return QCoreApplication::translate(kCtx, "Hard limit triggered. Machine position is likely lost. Type $X to unlock, then re-home.");
    case 2:  return QCoreApplication::translate(kCtx, "G-code motion target exceeds machine travel (soft limit). Type $X to unlock.");
    case 3:  return QCoreApplication::translate(kCtx, "Reset while in motion. Position not guaranteed. Type $X to unlock and re-home.");
    case 4:  return QCoreApplication::translate(kCtx, "Probe fail. Probe is not in the expected initial state before starting the probe cycle.");
    case 5:  return QCoreApplication::translate(kCtx, "Probe fail. Probe did not contact the workpiece within the programmed travel.");
    case 6:  return QCoreApplication::translate(kCtx, "Homing fail. Reset during active homing cycle.");
    case 7:  return QCoreApplication::translate(kCtx, "Homing fail. Safety door was opened during active homing cycle.");
    case 8:  return QCoreApplication::translate(kCtx, "Homing fail. Cycle failed to clear limit switch when pulling off.");
    case 9:  return QCoreApplication::translate(kCtx, "Homing fail. Could not find limit switch within search distance.");
    case 10: return QCoreApplication::translate(kCtx, "Homing fail. Second X-axis homing switch failed to trigger.");
    case 11: return QCoreApplication::translate(kCtx, "Homing fail. Second Y-axis homing switch failed to trigger.");
    case 12: return QCoreApplication::translate(kCtx, "Homing fail. Second Z-axis homing switch failed to trigger.");
    case 13: return QCoreApplication::translate(kCtx, "Critical event. E-stop triggered. Clear the E-stop and reset.");
    case 14: return QCoreApplication::translate(kCtx, "Motor fault. Check motor driver alarms.");
    default: return QCoreApplication::translate(kCtx, "Unknown machine alarm. Type $X to attempt unlock.");
    }
}

} // namespace cnc
