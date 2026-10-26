#include "SystemClock.h"

#include <ctime>

namespace lms {

DateTime SystemClock::now() const {
    const std::time_t t = std::time(nullptr);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &t);
#else
    localtime_r(&t, &local);
#endif
    char buffer[20];
    std::strftime(buffer, sizeof buffer, "%Y-%m-%dT%H:%M", &local);
    return DateTime::parse(buffer);
}

}  // namespace lms
