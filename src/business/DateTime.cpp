#include "DateTime.h"

#include <cstdio>

#include "Exceptions.h"

namespace lms {

namespace {

// Days from 1970-01-01 to the given civil date (H. Hinnant's algorithm).
long long daysFromCivil(long long y, unsigned m, unsigned d) {
    if (m <= 2) y -= 1;
    const long long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<long long>(doe) - 719468;
}

void civilFromDays(long long z, long long& y, unsigned& m, unsigned& d) {
    z += 719468;
    const long long era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = static_cast<long long>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp < 10 ? mp + 3 : mp - 9;
    if (m <= 2) y += 1;
}

}  // namespace

DateTime DateTime::parse(const std::string& text) {
    int year = 0, month = 0, day = 0, hour = 0, minute = 0;
    char separator = 0;
    int used = 0;
    const int found =
        std::sscanf(text.c_str(), "%4d-%2d-%2d%c%2d:%2d%n", &year, &month, &day, &separator, &hour, &minute, &used);
    const bool shapeOk = found == 6 && used == static_cast<int>(text.size()) && text.size() == 16 &&
                         (separator == 'T' || separator == ' ');
    if (!shapeOk) {
        throw ValidationError("A date and time must look like 2026-11-30 17:00 (year-month-day hour:minute).");
    }
    const long long days = daysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day));
    long long y2 = 0;
    unsigned m2 = 0, d2 = 0;
    civilFromDays(days, y2, m2, d2);
    const bool real = month >= 1 && month <= 12 && day >= 1 && hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 &&
                      y2 == year && static_cast<int>(m2) == month && static_cast<int>(d2) == day;
    if (!real) {
        throw ValidationError("That date or time does not exist: " + text);
    }
    return DateTime(days * 1440 + static_cast<long long>(hour) * 60 + minute);
}

DateTime DateTime::fromMinutes(long long minutesSinceEpoch) {
    return DateTime(minutesSinceEpoch);
}

std::string DateTime::toString() const {
    long long days = minutes_ / 1440;
    long long rest = minutes_ % 1440;
    if (rest < 0) {
        rest += 1440;
        days -= 1;
    }
    long long y = 0;
    unsigned m = 0, d = 0;
    civilFromDays(days, y, m, d);
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%04lld-%02u-%02uT%02lld:%02lld", y, m, d, rest / 60, rest % 60);
    return buffer;
}

std::string DateTime::toDisplay() const {
    std::string s = toString();
    s[10] = ' ';
    return s;
}

}  // namespace lms
