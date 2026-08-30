#pragma once

#include "cnc/core/SerialLink.h"
#include "cnc/core/SerialPortInfo.h"
#include "cnc/core/grbl/GrblTypes.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

namespace cnc {

// The CNC backend "brain". Owns the serial transport, polls the controller for
// status, and interprets every incoming line into typed signals. This is where
// all GRBL knowledge lives; the UI only reacts to the signals below.
class GrblController : public QObject
{
    Q_OBJECT

public:
    explicit GrblController(QObject* parent = nullptr);

    QList<SerialPortInfo> availablePorts() const;
    bool isConnected() const;

public slots:
    bool connectTo(const QString& portName, int baudRate);
    void disconnectFromMachine();

    // Send a manual/console command. Real-time framing is handled internally.
    void sendCommand(const QString& command);

    void requestStatus();    // '?'
    void requestSettings();  // '$$'

    // Machine intents. The exact G-code lives here (backend), never in the UI.
    void homeCycle();                    // '$H'
    void zeroAxis(const QString& axis);  // "X" / "Y" / "Z" / "ALL"
    void probeMaterialZ();               // probe down, then set work Z

    // Jog motion. dx/dy/dz are unit directions (-1, 0 or 1); distanceMm is the
    // travel (a step size, or a large value for continuous jog); feed is mm/min.
    // cancelJog() stops a running continuous jog (real-time 0x85).
    void jog(double dx, double dy, double dz, double distanceMm, double feed);
    void cancelJog();

    // Feed-rate override (real-time, applied on the fly).
    void feedOverride(cnc::FeedOverride action);

    // Torch height control (GRBL-HAL plasma). Detailed tuning ($351-$369, ...)
    // is done through the settings table; here we only pick the mode and toggle.
    void setThcMode(int mode);         // $350=<mode>
    void setThcEnabled(bool enabled);  // M63 P2 (on) / M62 P2 (off), needs virtual ports

signals:
    void connectionChanged(bool connected);
    void statusUpdated(const cnc::GrblStatus& status);
    void settingReceived(const cnc::GrblSetting& setting);
    void errorReceived(int code, const QString& message);
    void alarmReceived(int code, const QString& message);

    // Classified response kind, for streaming flow control (ok -> next line,
    // error/alarm -> abort). Emitted for ok / error / alarm lines.
    void responseReceived(cnc::GrblResponseKind kind, int code);

    // A line destined for the console, tagged so the UI can style it.
    void lineLogged(const QString& text, cnc::LogCategory category);

    // A transport-level failure (port could not open, cable pulled, ...).
    void connectionError(const QString& message);

private:
    void onLineReceived(const QString& line);
    void onOpenedChanged(bool open);

    SerialLink m_link;
    QTimer m_statusTimer;

    static constexpr int kStatusPollIntervalMs = 200;
};

} // namespace cnc
