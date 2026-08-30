#pragma once

#include "cnc/core/SerialPortInfo.h"

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QSerialPort>
#include <QString>

namespace cnc {

// Asynchronous serial transport. Owns a QSerialPort, frames incoming bytes into
// whole lines, and writes raw bytes out. It carries no GRBL semantics — it is a
// byte pipe that never blocks the caller.
class SerialLink : public QObject
{
    Q_OBJECT

public:
    explicit SerialLink(QObject* parent = nullptr);

    // Ports currently reported by the OS.
    static QList<SerialPortInfo> availablePorts();

    bool open(const QString& portName, int baudRate);
    void close();
    bool isOpen() const;

    // Write raw bytes exactly as given (already framed by the caller).
    void sendBytes(const QByteArray& bytes);

signals:
    void lineReceived(const QString& line);
    void openedChanged(bool open);
    void errorOccurred(const QString& message);

private:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);

    QSerialPort m_port;
    QString m_readBuffer;  // holds a partial trailing line between reads
};

} // namespace cnc
