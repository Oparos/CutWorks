#include "cnc/core/GrblController.h"

#include "cnc/core/grbl/GrblProtocol.h"

namespace cnc {

GrblController::GrblController(QObject* parent)
    : QObject(parent)
{
    connect(&m_link, &SerialLink::lineReceived, this, &GrblController::onLineReceived);
    connect(&m_link, &SerialLink::openedChanged, this, &GrblController::onOpenedChanged);
    connect(&m_link, &SerialLink::errorOccurred, this, [this](const QString& message) {
        emit connectionError(message);
        emit lineLogged(message, LogCategory::Error);
    });

    m_statusTimer.setInterval(kStatusPollIntervalMs);
    connect(&m_statusTimer, &QTimer::timeout, this, &GrblController::requestStatus);
}

QList<SerialPortInfo> GrblController::availablePorts() const
{
    return SerialLink::availablePorts();
}

bool GrblController::isConnected() const
{
    return m_link.isOpen();
}

bool GrblController::connectTo(const QString& portName, int baudRate)
{
    if (!m_link.open(portName, baudRate)) {
        const QString message = tr("Could not open port %1").arg(portName);
        emit connectionError(message);
        emit lineLogged(message, LogCategory::Error);
        return false;
    }
    return true;  // onOpenedChanged() takes over from here
}

void GrblController::disconnectFromMachine()
{
    m_link.close();
}

void GrblController::sendCommand(const QString& command)
{
    if (!isConnected()) {
        emit lineLogged(tr("No connection to machine."), LogCategory::Warning);
        return;
    }
    m_link.sendBytes(encodeGrblCommand(command));
    emit lineLogged(command, LogCategory::Sent);
}

void GrblController::requestStatus()
{
    // Sent silently (no console log) because it is polled continuously.
    if (isConnected()) {
        m_link.sendBytes(encodeGrblCommand(QStringLiteral("?")));
    }
}

void GrblController::requestSettings()
{
    sendCommand(QStringLiteral("$$"));
}

void GrblController::homeCycle()
{
    sendCommand(QStringLiteral("$H"));
}

void GrblController::zeroAxis(const QString& axis)
{
    // G10 L20 P1 sets the work coordinate origin of the active system.
    if (axis == QLatin1String("ALL")) {
        sendCommand(QStringLiteral("G10 L20 P1 X0 Y0 Z0"));
    }
    else {
        sendCommand(QStringLiteral("G10 L20 P1 ") + axis + QStringLiteral("0"));
    }
}

void GrblController::probeMaterialZ()
{
    // Move down until the probe/torch touches the material (max 100 mm),
    // then set the work Z, compensating for the floating-head slack.
    static constexpr double kFloatingHeadOffsetMm = 2.0;
    sendCommand(QStringLiteral("G38.2 Z-100 F100"));
    sendCommand(QStringLiteral("G92 Z-%1").arg(kFloatingHeadOffsetMm));
}

void GrblController::jog(double dx, double dy, double dz, double distanceMm, double feed)
{
    if (!isConnected()) {
        emit lineLogged(tr("No connection. Ignoring jog."), LogCategory::Warning);
        return;
    }

    QString cmd = QStringLiteral("$J=G91 G21");
    if (dx != 0.0) cmd += QStringLiteral(" X%1").arg(dx * distanceMm);
    if (dy != 0.0) cmd += QStringLiteral(" Y%1").arg(dy * distanceMm);
    if (dz != 0.0) cmd += QStringLiteral(" Z%1").arg(dz * distanceMm);
    cmd += QStringLiteral(" F%1").arg(feed);

    sendCommand(cmd);
}

void GrblController::cancelJog()
{
    if (!isConnected()) {
        return;
    }
    // Real-time jog cancel is a single 0x85 byte; log a friendly line instead
    // of the raw control character.
    m_link.sendBytes(encodeGrblCommand(QString(QChar(0x85))));
    emit lineLogged(tr("Jog cancel"), LogCategory::Info);
}

void GrblController::feedOverride(FeedOverride action)
{
    if (!isConnected()) {
        return;
    }

    unsigned char byte = 0;
    QString label;
    switch (action) {
    case FeedOverride::Reset:   byte = 0x90; label = tr("Feed override: 100%"); break;
    case FeedOverride::Plus10:  byte = 0x91; label = tr("Feed override: +10%"); break;
    case FeedOverride::Minus10: byte = 0x92; label = tr("Feed override: -10%"); break;
    case FeedOverride::Plus1:   byte = 0x93; label = tr("Feed override: +1%"); break;
    case FeedOverride::Minus1:  byte = 0x94; label = tr("Feed override: -1%"); break;
    }

    // Real-time single byte, no newline.
    m_link.sendBytes(QByteArray(1, static_cast<char>(byte)));
    emit lineLogged(label, LogCategory::Info);
}

void GrblController::setThcMode(int mode)
{
    if (!isConnected()) {
        return;
    }
    sendCommand(QStringLiteral("$350=%1").arg(mode));
}

void GrblController::setThcEnabled(bool enabled)
{
    if (!isConnected()) {
        return;
    }
    sendCommand(enabled ? QStringLiteral("M63 P2") : QStringLiteral("M62 P2"));
}

void GrblController::onOpenedChanged(bool open)
{
    if (open) {
        m_statusTimer.start();
        // Wake GRBL up after opening the port.
        m_link.sendBytes(QByteArrayLiteral("\r\n"));
    }
    else {
        m_statusTimer.stop();
    }
    emit connectionChanged(open);
}

void GrblController::onLineReceived(const QString& line)
{
    const GrblResponse response = classifyGrblLine(line);

    switch (response.kind) {
    case GrblResponseKind::Status:
        if (const auto status = parseGrblStatus(line)) {
            emit statusUpdated(*status);  // not logged: it is polled ~5x/second
        }
        return;

    case GrblResponseKind::Setting:
        if (const auto setting = parseGrblSetting(line)) {
            emit settingReceived(*setting);
        }
        return;

    case GrblResponseKind::Error: {
        const QString text = grblErrorText(response.code);
        emit responseReceived(GrblResponseKind::Error, response.code);
        emit errorReceived(response.code, text);
        emit lineLogged(tr("Error %1: %2").arg(response.code).arg(text), LogCategory::Error);
        return;
    }

    case GrblResponseKind::Alarm: {
        const QString text = grblAlarmText(response.code);
        emit responseReceived(GrblResponseKind::Alarm, response.code);
        emit alarmReceived(response.code, text);
        emit lineLogged(tr("ALARM %1: %2").arg(response.code).arg(text), LogCategory::Error);
        return;
    }

    case GrblResponseKind::Ok:
        emit responseReceived(GrblResponseKind::Ok, 0);
        emit lineLogged(line, LogCategory::Received);
        return;

    case GrblResponseKind::Welcome:
    case GrblResponseKind::Message:
    case GrblResponseKind::Unknown:
        emit lineLogged(line, LogCategory::Received);
        return;
    }
}

} // namespace cnc
