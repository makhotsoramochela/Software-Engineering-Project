// Layer 2 (business logic): moderation and release by the Head of Department
// (UC-12 to UC-14; FR-20 to FR-23, FR-25; BR-06, BR-07).
#pragma once

#include <string>
#include <vector>

#include "AuditService.h"
#include "MessageService.h"
#include "Ports.h"

namespace lms {

class ModerationService {
public:
    ModerationService(ISubmissionRepository& submissions, IAssignmentRepository& assignments,
                      ICourseRepository& courses, AuditService& audit, MessageService& messages, Clock& clock,
                      UnitOfWork& uow);

    // FR-20: the submissions with the status Marked in the department of the Head of Department.
    std::vector<SubmissionRow> listMarked(const User& actor);

    // FR-21: the three things that can be done with a Marked submission.
    void approve(const User& actor, int submissionId);
    void adjust(const User& actor, int submissionId, double newFinal, const std::string& reason);
    void returnToLecturer(const User& actor, int submissionId, const std::string& comment);

    std::vector<SubmissionRow> listModerated(const User& actor, int assignmentId);

    // FR-22, BR-07: only a Moderated submission can be released. Returns the number released.
    void release(const User& actor, int submissionId);
    int releaseAssignment(const User& actor, int assignmentId);

    // FR-25: for each assignment of the department, the number of Marked, Moderated and Released submissions.
    std::vector<AssignmentSummary> summary(const User& actor);

private:
    struct Loaded {
        Submission submission;
        Assignment assignment;
        Course course;
    };
    Loaded requireSubmissionOfDepartment(const User& actor, int submissionId);
    void releaseInside(const User& actor, Loaded& loaded);

    ISubmissionRepository& submissions_;
    IAssignmentRepository& assignments_;
    ICourseRepository& courses_;
    AuditService& audit_;
    MessageService& messages_;
    Clock& clock_;
    UnitOfWork& uow_;
};

}  // namespace lms
