#pragma once

#include <QString>

namespace cnc {

// A serial port as reported by the OS, with a human-readable description.
// Replaces the legacy "COM3|description..." string packing with real fields.
struct SerialPortInfo
{
    QString name;         // e.g. "COM3" or "/dev/ttyUSB0"
    QString description;  // e.g. "USB-SERIAL CH340" (may be empty)

    // Text to show in a port picker: "COM3 (USB-SERIAL CH340)", or just "COM3".
    QString displayLabel() const
    {
        return description.isEmpty() ? name : name + " (" + description + ")";
    }
};

} // namespace cnc
