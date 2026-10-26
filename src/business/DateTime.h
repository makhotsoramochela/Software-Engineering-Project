// Layer 2 (business logic): a date and time to the minute (BR-01: times are compared to the minute).
#pragma once

#include <string>

namespace lms {

class DateTime {
public:
    DateTime() = default;  // 1970-01-01T00:00

    // Accepts "YYYY-MM-DDTHH:MM" (the stored form) or "YYYY-MM-DD HH:MM".
    // Throws ValidationError if the text is not a real date and time.
    static DateTime parse(const std::string& text);
    static DateTime fromMinutes(long long minutesSinceEpoch);

    std::string toString() const;   // "YYYY-MM-DDTHH:MM", used in the database
    std::string toDisplay() const;  // "YYYY-MM-DD HH:MM", used on the screen

    long long minutes() const { return minutes_; }
    DateTime plusMinutes(long long minutes) const { return DateTime(minutes_ + minutes); }
    // Number of minutes from this time to the other time (negative if the other is earlier).
    long long minutesUntil(const DateTime& other) const { return other.minutes_ - minutes_; }

    bool operator==(const DateTime& o) const { return minutes_ == o.minutes_; }
    bool operator!=(const DateTime& o) const { return minutes_ != o.minutes_; }
    bool operator<(const DateTime& o) const { return minutes_ < o.minutes_; }
    bool operator<=(const DateTime& o) const { return minutes_ <= o.minutes_; }
    bool operator>(const DateTime& o) const { return minutes_ > o.minutes_; }
    bool operator>=(const DateTime& o) const { return minutes_ >= o.minutes_; }

private:
    explicit DateTime(long long minutes) : minutes_(minutes) {}
    long long minutes_ = 0;
};

}  // namespace lms
