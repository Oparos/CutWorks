#pragma once

#include <QString>

namespace cnc {

// Human-readable description for a GRBL / GRBL-HAL setting code ("$110", "$350",
// ...). Returns a generic fallback for codes not in the table. This is GRBL
// domain knowledge, kept in the backend so the UI only has to display it.
QString grblSettingDescription(const QString& code);

} // namespace cnc
