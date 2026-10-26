// Layer 2 (business logic): the functions of the student (UC-15 to UC-19; FR-15 to FR-19; BR-01 to BR-03).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "AssignmentService.h"
#include "PenaltyStrategy.h"
#include "Ports.h"

namespace lms {

class SubmissionService {
public:
    SubmissionService(ISubmissionRepository& submissions, IAssignmentRepository& assignments,
                      ICourseRepository& courses, AssignmentService& deadlines, PenaltyStrategy& penalty, Clock& clock,
                      UnitOfWork& uow);

    // FR-15: the courses of the student, with the assignments, the deadline of the student and the
    // status that BR-02 allows the student to see.
    std::vector<StudentAssignmentRow> listMyAssignments(const User& actor);

    // Step 1 of "Submit assignment": checks the file and the deadline and returns what the student must
    // see before the file is saved (for late work: the days late and the penalty, FR-18). Saves nothing.
    SubmissionPreview prepareSubmission(const User& actor, int assignmentId, const std::string& fileName,
                                        const std::vector<std::uint8_t>& content);

    // Step 2: saves the file (a new version if it is a resubmission) and returns the receipt.
    Receipt confirmSubmission(const User& actor, const SubmissionPreview& preview);

    // FR-19: only the released marks of the student, with the feedback.
    std::vector<ResultRow> listMyResults(const User& actor);

    static constexpr std::size_t MAX_BYTES = 5242880;  // 5 MB (FR-16)

private:
    ISubmissionRepository& submissions_;
    IAssignmentRepository& assignments_;
    ICourseRepository& courses_;
    AssignmentService& deadlines_;
    PenaltyStrategy& penalty_;
    Clock& clock_;
    UnitOfWork& uow_;
};

}  // namespace lms
