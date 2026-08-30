#include "cnc/core/SerialLink.h"

#include <QSerialPortInfo>

namespace cnc {

SerialLink::SerialLink(QObject* parent)
    : QObject(parent)
{
    connect(&m_port, &QSerialPort::readyRead, this, &SerialLink::onReadyRead);
    connect(&m_port, &QSerialPort::errorOccurred, this, &SerialLink::onErrorOccurred);
}

QList<SerialPortInfo> SerialLink::availablePorts()
{
    QList<SerialPortInfo> ports;
    for (const QSerialPortInfo& info : QSerialPortInfo::availablePorts()) {
        ports.append(SerialPortInfo{info.portName(), info.description()});
    }
    return ports;
}

bool SerialLink::open(const QString& portName, int baudRate)
{
    if (m_port.isOpen()) {
        m_port.close();
    }

    m_port.setPortName(portName);
    m_port.setBaudRate(baudRate);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    m_readBuffer.clear();

    if (m_port.open(QIODevice::ReadWrite)) {
        emit openedChanged(true);
        return true;
    }
    return false;
}

void SerialLink::close()
{
    if (m_port.isOpen()) {
        m_port.close();
        emit openedChanged(false);
    }
}

bool SerialLink::isOpen() const
{
    return m_port.isOpen();
}

void SerialLink::sendBytes(const QByteArray& bytes)
{
    if (m_port.isOpen() && m_port.isWritable()) {
        m_port.write(bytes);
    }
}

void SerialLink::onReadyRead()
{
    m_readBuffer += QString::fromUtf8(m_port.readAll());

    // GRBL answers line by line; emit each complete line, keep any partial tail.
    int newlineIndex = m_readBuffer.indexOf('\n');
    while (newlineIndex != -1) {
        const QString line = m_readBuffer.left(newlineIndex).trimmed();
        m_readBuffer.remove(0, newlineIndex + 1);
        if (!line.isEmpty()) {
            emit lineReceived(line);
        }
        newlineIndex = m_readBuffer.indexOf('\n');
    }
}

void SerialLink::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError || error == QSerialPort::TimeoutError) {
        return;
    }

    emit errorOccurred(m_port.errorString());
    if (m_port.isOpen()) {
        close();
    }
}

} // namespace cnc
