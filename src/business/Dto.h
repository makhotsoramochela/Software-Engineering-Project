// Layer 2 (business logic): small data holders that services return to the menus.
// They hold data and have no behaviour.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "DateTime.h"
#include "Types.h"

namespace lms {

struct AssignmentInfo {  // one row of "My assignments" for a lecturer
    int assignmentId = 0;
    std::string courseCode;
    std::string courseTitle;
    std::string title;
    DateTime deadline;
    int maxMark = 0;
};

struct StudentAssignmentRow {  // FR-15: one row of "My courses and assignments" for a student
    int assignmentId = 0;
    std::string courseCode;
    std::string courseTitle;
    std::string title;
    DateTime deadline;  // the deadline of this student (with any extension)
    int maxMark = 0;
    std::string statusText;  // "Not submitted" or the visible status (BR-02)
};

struct SubmissionRow {  // FR-11, FR-20: a submission as a lecturer or a Head of Department sees it
    int submissionId = 0;
    int assignmentId = 0;
    std::string courseCode;
    std::string assignmentTitle;
    int maxMark = 0;
    int studentId = 0;
    std::string studentName;
    std::string studentUsername;
    SubmissionStatus status = SubmissionStatus::SUBMITTED;
    int daysLate = 0;
    DateTime submittedAt;  // time of the latest version
    int versionCount = 1;
    std::optional<double> rawMark;
    std::optional<double> penalty;
    std::optional<double> finalMark;
    std::optional<std::string> feedback;
    std::string markedByName;  // empty until marked
};

struct SubmissionPreview {  // FR-16, FR-18: what the student sees before the file is saved
    int assignmentId = 0;
    std::string assignmentTitle;
    std::string fileName;
    std::vector<std::uint8_t> content;
    DateTime deadline;
    DateTime checkedAt;
    bool isLate = false;
    int daysLate = 0;
    int maxMark = 0;
    double maxPenaltyMarks = 0;  // marks taken from the maximum mark (all of it if the work gets zero)
    bool getsZero = false;       // BR-01: more than 72 hours late
    bool isResubmission = false;
    int nextVersionNo = 1;
};

struct Receipt {  // FR-16: shown after a submission is saved
    std::string assignmentTitle;
    std::string fileName;
    int versionNo = 1;
    DateTime submittedAt;
    SubmissionStatus status = SubmissionStatus::SUBMITTED;
    int daysLate = 0;
};

struct ResultRow {  // FR-19: a released mark with its feedback
    std::string courseCode;
    std::string assignmentTitle;
    int maxMark = 0;
    int daysLate = 0;
    double penalty = 0;
    double finalMark = 0;
    std::string feedback;
};

struct MarkPreview {  // FR-13: shown to the lecturer before a mark is saved
    double rawMark = 0;
    double penalty = 0;
    double finalMark = 0;
    int daysLate = 0;
    int maxMark = 0;
};

struct StoredFile {
    std::string fileName;
    std::vector<std::uint8_t> content;
};

struct AssignmentSummary {  // FR-25
    int assignmentId = 0;
    std::string courseCode;
    std::string title;
    int marked = 0;
    int moderated = 0;
    int released = 0;
};

}  // namespace lms
