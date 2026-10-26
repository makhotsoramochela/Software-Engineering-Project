#include "PenaltyStrategy.h"

#include <algorithm>
#include <cmath>

namespace lms {

int TenPercentPerDayPenalty::daysLate(const DateTime& due, const DateTime& submittedAt) const {
    const long long late = due.minutesUntil(submittedAt);  // minutes after the deadline
    if (late <= 0) return 0;
    return static_cast<int>((late + 1439) / 1440);  // divide by 24 hours and round up
}

PenaltyResult TenPercentPerDayPenalty::apply(double rawMark, int maxMark, int daysLate) const {
    // The marks are worked out in tenths of a mark, so no rounding error can appear.
    const long long rawTenths = std::llround(rawMark * 10.0);
    if (daysLate <= 0) return {0.0, static_cast<double>(rawTenths) / 10.0};
    if (daysLate > LAST_PENALISED_DAY) return {static_cast<double>(rawTenths) / 10.0, 0.0};
    // n x 10% of the maximum mark M, in tenths: n x (10 / 100) x M x 10 = n x M x RATE_PERCENT / 10
    const long long penaltyTenths = static_cast<long long>(daysLate) * maxMark * RATE_PERCENT / 10;
    const long long finalTenths = std::max<long long>(0, rawTenths - penaltyTenths);
    return {static_cast<double>(penaltyTenths) / 10.0, static_cast<double>(finalTenths) / 10.0};
}

}  // namespace lms
