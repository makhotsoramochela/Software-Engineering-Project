// Layer 1 (presentation): small helpers to print text in columns. Everything is text (NFR-12).
#pragma once

#include <cstdio>
#include <optional>
#include <string>

namespace lms {
namespace format {

// Pads (or cuts) the text to exactly the given width. At least one space is always left at the
// end, so a long text never touches the next column.
inline std::string cell(const std::string& text, std::size_t width) {
    if (width < 3) return text.substr(0, width);
    std::string shown = text.size() >= width ? text.substr(0, width - 2) + "~" : text;
    return shown + std::string(width - shown.size(), ' ');
}

inline std::string number(double value) {
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%.1f", value);
    return buffer;
}

inline std::string number(const std::optional<double>& value) {
    return value ? number(*value) : "-";
}

}  // namespace format
}  // namespace lms
