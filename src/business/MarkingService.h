// Layer 2 (business logic): marking by the lecturer (UC-08 to UC-11; FR-11 to FR-14; BR-01, BR-05, BR-06).
#pragma once

#include <string>
#include <vector>

#include "AuditService.h"
#include "PenaltyStrategy.h"
#include "Ports.h"

namespace lms {

class MarkingService {
public:
    MarkingService(ISubmissionRepository& submissions, IAssignmentRepository& assignments, ICourseRepository& courses,
                   PenaltyStrategy& penalty, AuditService& audit, Clock& clock, UnitOfWork& uow);

    // FR-11: the submissions of an assignment of a course the lecturer teaches.
    std::vector<SubmissionRow> listSubmissions(const User& actor, int assignmentId);

    // FR-13: shows the raw mark, the penalty and the final mark. Saves nothing.
    MarkPreview previewMark(const User& actor, int submissionId, double rawMark);

    // FR-12, FR-14: saves the mark, the status change and the audit entry in one transaction.
    // The feedback is required. For a change of an earlier mark the reason is required (BR-06);
    // for the first mark the reason is "initial marking" (BR-05).
    void saveMark(const User& actor, int submissionId, double rawMark, const std::string& feedback,
                  const std::string& reason);

    // The latest version of the file, for the lecturer or for the Head of Department of the course.
    StoredFile exportFile(const User& actor, int submissionId);

private:
    struct Loaded {
        Submission submission;
        Assignment assignment;
    };
    Loaded requireSubmissionOfLecturer(const User& actor, int submissionId);

    ISubmissionRepository& submissions_;
    IAssignmentRepository& assignments_;
    ICourseRepository& courses_;
    PenaltyStrategy& penalty_;
    AuditService& audit_;
    Clock& clock_;
    UnitOfWork& uow_;
};

}  // namespace lms
