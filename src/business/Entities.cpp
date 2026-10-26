#include "Entities.h"

#include "Exceptions.h"

namespace lms {

void Submission::restoreMark(std::optional<double> raw, std::optional<double> penalty, std::optional<double> final,
                             std::optional<std::string> feedback, std::optional<int> markedBy) {
    rawMark_ = raw;
    penalty_ = penalty;
    finalMark_ = final;
    feedback_ = std::move(feedback);
    markedBy_ = markedBy;
}

void Submission::setMark(double raw, double penalty, double final, std::string feedback, int markedBy) {
    rawMark_ = raw;
    penalty_ = penalty;
    finalMark_ = final;
    feedback_ = std::move(feedback);
    markedBy_ = markedBy;
}

SubmissionStatus Submission::visibleStatus() const {
    if (status_ == SubmissionStatus::RELEASED) return SubmissionStatus::RELEASED;
    return daysLate_ > 0 ? SubmissionStatus::LATE : SubmissionStatus::SUBMITTED;
}

bool Submission::canBeReplaced(const DateTime& now, const DateTime& due) const {
    return status_ == SubmissionStatus::SUBMITTED && now <= due;
}

bool Submission::isAllowedChange(SubmissionStatus from, SubmissionStatus to) {
    using S = SubmissionStatus;
    switch (from) {
        case S::SUBMITTED:
            return to == S::MARKED;
        case S::LATE:
            return to == S::MARKED || to == S::SUBMITTED;  // Submitted: an extension (BR-04)
        case S::MARKED:
            return to == S::MODERATED || to == S::RETURNED;
        case S::RETURNED:
            return to == S::MARKED;
        case S::MODERATED:
            return to == S::RELEASED;
        case S::RELEASED:
            return false;
    }
    return false;
}

void Submission::changeStatus(SubmissionStatus to) {
    if (!isAllowedChange(status_, to)) {
        throw RuleError("A submission with the status " + displayName(status_) + " cannot change to " +
                        displayName(to) + ".");
    }
    status_ = to;
}

}  // namespace lms
