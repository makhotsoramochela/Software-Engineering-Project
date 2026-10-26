// Layer 2 (business logic): Strategy pattern for the late penalty (design: Figure 3.7, BR-01).
// A different rule is a new subclass of PenaltyStrategy. No service changes.
#pragma once

#include "DateTime.h"

namespace lms {

struct PenaltyResult {
    double penalty = 0;    // marks taken off (all of the raw mark if the work gets zero)
    double finalMark = 0;  // never below 0
};

class PenaltyStrategy {
public:
    virtual ~PenaltyStrategy() = default;
    // Whole days between the deadline and the time of submission, rounded up. 0 if on time.
    virtual int daysLate(const DateTime& due, const DateTime& submittedAt) const = 0;
    virtual PenaltyResult apply(double rawMark, int maxMark, int daysLate) const = 0;
};

// BR-01: 10% of the maximum mark for each day late, for up to 3 days. After that the mark is 0.
class TenPercentPerDayPenalty : public PenaltyStrategy {
public:
    int daysLate(const DateTime& due, const DateTime& submittedAt) const override;
    PenaltyResult apply(double rawMark, int maxMark, int daysLate) const override;

private:
    static constexpr int RATE_PERCENT = 10;
    static constexpr int LAST_PENALISED_DAY = 3;
};

}  // namespace lms
